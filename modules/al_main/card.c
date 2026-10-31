/* the memory cards, as AL_MAIN.PRS keeps them. it restarts the backup
 * library on every drive, with work and callbacks of its own, which keep a
 * record of each drive. it reads and writes the a-life file, and the Chao
 * and the egg a card's executable file keeps */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "al_main/card.h"
#include "al_main/vms.h"
#include "chao/chao.h"
#include "save/card.h"
#include "sound/sound.h"
#include "system/loader.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 30
#define TEXT_3_LENGTH 16
#include "al_main/card/text.inc"

/* a card's block, 512 bytes */
#define BLOCK_BITS 9
#define BLOCK (1 << BLOCK_BITS)

/* a backup file's header without a visual: 128 bytes, and a block for each
 * frame of its icon */
#define FILE_HEADER(icons) (128 + ((icons) * BLOCK))

/* the a-life file's icon frames, and the records its data holds after
 * chao_get_file_data's bytes */
#define ALIFE_ICONS 3
#define ALIFE_RECORDS 24
#define ALIFE_DATA_SIZE                                                        \
    (CHAO_FILE_DATA_SIZE + (ALIFE_RECORDS * sizeof(struct chao)))

/* the block the a-life file's data starts in, and its offset in that
 * block */
#define ALIFE_DATA_BLOCK (FILE_HEADER(ALIFE_ICONS) / BLOCK)
#define ALIFE_DATA_OFFSET (FILE_HEADER(ALIFE_ICONS) % BLOCK)

/* the executable file a card without one gets: SET_AREA_1's 128 blocks, as
 * the gardens load them from samp_lcd.PRS. block 24 holds its Chao, and
 * block 25 its egg */
#define EXEC_BLOCKS 128
#define EXEC_CHAO (SET_AREA_1 + (24 * BLOCK))
#define EXEC_EGG (SET_AREA_1 + (25 * BLOCK))

#define SAVE_FLAGS (BUD_FLAG_VERIFY | BUD_FLAG_COPYDISABLE)

/* the mark's bytes: CHAO ACCEPT and its NUL */
#define MARK_SIZE 12

/* the mark an executable file's first block holds 0x1e0 bytes in */
struct exec_mark {
    char text[MARK_SIZE];
    Uint16 chao_offset; /* a byte offset into the file, in the Chao's block */
};

/* the first block of a card's executable file */
struct exec_block {
    Uint8 unk_000[0x1e0];
    struct exec_mark mark;
    Uint8 unk_1ee[0x12];
};

/* the object's bss, in the image's order. no code reads or writes a dat_
 * item */
struct card_menu_drive card_menu_drives[CARD_DRIVES];
/* the drives ready at card_drives_refresh's last count. no other code reads it
 */
static Sint16 card_count;
static Uint8 dat_0ca865fa[0x200a];
/* the backup library's work, sized for a 1 MB card in each drive */
static Uint8 card_work[BUM_WORK_SIZE(BUD_CAPACITY_1MB, CARD_DRIVES)];
static Uint8 dat_0cb02704[4];
/* the first block of the executable file read_mark last read */
#ifdef GCC
/* buLoadFileEx reads the block in by vmsfs_memcpyl's longword stores, which
 * fault on an address off 4. GCC aligns this structure only to its Uint16 */
static struct exec_block exec_block __attribute__((aligned(4)));
#else
static struct exec_block exec_block;
#endif
static Uint8 dat_0cb02908[0x200];
/* the placeholder record, named DUMMY, that card_clear_chao and fun_0c902760
 * write */
static struct chao dummy_chao;
/* the chao_offset read_mark last found after the mark */
static Uint16 exec_chao_offset;
static Uint8 dat_0cb02d0a[6];
/* the a-life file's blocks from ALIFE_DATA_BLOCK on, as alife_load reads
 * them */
