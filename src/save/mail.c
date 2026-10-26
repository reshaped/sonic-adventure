/* mail: pages written to the memory card for the browser to post, each
 * with a Chao or a record */
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "chao/chao.h"
#include "save/card.h"
#include "save/mail.h"
#include "save/save.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/history.h"
#include "system/loader.h"
#include "system/task.h"

/* the browser's internet settings: the flash partition and the packets
 * that hold them, and the size of the work area fmMngInit caches the
 * partition in */
#define ISP_PARTITION 2
#define ISP_FIRST_PACKET 0xc0
#define ISP_LAST_PACKET 0xcb
#define ISP_WORK_SIZE 0x4000
/* the word that marks a set of settings as present: "SEGA" in memory
 * order */
#define ISP_MAGIC 0x41474553
#define ISP_MAIL_SIZE 48

/* a set of the browser's internet settings, present if magic is
 * ISP_MAGIC */
struct isp_settings {
    Uint32 magic;
    Uint8 unk_04[0xac];
    /* the user's mail address, which the pages post as mailid. it ends at
     * a null or at the array's end */
    char mail[ISP_MAIL_SIZE];
    Uint8 unk_e0[0x84];
};

/* the settings as the flash holds them, in packets ISP_FIRST_PACKET to
 * ISP_LAST_PACKET */
struct isp_flash {
    Sint32 chosen; /* 1 if second is the set in use, else first */
    struct isp_settings first;
    struct isp_settings second;
    Uint8 unk_2cc[4];
};

/* the head of each record a page carries. crc is mail_crc of the
 * record's bytes after it, and magic gives the record's kind. starts and times
 * hold the console's play history, as history_read gives it */
struct mail_head {
    Uint32 crc;
    Uint32 magic;
    Uint32 starts[3];
    Uint16 times[24];
};

struct chao_mail {
    struct mail_head head;
    struct chao chao;
};

struct save_mail {
    struct mail_head head;
    struct save save;
};

/* a record of three values: a mission's id or a race's act, the time, and
 * the character */
struct values_mail {
    struct mail_head head;
    Sint32 values[3];
};

/* the records' kinds. each spells four CP932 characters, half-width kana
 * or !, from its high byte down */
#define CHAO_MAGIC 0xcdacb521
#define SAVE_MAGIC 0xc3ddbdb3
#define MISSION_MAGIC 0xb5d8b4dd
#define CART_MAGIC 0xb6b1c421

/* the spaces a writer puts ahead of its page */
#define PAGE_INDENT 64
/* a page's form has three hidden fields: mailid, dcid and chaodata.
 * chaodata carries the record, whatever its kind. for each field, _VALUE
 * is the offset from its name to its value, and _PLACEHOLDER the length of
 * the value a writer replaces */
#define MAILID_VALUE 15
#define MAILID_PLACEHOLDER 20
#define DCID_VALUE 13
#define DCID_PLACEHOLDER 25
#define CHAODATA_VALUE 17
#define CHAODATA_PLACEHOLDER 22

/* the CRC-16-CCITT polynomial, for a CRC taken high bit first */
#define CRC_POLYNOMIAL 0x1021

/* mail_scramble's words, each four ASCII characters from the high byte:
 * ZETA and NANA */
#define SCRAMBLE_MASK 0x5a455441
#define SCRAMBLE_STEP 0x4e414e41

static Bool isp_read(struct isp_flash *flash);
static int base64_encode(char *text, const Uint8 *from, int size);
static Uint16 mail_crc(int size, const Uint8 *data);
static void mail_scramble(Uint32 *data, int size);