static Uint8 alife_blocks[ALIFE_DATA_OFFSET + ALIFE_DATA_SIZE];
static Uint8 dat_0cb05f10[0x1420];
/* the backup file image the saves build, and the data they build it of */
static Uint8 alife_image[0x4000];
static Uint8 alife_data[ALIFE_DATA_SIZE];
static Uint8 dat_0cb0e4b0[0x10008];

/* the a-life file's icon: 16 colors in ARGB4444, and three frames of 32 by
 * 32 4-bit pixels */
static Uint16 alife_palette[16] = {
#include "al_main/card/alife_palette.inc"
};

static Uint8 alife_icon[ALIFE_ICONS * BLOCK] = {
#include "al_main/card/alife_icon.inc"
};

static const char alife_file_name[] = "SONICADV_ALF";
static const char exec_file_name[] = "SONICADV__VM";

static void drive_init_done(void);

int card_drives_start(void)
{
    int i;

    buInit(BUD_CAPACITY_1MB, BUD_USE_DRIVE_ALL, card_work, drive_init_done);
    for (i = 0; i < CARD_DRIVES; i++) {
        struct card_menu_drive *drive = &card_menu_drives[i];

        drive->was_present = 0;
        drive->present = 0;
        drive->unk_08 = 0;
        drive->error = 0;
        drive->exec_marked = 0;
        drive->chao.type = CHAO_NOTHING;
        drive->egg.type = CHAO_NOTHING;
        drive->unk_424 = 0.0F;
    }
    card_drives_refresh();
    return 1;
}

void card_drives_end(void)
{
    card_init();
}

/* the library's progress callback, which records a command's progress in
 * the drive's record */
static Sint32 drive_progress(Sint32 drive, Sint32 op, Sint32 count, Sint32 max)
{
    struct card_menu_drive *record = &card_menu_drives[drive];

    record->count = count;
    record->max = max;
    record->op = op;
    return BUD_CBRET_OK;
}

/* the library's completion callback, which records a mount, an unmount or
 * another command's status in the drive's record */
static Sint32 drive_complete(Sint32 drive, Sint32 op, Sint32 status,
                             Uint32 param)
{
    struct card_menu_drive *record = &card_menu_drives[drive];
    /* @hack spelling: the 0 each clear stores, held in cleared, which keeps
     * the return's own MOV #0,R0 at 0x0c901ca0; with 0 at each store, the
     * return takes MOV R7,R0 and 1 of 33 words differ */
    Sint32 cleared = 0;

    switch (op) {
    case BUD_OP_MOUNT:
        if (status == BUD_ERR_OK) {
            record->was_present = record->present;
            record->present = 1;
            record->error = cleared;
        }
        break;
    case BUD_OP_UNMOUNT:
        record->was_present = record->present;
        record->present = cleared;
        record->count = cleared;
        record->max = cleared;
        record->op = cleared;
        break;
    default:
        record->error = status;
        break;
    }
    record->op = cleared;
    return BUD_CBRET_OK;
}

/* buInit's callback, once the library starts. the game's frame runs
 * buExecServer itself, so this takes the library's own handler off, then
 * sets the callbacks */
static void drive_init_done(void)
{
    buDisableServer();
    buSetCompleteCallback(drive_complete);
    buSetProgressCallback(drive_progress);
}

int card_get_save_drive(void)
{
    return save_drive;
}

/* 0 for a status that ends a wait on the drive as a failure, and 1 for any
 * other, BUD_STAT_BUSY among them. a wait stops at BUD_STAT_READY before it
 * asks */
static int keep_waiting(Sint32 status)
{
    switch (status) {
    case (Sint32)BUD_ERR_INVALID_PARAM:
    case (Sint32)BUD_ERR_NO_DISK:
    case (Sint32)BUD_ERR_UNFORMAT:
    case (Sint32)BUD_ERR_DISK_FULL:
    case (Sint32)BUD_ERR_FILE_NOT_FOUND:
    case (Sint32)BUD_ERR_CANNOT_OPEN:
    case (Sint32)BUD_ERR_ACCESS_DENIED:
    case (Sint32)BUD_ERR_VERIFY:
    case (Sint32)BUD_ERR_ILLEGAL_DISK:
    case (Sint32)BUD_ERR_UNKNOWN_DISK:
        return 0;
    }
    return 1;
}

int alife_load(void)
{
    int drive = card_get_save_drive();
    struct chao *records = chao_get_records();
    Uint8 *head = chao_get_file_data();
    Sint32 status;

    status = buIsExistFile(drive, alife_file_name);
    /* @hack redundant: the else after the return, which loads the read's block
     * count, 25, into R2 and buLoadFileEx's address into R3 (0x0c901db4,
     * 0x0c901dba); without it, the two trade and 4 of 66 words differ */
    if (status != BUD_ERR_OK) {
        return 0;
    } else {
        status = buLoadFileEx(drive, alife_file_name, alife_blocks,
                              ALIFE_DATA_BLOCK, sizeof(alife_blocks) / BLOCK);
        /* @hack spelling: a switch of one case, which tests the status with
         * CMP/EQ #0,R0 and moves it to R4 in the branch's slot (0x0c901dc6,
         * 0x0c901dca); as an if, 3 of 66 words differ */
        switch (status) {
        case BUD_ERR_OK:
            for (;;) {
                status = buStat(drive);
                if (status == BUD_STAT_READY) {
                    break;
                }
                if (!keep_waiting(status)) {
                    return 0;
                }
            }
            njMemCopy(head, &alife_blocks[ALIFE_DATA_OFFSET],
                      CHAO_FILE_DATA_SIZE);
            njMemCopy(records,
                      &alife_blocks[ALIFE_DATA_OFFSET + CHAO_FILE_DATA_SIZE],
                      ALIFE_RECORDS * sizeof(struct chao));
            return 1;
        }
    }
    return 0;
}

static void card_get_time(BUS_TIME *time)
{
    SYS_RTC_DATE date;

    syRtcGetDate(&date);
    time->year = date.year;
    time->month = date.month;
    time->day = date.day;
    time->hour = date.hour;
    time->minute = date.minute;
    time->second = date.second;
    time->dayofweek = date.dayofweek;
}

int alife_save(void)
{
    int drive = card_get_save_drive();
    struct chao *records = chao_get_records();
    Uint8 *head = chao_get_file_data();
    BUS_TIME time;
    BUS_BACKUPFILEHEADER header;
    Sint32 blocks;
    Sint32 status;

    memset(alife_image, 0, sizeof(alife_image));
    memset(alife_data, 0, sizeof(alife_data));
    /* @bug memory: njMemCopy4 counts longs, so this copy of
     * chao_get_file_data's 0x180 bytes reads 0x600 (0x0c901ed8, 0x0c901eda).
     * the 0x480 bytes past them are chao_get_records's first. the next copy
     * writes the same bytes to the same place in alife_data */
#ifdef GCC
    /* repairs the count the tag names: GCC copies the 0x180 bytes as longs */
    njMemCopy4(alife_data, head, CHAO_FILE_DATA_SIZE / sizeof(Sint32));
#else
    njMemCopy4(alife_data, head, CHAO_FILE_DATA_SIZE);
#endif
    njMemCopy4(&alife_data[CHAO_FILE_DATA_SIZE], records,
               ALIFE_RECORDS * sizeof(struct chao) / sizeof(Sint32));
    strcpy(header.vms_comment, TEXT_1);
    strcpy(header.btr_comment, TEXT_2);
    /* @bug memory: TEXT_3 fills game_name's 16 bytes, so strcpy
     * writes its NUL into icon_palette's first byte (0x0c901f00). the store
     * to icon_palette after it writes that byte again (0x0c901f0c) */
    strcpy((char *)header.game_name, TEXT_3);
    header.icon_palette = alife_palette;
    header.icon_data = alife_icon;
    header.icon_num = ALIFE_ICONS;
    header.icon_speed = 6;
    header.visual_data = NULL;
    header.visual_type = BUD_VISUALTYPE_NONE;
    header.save_data = alife_data;
    header.save_size = sizeof(alife_data);
    card_get_time(&time);
    blocks = buMakeBackupFileImage(alife_image, &header);
    status = buIsExistFile(drive, alife_file_name);
    /* @hack spelling: the create reached by goto, not by an else, which gives
     * the first write's flags R2 and &time R3 (0x0c901f52, 0x0c901f5e); as
     * if and else, the two trade and 5 of 137 words differ */
    if (status != BUD_ERR_OK) {
        goto create;
    }
    status = buSaveFile(drive, alife_file_name, alife_image, blocks, &time,
                        SAVE_FLAGS);
    /* @hack spelling: a switch of one case, which tests the status with
     * CMP/EQ #0,R0 and moves it to R4 in the branch's slot (0x0c901f6a,
     * 0x0c901f6e); as an if, 12 of 137 words differ */
    switch (status) {
    case BUD_ERR_OK:
        return 1;
    }
    goto done;
create:
    if (buGetDiskFree(drive, BUD_FILETYPE_NORMAL) >= blocks) {
        status = buSaveFile(drive, alife_file_name, alife_image, blocks, &time,
                            SAVE_FLAGS);
        /* @hack spelling: a switch of one case, which tests the status with
         * CMP/EQ #0,R0 and moves it to R4 in the branch's slot (0x0c902004,
         * 0x0c902008); as an if, 3 of 137 words differ */
        switch (status) {
        case BUD_ERR_OK:
            return 1;
        }
    }
done:
    return 0;
}