/* the alphabet's 64 characters fill the array, so it holds no null */
/* NOLINTBEGIN(clang-diagnostic-unterminated-string-initialization) */
static char base64_alphabet[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/* NOLINTEND(clang-diagnostic-unterminated-string-initialization) */

/* each page is an HTML form that posts its hidden fields: the mail
 * address, the console's id and a record. no page ends with a null.
 *
 * chao_page posts a Chao to chao.cgi */
static char chao_page[1425] = {
#include "save/mail/chao_page.inc"
};

/* player_page posts the save to score.cgi. its form asks which record the
 * player enters: a time, a score, a fish's weight or another. it asks the
 * record's stage or minigame and character too */
static char player_page[3334] = {
#include "save/mail/player_page.inc"
};

/* cart_page posts a race to cart.cgi */
static char cart_page[1272] = {
#include "save/mail/cart_page.inc"
};

/* mission_page posts a mission to minigame.cgi */
static char mission_page[1247] = {
#include "save/mail/mission_page.inc"
};

static int chao_page_size = sizeof(chao_page);
static int player_page_size = sizeof(player_page);
static int cart_page_size = sizeof(cart_page);
static int mission_page_size = sizeof(mission_page);

/* where the next byte of the page being written goes */
static char *mail_cursor;

Bool save_isp_found(void)
{
    struct isp_flash flash;

    if (isp_read(&flash)) {
        if (flash.first.magic == ISP_MAGIC || flash.second.magic == ISP_MAGIC) {
            return TRUE;
        }
    }
    return FALSE;
}

/* read the settings' packets into *flash. it returns TRUE, or FALSE once
 * a flash call fails, with *flash partly read */
static Bool isp_read(struct isp_flash *flash)
{
    char *buffer;
    void *work;
    int packet;
    int result;

    /* @hack spelling: flash through a byte pointer of its own, which has SHC
     * work out ISP_LAST_PACKET as ISP_FIRST_PACKET + 11 (ADD #11,R10 at
     * 0x8c1195ea); through flash, it pools ISP_LAST_PACKET as a word of its
     * own and 35 of 71 words differ */
    buffer = (char *)flash;
    work = heap_alloc(ISP_WORK_SIZE);
    if (fmMngInit(ISP_PARTITION, work) != 0) {
        heap_free(work);
        return FALSE;
    }
    packet = ISP_FIRST_PACKET;
    /* @hack spelling: a for (;;) whose break tests the packet at the bottom,
     * which loads fmMngFinish's address into R3 (0x8c119666); as a plain
     * for, into R2, and 2 of 71 words differ */
    for (;;) {
        result = fmMngRead(
            packet, buffer + ((packet - ISP_FIRST_PACKET) * FMD_DATA_BYTES));
        if (result != 0) {
            heap_free(work);
            return FALSE;
        }
        for (;;) {
            result = fmMngExecServer();
            if (result == FMD_STAT_COMPLETE) {
                break;
            }
            if (result == FMD_STAT_ERR) {
                heap_free(work);
                return FALSE;
            }
        }
        packet++;
        if (packet > ISP_LAST_PACKET) {
            break;
        }
    }
    fmMngFinish();
    heap_free(work);
    return TRUE;
}

static Bool is_digit(int ch)
{
    if (ch >= '0' && ch <= '9') {
        return TRUE;
    }
    return FALSE;
}

static Bool is_alpha(int ch)
{
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
        return TRUE;
    }
    return FALSE;
}

Bool mail_is_alnum(int ch)
{
    if (!is_alpha(ch) && !is_digit(ch)) {
        return FALSE;
    }
    return TRUE;
}

/* write the size bytes at from as base64 at text, with no null after, and
 * return the text's length */
static int base64_encode(char *text, const Uint8 *from, int size)
{
    char *to;
    Uint8 rest[3];
    int i;
    int j;

    if (size == 0) {
        return 0;
    }
    to = text;
    for (i = 0; i < size - 3; i += 3) {
        *to++ = base64_alphabet[from[0] >> 2];
        *to++ = base64_alphabet[((from[0] & 3) << 4) | (from[1] >> 4)];
        *to++ = base64_alphabet[((from[1] & 15) << 2) | (from[2] >> 6)];
        *to++ = base64_alphabet[from[2] & 63];
        from += 3;
    }
    rest[0] = rest[1] = rest[2] = 0;
    for (j = i; j < size; j++) {
        rest[j - i] = *from++;
    }
    *to++ = base64_alphabet[rest[0] >> 2];
    *to++ = base64_alphabet[((rest[0] & 3) << 4) | (rest[1] >> 4)];
    *to++ = base64_alphabet[((rest[1] & 15) << 2) | (rest[2] >> 6)];
    *to++ = base64_alphabet[rest[2] & 63];
    switch (size - i) {
    case 1:
        to[-1] = to[-2] = '=';
        break;
    case 2:
        to[-1] = '=';
        break;
    }
    return to - text;
}

/* put the mail address of the settings in use at mail_cursor. it returns
 * TRUE, or FALSE without settings or if a flash call fails */
static Bool mail_put_address(void)
{
    struct isp_flash flash;
    char *from;
    int i;
    Bool end = FALSE;

    if (save_isp_found() && isp_read(&flash)) {
        if (flash.chosen != 1) {
            from = flash.first.mail;
        } else {
            from = flash.second.mail;
        }
        for (i = 0; i < ISP_MAIL_SIZE; i++) {
            /* @hack redundant: an empty else, which keeps the copy's cursor in
             * R3 and its byte in R2 (0x8c1198bc to 0x8c1198c8, and again from
             * 0x8c1198ec); without it, the two trade and 14 of 84 words
             * differ */
            if (*from == 0) {
                end = TRUE;
            } else {
            }
            if (!end) {
                *mail_cursor++ = *from++;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* put the console's id in base64 at mail_cursor, then its area */
static Bool mail_put_console_id(void)
{
    /* flash goes unused, but the image's frame keeps its 720 bytes */
    struct isp_flash flash;
    Uint8 id[8];
    char area;
    struct console_record *console;
    int length;

    kbCfgIni(0);
    console = kbCfgGetId();
    memcpy(id, console->id, sizeof(id));
    area = console->area;
    length = base64_encode(mail_cursor, id, sizeof(id));
    mail_cursor += length;
    *mail_cursor++ = area;
    return TRUE;
}

/* put the Chao's record at mail_cursor in base64: the head and the Chao,
 * scrambled once the crc is set. mail_put_save, mail_put_mission and
 * mail_put_race do the same with their records. each returns TRUE */
static Bool mail_put_chao(struct chao *chao)
{
    struct chao_mail mail;
    Uint16 crc;
    int length;

    mail.chao = *chao;
    mail.head.magic = CHAO_MAGIC;
    history_read(mail.head.times, mail.head.starts);
    crc = mail_crc(sizeof(mail) - sizeof(mail.head.crc),
                   (Uint8 *)&mail.head.magic);
    mail.head.crc = crc;
    mail_scramble((Uint32 *)&mail, sizeof(mail));
    length = base64_encode(mail_cursor, (Uint8 *)&mail, sizeof(mail));
    mail_cursor += length;
    return TRUE;
}

static Bool mail_put_save(void)
{
    struct save_mail mail;
    Uint16 crc;
    int length;

    mail.save = saves[save_slot];
    mail.head.magic = SAVE_MAGIC;
    history_read(mail.head.times, mail.head.starts);
    crc = mail_crc(sizeof(mail) - sizeof(mail.head.crc),
                   (Uint8 *)&mail.head.magic);
    mail.head.crc = crc;
    mail_scramble((Uint32 *)&mail, sizeof(mail));
    length = base64_encode(mail_cursor, (Uint8 *)&mail, sizeof(mail));
    mail_cursor += length;
    return TRUE;
}

static Bool mail_put_mission(int mission, int time, int character)
{
    struct values_mail mail;
    Uint16 crc;
    int length;

    mail.values[0] = mission;
    mail.values[1] = time;
    mail.values[2] = character;
    mail.head.magic = MISSION_MAGIC;
    history_read(mail.head.times, mail.head.starts);
    crc = mail_crc(sizeof(mail) - sizeof(mail.head.crc),
                   (Uint8 *)&mail.head.magic);
    mail.head.crc = crc;
    mail_scramble((Uint32 *)&mail, sizeof(mail));
    length = base64_encode(mail_cursor, (Uint8 *)&mail, sizeof(mail));
    mail_cursor += length;
    return TRUE;
}

/* put a race's record at mail_cursor, as mail_put_chao puts the Chao's.
 * the record holds the act, the time and the character, from race and
 * frames as save_write_cart_mail takes them */
static Bool mail_put_race(int race, Uint8 frames)
{
    Uint8 act;
    Uint8 character;
    /* @hack redundant: register on minutes, which keeps it in R6 (SHLR16 R6
     * at 0x8c119b60) and leaves the frame's byte at @R15 to character
     * (0x8c119b5a); without it, minutes takes that byte and 56 of 67 words
     * differ */
    register Uint8 minutes;
    Uint8 seconds;
    struct values_mail mail;
    int time;
    Uint16 crc;
    int length;

    act = race;
    character = race >> 8;
    minutes = race >> 16;
    seconds = race >> 24;
    /* @bug logic: minutes and seconds count sixtieths of a second
     * here, but frames comes from a cart time, which holds hundredths (ADD
     * R5,R4 at 0x8c119b78). the mail's time mixes the two */
    time = (seconds * 60) + (minutes * 3600) + frames;
    mail.values[0] = act;
    mail.values[1] = time;
    mail.values[2] = character;
    mail.head.magic = CART_MAGIC;
    history_read(mail.head.times, mail.head.starts);
    crc = mail_crc(sizeof(mail) - sizeof(mail.head.crc),
                   (Uint8 *)&mail.head.magic);
    mail.head.crc = crc;
    mail_scramble((Uint32 *)&mail, sizeof(mail));
    length = base64_encode(mail_cursor, (Uint8 *)&mail, sizeof(mail));
    mail_cursor += length;
    return TRUE;
}

/* the CRC-16-CCITT of size bytes at data, from 0xffff and returned
 * inverted */
static Uint16 mail_crc(int size, const Uint8 *data)
{
    Uint16 crc = 0xffff;
    int i;
    int bit;

    for (i = 0; i < size; i++) {
        crc ^= data[i] << 8;
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC_POLYNOMIAL;
            } else {
                crc <<= 1;
            }
        }
    }
    return ~crc;
}

/* xor each of the size / 4 words at data with SCRAMBLE_MASK and with a
 * key. the key starts at 0 and grows by SCRAMBLE_STEP a word */
/* @hack type: the mail record read and written as Uint32 words, a MOV.L
 * each; through memcpy word copies, the function runs 36 bytes long and 24
 * of 24 words differ */
static void mail_scramble(Uint32 *data, int size)
{
    int i;
    Uint32 key = 0;

    for (i = 0; i < size / 4; i++) {
        *data ^= key ^ SCRAMBLE_MASK;
        /* @hack redundant: the key's step in an if (1) block, which keeps
         * SCRAMBLE_MASK in R0 and SCRAMBLE_STEP in R5 (0x8c119c80,
         * 0x8c119c82); without it, the two trade, their pool words swap and
         * 2 of 24 words differ */
        if (1) {
            key += SCRAMBLE_STEP;
        }
        data++;
    }
}

/* each writer finds where each field's value starts in its page. it then
 * puts PAGE_INDENT spaces and the page at WORK_AREA, with each field's
 * placeholder replaced by its value */
Bool save_write_chao_mail(int unused, struct chao *chao)
{
    char name[16] = "SONICADV_H00";
    int i;
    int mailid;
    int chaodata;
    int dcid;
    int size;
    char *at;
    char *page;

    /* @hack spelling: the three stores chained, which leaves a 24-byte frame
     * (ADD #-24,R15 at 0x8c119cf0); as three statements, the frame is 28
     * bytes and 197 of 219 words differ */
    mailid = dcid = chaodata = 0;
    at = page = chao_page;
    mail_cursor = (char *)WORK_AREA;
    for (i = 0; i < chao_page_size; i++) {
        if (mailid == 0 && at[0] == 'm' && at[1] == 'a' && at[2] == 'i' &&
            at[3] == 'l' && at[4] == 'i' && at[5] == 'd') {
            mailid = i + MAILID_VALUE;
        }
        if (dcid == 0 && at[0] == 'd' && at[1] == 'c' && at[2] == 'i' &&
            at[3] == 'd') {
            dcid = i + DCID_VALUE;
        }
        if (chaodata == 0 && at[0] == 'c' && at[1] == 'h' && at[2] == 'a' &&
            at[3] == 'o' && at[4] == 'd' && at[5] == 'a' && at[6] == 't' &&
            at[7] == 'a') {
            chaodata = i + CHAODATA_VALUE;
        }
        at++;
    }
    for (i = 0; i < PAGE_INDENT; i++) {
        *mail_cursor++ = ' ';
    }
    for (i = 0, at = page; i < mailid; i++) {
        *mail_cursor++ = *at++;
    }
    if (!mail_put_address()) {
        return FALSE;
    }
    for (i = mailid + MAILID_PLACEHOLDER; i < dcid; i++) {
        *mail_cursor++ = page[i];
    }
    if (!mail_put_console_id()) {
        return FALSE;
    }
    for (i = dcid + DCID_PLACEHOLDER; i < chaodata; i++) {
        *mail_cursor++ = page[i];
    }
    if (!mail_put_chao(chao)) {
        return FALSE;
    }
    /* @hack spelling: i = i + 1, which keeps the search's CHAODATA_VALUE in
     * R6 and MAILID_VALUE in R7 (0x8c119d10, 0x8c119d2c); as i++, the two
     * trade and 6 of 219 words differ */
    for (i = chaodata + CHAODATA_PLACEHOLDER; i < chao_page_size; i = i + 1) {
        *mail_cursor++ = page[i];
    }
    size = mail_cursor - (char *)WORK_AREA;
    save_write_file(name, size, SAVE_FILE_CHAO);
    sound_play(SE_RING, 0, 0, 0);
    return TRUE;
}

Bool save_write_player_mail(int unused)
{
    char name[16] = "SONICADV_H01";
    int i;
    int mailid;
    int chaodata;
    int dcid;
    int size;
    char *at;
    char *page;

    /* @hack spelling: the three stores chained, which leaves a 20-byte frame
     * (ADD #-20,R15 at 0x8c119ed0); as three statements, the frame is 24
     * bytes and 196 of 218 words differ */
    mailid = dcid = chaodata = 0;
    at = page = player_page;
    mail_cursor = (char *)WORK_AREA;
    for (i = 0; i < player_page_size; i++) {
        if (mailid == 0 && at[0] == 'm' && at[1] == 'a' && at[2] == 'i' &&
            at[3] == 'l' && at[4] == 'i' && at[5] == 'd') {
            mailid = i + MAILID_VALUE;
        }
        if (dcid == 0 && at[0] == 'd' && at[1] == 'c' && at[2] == 'i' &&
            at[3] == 'd') {
            dcid = i + DCID_VALUE;
        }
        if (chaodata == 0 && at[0] == 'c' && at[1] == 'h' && at[2] == 'a' &&
            at[3] == 'o' && at[4] == 'd' && at[5] == 'a' && at[6] == 't' &&
            at[7] == 'a') {
            chaodata = i + CHAODATA_VALUE;
        }
        at++;
    }
    for (i = 0; i < PAGE_INDENT; i++) {
        *mail_cursor++ = ' ';
    }
    for (i = 0, at = page; i < mailid; i++) {
        *mail_cursor++ = *at++;
    }
    if (!mail_put_address()) {
        return FALSE;
    }
    for (i = mailid + MAILID_PLACEHOLDER; i < dcid; i++) {
        *mail_cursor++ = page[i];
    }
    if (!mail_put_console_id()) {
        return FALSE;
    }
    for (i = dcid + DCID_PLACEHOLDER; i < chaodata; i++) {
        *mail_cursor++ = page[i];
    }
    if (!mail_put_save()) {
        return FALSE;
    }
    for (i = chaodata + CHAODATA_PLACEHOLDER; i < player_page_size; i++) {
        *mail_cursor++ = page[i];
    }
    size = mail_cursor - (char *)WORK_AREA;
    save_write_file(name, size, SAVE_FILE_RECORD);
    sound_play(SE_RING, 0, 0, 0);
    return TRUE;
}

Bool save_write_cart_mail(int unused, int race, int frames)
{
    char name[16] = "SONICADV_H02";
    int i;
    int mailid;
    int chaodata;
    int dcid;
    int size;
    char *at;
    char *page;

    /* @hack spelling: the three stores chained, which leaves a 24-byte frame
     * (ADD #-24,R15 at 0x8c11a0b0); as three statements, the frame is 28
     * bytes and 200 of 219 words differ */
    mailid = dcid = chaodata = 0;
    at = page = cart_page;
    mail_cursor = (char *)WORK_AREA;
    /* the search stops at mission_page_size, short of the cart page's end
     * but past its fields */
    for (i = 0; i < mission_page_size; i++) {
        if (mailid == 0 && at[0] == 'm' && at[1] == 'a' && at[2] == 'i' &&
            at[3] == 'l' && at[4] == 'i' && at[5] == 'd') {
            mailid = i + MAILID_VALUE;
        }
        if (dcid == 0 && at[0] == 'd' && at[1] == 'c' && at[2] == 'i' &&
            at[3] == 'd') {
            dcid = i + DCID_VALUE;
        }
        if (chaodata == 0 && at[0] == 'c' && at[1] == 'h' && at[2] == 'a' &&
            at[3] == 'o' && at[4] == 'd' && at[5] == 'a' && at[6] == 't' &&
            at[7] == 'a') {
            chaodata = i + CHAODATA_VALUE;
        }
        at++;
    }
    for (i = 0; i < PAGE_INDENT; i++) {
        *mail_cursor++ = ' ';
    }
    for (i = 0, at = page; i < mailid; i++) {
        *mail_cursor++ = *at++;
    }
    if (!mail_put_address()) {
        return FALSE;
    }
    /* @hack spelling: the copy reading the page through a pointer, which stores
     * race at @(4,R15) and frames at @R15 (0x8c11a0be, 0x8c11a0c2); indexed
     * as the other writers' copies are, the two trade slots and 4 of 219
     * words differ */
    for (at = (i = mailid + MAILID_PLACEHOLDER) + page; i < dcid; i++) {
        *mail_cursor++ = *at++;
    }
    if (!mail_put_console_id()) {
        return FALSE;
    }
    /* @hack spelling: the copy through a pointer, which keeps race and frames
     * in their slots as the copy above does, and CHAODATA_VALUE in R6 and
     * MAILID_VALUE in R7 (0x8c11a0c8, 0x8c11a0ea); indexed, those trade too
     * and 11 of 219 words differ */
    for (at = (i = dcid + DCID_PLACEHOLDER) + page; i < chaodata; i++) {
        *mail_cursor++ = *at++;
    }
    if (!mail_put_race(race, frames)) {
        return FALSE;
    }
    /* @hack spelling: the copy through a pointer, which keeps race and frames
     * in their slots as the first copy does (0x8c11a0be, 0x8c11a0c2);
     * indexed, 4 of 219 words differ */
    for (at = (i = chaodata + CHAODATA_PLACEHOLDER) + page; i < cart_page_size;
         i++) {
        *mail_cursor++ = *at++;
    }
    size = mail_cursor - (char *)WORK_AREA;
    save_write_file(name, size, SAVE_FILE_CART);
    sound_play(SE_RING, 0, 0, 0);
    return TRUE;
}

Bool save_write_mission_mail(int unused, int mission, int time)
{
    char name[16] = "SONICADV_H03";
    int i;
    int mailid;
    int chaodata;
    int dcid;
    int size;
    char *at;
    char *page;

    /* @hack spelling: the three stores chained, which leaves a 28-byte frame
     * (ADD #-28,R15 at 0x8c11a2b0); as three statements, the frame is 32
     * bytes and 201 of 225 words differ */
    mailid = dcid = chaodata = 0;
    at = page = mission_page;
    mail_cursor = (char *)WORK_AREA;
    for (i = 0; i < mission_page_size; i++) {
        if (mailid == 0 && at[0] == 'm' && at[1] == 'a' && at[2] == 'i' &&
            at[3] == 'l' && at[4] == 'i' && at[5] == 'd') {
            mailid = i + MAILID_VALUE;
        }
        if (dcid == 0 && at[0] == 'd' && at[1] == 'c' && at[2] == 'i' &&
            at[3] == 'd') {
            dcid = i + DCID_VALUE;
        }
        if (chaodata == 0 && at[0] == 'c' && at[1] == 'h' && at[2] == 'a' &&
            at[3] == 'o' && at[4] == 'd' && at[5] == 'a' && at[6] == 't' &&
            at[7] == 'a') {
            chaodata = i + CHAODATA_VALUE;
        }
        at++;
    }
    for (i = 0; i < PAGE_INDENT; i++) {
        *mail_cursor++ = ' ';
    }
    /* @hack spelling: i = i + 1, which keeps DCID_VALUE in R7, CHAODATA_VALUE
     * in R6 and MAILID_VALUE in R5 (0x8c11a2c2, 0x8c11a2cc, 0x8c11a2ee); as
     * i++, the three rotate and 16 of 225 words differ */
    for (i = 0, at = page; i < mailid; i = i + 1) {
        *mail_cursor++ = *at++;
    }
    if (!mail_put_address()) {
        return FALSE;
    }
    for (i = mailid + MAILID_PLACEHOLDER; i < dcid; i++) {
        *mail_cursor++ = page[i];
    }
    if (!mail_put_console_id()) {
        return FALSE;
    }
    for (i = dcid + DCID_PLACEHOLDER; i < chaodata; i++) {
        *mail_cursor++ = page[i];
    }
    if (!mail_put_mission(mission, time, get_character())) {
        return FALSE;
    }
    /* @hack spelling: i = i + 1, which keeps mailid in R13 and chaodata in
     * R12 (TST R13,R13 at 0x8c11a2f0, TST R12,R12 at 0x8c11a338); as i++,
     * the two trade and 34 of 225 words differ */
    for (i = chaodata + CHAODATA_PLACEHOLDER; i < mission_page_size;
         i = i + 1) {
        *mail_cursor++ = page[i];
    }
    size = mail_cursor - (char *)WORK_AREA;
    save_write_file(name, size, SAVE_FILE_EVENT);
    sound_play(SE_RING, 0, 0, 0);
    return TRUE;
}