int alife_save_wait(void)
{
    int drive = card_get_save_drive();
    struct chao *records = chao_get_records();
    Uint8 *head = chao_get_file_data();
    BUS_TIME time;
    BUS_BACKUPFILEHEADER header;
    Sint32 blocks;
    Sint32 status;

    memset(alife_image, 0, sizeof(alife_image));
    memset(alife_data, 0, sizeof(alife_data));
    /* @bug memory: as in alife_save, this copy reads 0x600 bytes for
     * chao_get_file_data's 0x180 (0x0c90207a, 0x0c90207c) */
#ifdef GCC
    /* repairs the count, as in alife_save */
    njMemCopy4(alife_data, head, CHAO_FILE_DATA_SIZE / sizeof(Sint32));
#else
    njMemCopy4(alife_data, head, CHAO_FILE_DATA_SIZE);
#endif
    njMemCopy4(&alife_data[CHAO_FILE_DATA_SIZE], records,
               ALIFE_RECORDS * sizeof(struct chao) / sizeof(Sint32));
    strcpy(header.vms_comment, TEXT_1);
    strcpy(header.btr_comment, TEXT_2);
    /* @bug memory: as in alife_save, strcpy writes TEXT_3's NUL into
     * icon_palette's first byte (0x0c9020a2), and the store after it writes
     * that byte again (0x0c9020ae) */
    strcpy((char *)header.game_name, TEXT_3);
    header.icon_palette = alife_palette;
    header.icon_data = alife_icon;
    header.icon_num = ALIFE_ICONS;
    header.icon_speed = 6;
    header.visual_data = NULL;
    header.visual_type = BUD_VISUALTYPE_NONE;
    header.save_data = alife_data;
    header.save_size = sizeof(alife_data);
    card_get_time(&time);
    blocks = buMakeBackupFileImage(alife_image, &header);
    status = buIsExistFile(drive, alife_file_name);
    /* @hack spelling: the create reached by goto, as in alife_save (0x0c9020f6,
     * 0x0c902102); as if and else, the two trade and 5 of 160 words
     * differ */
    if (status != BUD_ERR_OK) {
        goto create;
    }
    status = buSaveFile(drive, alife_file_name, alife_image, blocks, &time,
                        SAVE_FLAGS);
    /* @hack spelling: a switch of one case, as in alife_save (0x0c90210e,
     * 0x0c902112); as an if, 3 of 160 words differ */
    switch (status) {
    case BUD_ERR_OK:
        for (;;) {
            status = buStat(drive);
            if (status == BUD_STAT_READY) {
                break;
            }
            if (!keep_waiting(status)) {
                return 0;
            }
        }
        return 1;
    }
    goto done;
create:
    if (buGetDiskFree(drive, BUD_FILETYPE_NORMAL) >= blocks) {
        status = buSaveFile(drive, alife_file_name, alife_image, blocks, &time,
                            SAVE_FLAGS);
        /* @hack spelling: a switch of one case, as in alife_save (0x0c9021c4,
         * 0x0c9021c8); as an if, 3 of 160 words differ */
        switch (status) {
        case BUD_ERR_OK:
            for (;;) {
                status = buStat(drive);
                if (status == BUD_STAT_READY) {
                    break;
                }
                if (!keep_waiting(status)) {
                    return 0;
                }
            }
            return 1;
        }
    }
done:
    return 0;
}

/* read the first block of drive's executable file, name, into exec_block,
 * and wait on the read. buLoadFileEx's result goes unchecked. it returns 1
 * when the block has the mark, and keeps the offset after it in
 * exec_chao_offset. it returns 0 without the mark, or when the wait ends on
 * a failure */
static int read_mark(Sint32 drive, const char *name)
{
    char expected[MARK_SIZE] = "CHAO ACCEPT";
    char *cursor;
    Sint32 status;
    int i;

    buLoadFileEx(drive, name, &exec_block, 0, 1);
    for (;;) {
        status = buStat(drive);
        if (status == BUD_STAT_READY) {
            break;
        }
        if (!keep_waiting(status)) {
            return 0;
        }
    }
    /* @hack redundant: a call of buStat that never runs, which keeps
     * buStat's address in R14 and expected's in R12 (0x0c902248,
     * 0x0c90220a); without it, the two trade and 4 of 77 words differ */
    if (0) {
        buStat(0);
    }
    cursor = (char *)&exec_block.mark;
    for (i = 0; i < MARK_SIZE; i++, cursor++) {
        if (*cursor != expected[i]) {
            return 0;
        }
    }
    /* @hack spelling: the offset read through the cursor the compare leaves on
     * it, MOV.W @R5 at 0x0c902280; read as exec_block.mark.chao_offset, it
     * takes a pool load of its own, the function runs 6 bytes longer, and
     * 30 of 77 words differ */
    exec_chao_offset = *(Uint16 *)cursor;
    return 1;
}

int card_read_chao(Sint32 drive, struct chao *chao)
{
    char name[16];
    Sint32 block;
    Sint32 status;

    status = buFindExecFile(drive, name);
    if (status == (Sint32)BUD_ERR_FILE_NOT_FOUND) {
        return 0;
    }
    if (status == BUD_ERR_OK) {
        /* @hack redundant: the else after the return, which keeps the block in
         * R2 and passes it on with MOV R2,R7 at 0x0c902320; without it, the
         * block takes R3 and is read back from the frame, and 10 of 46 words
         * differ */
        if (!read_mark(drive, name)) {
            return 0;
        } else {
            block = exec_chao_offset >> BLOCK_BITS;
            buLoadFileEx(drive, name, chao, block, 1);
            return 1;
        }
    }
    return 0;
}

int card_delete_exec(Sint32 drive)
{
    char name[16];
    Sint32 status;

    status = buFindExecFile(drive, name);
    if (status == (Sint32)BUD_ERR_FILE_NOT_FOUND) {
        return 0;
    }
    if (status == BUD_ERR_OK) {
        /* @hack redundant: the else after the return, which loads
         * buDeleteFile's address into R2 (0x0c902364, 0x0c902368); without it,
         * R3 takes it and 2 of 35 words differ */
        if (!read_mark(drive, name)) {
            return 0;
        } else {
            status = buDeleteFile(drive, name);
            if (status == BUD_ERR_OK) {
                return 1;
            }
        }
    }
    return 0;
}

int fun_0c9023ac(Sint32 drive, struct chao *egg)
{
    char name[16];
    Sint32 block;
    Sint32 status;

    status = buFindExecFile(drive, name);
    if (status == (Sint32)BUD_ERR_FILE_NOT_FOUND) {
        return 0;
    }
    if (status == BUD_ERR_OK) {
        /* @hack redundant: the else after the return, which keeps the block in
         * R2 and passes it on with MOV R2,R7 at 0x0c9023ee; without it, the
         * block takes R3 and is read back from the frame, and 11 of 47 words
         * differ */
        if (!read_mark(drive, name)) {
            return 0;
        } else {
            block = (exec_chao_offset >> BLOCK_BITS) + 1;
            buLoadFileEx(drive, name, egg, block, 1);
            return 1;
        }
    }
    return 0;
}

int card_write_chao(Sint32 drive, const struct chao *chao)
{
    char name[16];
    BUS_TIME time;
    Sint32 block;
    Sint32 status;

    status = buFindExecFile(drive, name);
    if (status == (Sint32)BUD_ERR_FILE_NOT_FOUND) {
        if (buGetDiskFree(drive, BUD_FILETYPE_EXECUTABLE) >= EXEC_BLOCKS) {
            card_get_time(&time);
            njMemCopy(EXEC_CHAO, chao, sizeof(struct chao));
            buSaveExecFile(drive, exec_file_name, SET_AREA_1, EXEC_BLOCKS,
                           &time, SAVE_FLAGS);
            return 1;
        }
    } else if (status == BUD_ERR_OK) {
        if (read_mark(drive, name)) {
            block = exec_chao_offset >> BLOCK_BITS;
            buRewriteExecFile(drive, name, chao, block, 1);
            return 1;
        }
    }
    return 0;
}

int card_clear_chao(Sint32 drive)
{
    char name[16];
    BUS_TIME time;
    Sint32 block;
    Sint32 status;

    dummy_chao.type = CHAO_NOTHING;
    dummy_chao.skills.swim = 160;
    dummy_chao.skills.fly = 270;
    dummy_chao.skills.run = 380;
    dummy_chao.skills.power = 490;
    dummy_chao.mark_c = 'D';
    dummy_chao.mark_h = 'A';
    dummy_chao.mark_a = 'M';
    dummy_chao.mark_o = 'E';
    dummy_chao.unk_17a = 0;
    dummy_chao.name[0] = 'D';
    dummy_chao.name[1] = 'U';
    dummy_chao.name[2] = 'M';
    dummy_chao.name[3] = 'M';
    dummy_chao.name[4] = 'Y';
    dummy_chao.name[5] = '\0';
    status = buFindExecFile(drive, name);
    if (status == (Sint32)BUD_ERR_FILE_NOT_FOUND) {
        if (buGetDiskFree(drive, BUD_FILETYPE_EXECUTABLE) >= EXEC_BLOCKS) {
            card_get_time(&time);
            njMemCopy(EXEC_CHAO, &dummy_chao, sizeof(struct chao));
            buSaveExecFile(drive, exec_file_name, SET_AREA_1, EXEC_BLOCKS,
                           &time, SAVE_FLAGS);
            return 1;
        }
    } else if (status == BUD_ERR_OK) {
        if (read_mark(drive, name)) {
            block = exec_chao_offset >> BLOCK_BITS;
            buRewriteExecFile(drive, name, &dummy_chao, block, 1);
            return 1;
        }
    }
    return 0;
}

int card_write_egg(Sint32 drive, const struct chao *egg)
{
    char name[16];
    BUS_TIME time;
    struct chao empty;
    Sint32 block;
    Sint32 status;

    status = buFindExecFile(drive, name);
    if (status == (Sint32)BUD_ERR_FILE_NOT_FOUND) {
        if (buGetDiskFree(drive, BUD_FILETYPE_EXECUTABLE) >= EXEC_BLOCKS) {
#ifdef GCC
            /* repairs the unset bytes the tag below names: GCC's empty slot
             * holds 0 in every byte but its type's, so no stack bytes reach
             * the card */
            memset(&empty, 0, sizeof(empty));
#endif
            empty.type = CHAO_NOTHING;
            card_get_time(&time);
            /* @bug uninitialized: only empty's type is set (0x0c902680). the
             * copy reads all 512 of its bytes into the executable's Chao
             * block (0x0c90268e), and buSaveExecFile writes them to the
             * card */
            njMemCopy(EXEC_CHAO, &empty, sizeof(struct chao));
            njMemCopy(EXEC_EGG, egg, sizeof(struct chao));
            buSaveExecFile(drive, exec_file_name, SET_AREA_1, EXEC_BLOCKS,
                           &time, SAVE_FLAGS);
            return 1;
        }
    } else if (status == BUD_ERR_OK) {
        if (read_mark(drive, name)) {
            block = (exec_chao_offset >> BLOCK_BITS) + 1;
            buRewriteExecFile(drive, name, egg, block, 1);
            return 1;
        }
    }
    return 0;
}

int fun_0c902760(Sint32 drive)
{
    char name[16];
    BUS_TIME time;
    Sint32 block;
    Sint32 status;

    dummy_chao.type = CHAO_NOTHING;
    dummy_chao.skills.swim = 160;
    dummy_chao.skills.fly = 270;
    dummy_chao.skills.run = 380;
    dummy_chao.skills.power = 490;
    dummy_chao.mark_c = 'D';
    dummy_chao.mark_h = 'A';
    dummy_chao.mark_a = 'M';
    dummy_chao.mark_o = 'E';
    dummy_chao.unk_17a = 0;
    dummy_chao.name[0] = 'D';
    dummy_chao.name[1] = 'U';
    dummy_chao.name[2] = 'M';
    dummy_chao.name[3] = 'M';
    dummy_chao.name[4] = 'Y';
    dummy_chao.name[5] = '\0';
    status = buFindExecFile(drive, name);
    if (status == (Sint32)BUD_ERR_FILE_NOT_FOUND) {
        if (buGetDiskFree(drive, BUD_FILETYPE_EXECUTABLE) >= EXEC_BLOCKS) {
            card_get_time(&time);
            njMemCopy(EXEC_CHAO, &dummy_chao, sizeof(struct chao));
            buSaveExecFile(drive, exec_file_name, SET_AREA_1, EXEC_BLOCKS,
                           &time, SAVE_FLAGS);
            return 1;
        }
    } else if (status == BUD_ERR_OK) {
        if (read_mark(drive, name)) {
            block = exec_chao_offset >> BLOCK_BITS;
            buRewriteExecFile(drive, name, &dummy_chao, block + 1, 1);
            return 1;
        }
    }
    return 0;
}

int fun_0c90288c(void)
{
    /* @hack redundant: register on drive, which keeps it in R14 across the
     * calls (0x0c902894); as a plain local, the function runs 2 bytes longer
     * and 17 of 20 words differ */
    register int drive;
    /* @hack redundant: register on poll_status, which keeps buStat's address
     * in R13 (0x0c902896); as a plain local, the function runs 4 bytes
     * longer and 17 of 20 words differ */
    register Sint32 (*poll_status)(Sint32);
    int status;
    /* @hack spelling: save_drive read through selected_drive, which takes its
     * address in R4 (0x0c902892, 0x0c902894); read by name, R3 takes it and
     * 2 of 20 words differ */
    int *selected_drive;

    /* @hack spelling: the two loops written as labels and gotos, which SHC
     * doesn't take for loops: it loads buStat's address into R13 at each poll
     * (0x0c902896) and saves no R12. as a for around a for, it loads the
     * address into R12 once, before the loops, the function runs 6 bytes longer
     * and 18 of 20 words differ */
restart:
    selected_drive = &save_drive;
    drive = *selected_drive;
poll:
    /* @hack spelling: buStat called through poll_status, set before each call,
     * which loads its address into R13 at each poll (0x0c902896); called by
     * name, R3 takes it and R13 goes unsaved, the function runs 4 bytes
     * shorter, and 19 of 20 words differ */
    poll_status = buStat;
    status = poll_status(drive);
    if (status == BUD_STAT_READY) {
        goto restart;
    }
    if (keep_waiting(status)) {
        goto poll;
    }
    return 0;
}

void card_drives_refresh(void)
{
    int i;

    card_count = 0;
    for (i = 0; i < CARD_DRIVES; i++) {
        if (buIsReady(i)) {
            card_menu_drives[i].was_present = card_menu_drives[i].present;
            card_menu_drives[i].present = 1;
            card_count++;
        } else {
            card_menu_drives[i].was_present = card_menu_drives[i].present;
            card_menu_drives[i].present = 0;
        }
    }
}

void card_drives_take_changes(void)
{
    int i;
    char name[16];
    struct chao child;
    Sint32 status;
    Sint32 block;

    for (i = 0; i < CARD_DRIVES; i++) {
        struct card_menu_drive *drive = &card_menu_drives[i];

        drive->arrived = 0;
        if (!drive->was_present && drive->present) {
            drive->was_present = 1;
            drive->arrived = 1;
            status = buFindExecFile(i, name);
            /* @hack spelling: a switch of only a default, which gives the
             * loads' success a break past the failure's block; with a goto to
             * the test for a gone card in its place, the function runs 4 bytes
             * shorter and 201 of 212 words differ */
            switch (status) {
            default:
                /* @hack spelling: an empty arm for a failed find, ahead of the
                 * else, which loads the Chao block's offset into R2 and
                 * passes the block on with MOV R2,R7 (0x0c902ab0,
                 * 0x0c902abe); as if (status == BUD_ERR_OK) alone, the block
                 * takes R3 and is read back from the frame, and 11 of 212
                 * words differ */
                if (status) {
                } else {
                    drive->exec_marked = 1;
                    if (read_mark(i, name)) {
                        block = exec_chao_offset >> BLOCK_BITS;
                        buLoadFileEx(i, name, &drive->chao, block, 1);
                        for (;;) {
                            status = buStat(i);
                            if (status == BUD_STAT_READY) {
                                break;
                            }
                            if (!keep_waiting(status)) {
                                return;
                            }
                        }
                        if (!card_admit_type(&drive->chao.type)) {
                            drive->chao.type = CHAO_NOTHING;
                        }
                        buLoadFileEx(i, name, &drive->egg, block + 1, 1);
                        for (;;) {
                            status = buStat(i);
                            if (status == BUD_STAT_READY) {
                                break;
                            }
                            if (!keep_waiting(status)) {
                                return;
                            }
                        }
                        if (card_admit_type(&drive->egg.type)) {
                            if (drive->egg.type == CHAO_PARTNER) {
                                chao_breed(&drive->chao, &drive->egg, &child);
                                child.type = CHAO_EGG;
                                drive->egg = child;
                                buRewriteExecFile(i, name, &drive->egg,
                                                  block + 1, 1);
                                for (;;) {
                                    status = buStat(i);
                                    if (status == BUD_STAT_READY) {
                                        break;
                                    }
                                    if (!keep_waiting(status)) {
                                        return;
                                    }
                                }
                            } else if (drive->egg.type != CHAO_EGG) {
                                drive->egg.type = CHAO_NOTHING;
                                continue;
                            }
                        } else {
                            drive->egg.type = CHAO_NOTHING;
                            continue;
                        }
                        break;
                    }
                }
                drive->exec_marked = 0;
                drive->chao.type = CHAO_NOTHING;
                drive->egg.type = CHAO_NOTHING;
                continue;
            }
        }
        if (drive->was_present && !drive->present) {
            drive->was_present = 0;
            drive->exec_marked = 0;
            drive->chao.type = CHAO_NOTHING;
            drive->egg.type = CHAO_NOTHING;
            sound_play(SE_MENU_CLOSE, 0, 0, 0);
        }
    }
}
