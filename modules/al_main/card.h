/* the memory cards, as AL_MAIN.PRS keeps them: a record of each drive and
 * its card's Chao and egg, and the a-life file. AL_MAIN.PRS holds all of
 * it, so a caller outside it calls in only while it is loaded */
#ifndef AL_MAIN_CARD_H
#define AL_MAIN_CARD_H
#include <sg_xpt.h>

#include "chao/chao.h"
#include "save/card.h"

/* a drive and the card in it. chao and egg hold the Chao and the egg the
 * card's executable file keeps, each of type CHAO_NOTHING when the card
 * has none */
struct card_menu_drive {
    /* present as it was before card_drives_refresh or the library's completion
     * callback last wrote it. card_drives_take_changes sets it to present as it
     * takes in the change. a card is new to the drive while present is set and
     * was_present clear, and gone from it while was_present is set and
     * present clear */
    Sint32 was_present;
    /* 1 while a card is in the drive, as the library last said.
     * card_drives_refresh sets it to 1 where buIsReady holds, and to 0
     * elsewhere. a mount that ends without error sets it, and an unmount clears
     * it */
    Sint32 present;
    /* card_drives_start zeroes it, and no code reads it */
    Sint32 unk_08;
    /* the status the last command but a mount or an unmount ended with. a
     * mount without error zeroes it */
    Sint32 error;
    /* of the command that last reported its progress, the blocks it had
     * done and the blocks it does in all. an unmount zeroes both */
    Sint32 count;
    Sint32 max;
    /* that command, a BUD_OP_ value. the end of any command zeroes it */
    Sint32 op;
    /* 1 once card_drives_take_changes finds a new card's executable file with
     * the mark, and 0 for a card without one and for a card gone */
    Sint32 exec_marked;
    /* 1 when the last card_drives_take_changes to reach the drive found a card
     * new to it */
    Sint32 arrived;
    struct chao chao;
    struct chao egg;
    /* card_drives_start zeroes it, and no code reads it */
    Float unk_424;
    Uint8 unk_428[0xc];
};

extern struct card_menu_drive card_menu_drives[CARD_DRIVES];

/* restart the backup library on all eight drives, with work for 1 MB
 * cards and callbacks that keep these records. both are in AL_MAIN.PRS,
 * which must stay loaded until card_init restarts the library. then zero
 * each record's was_present, present, unk_08 and error, and its
 * exec_marked and unk_424. empty both its slots, and refresh present and
 * was_present with card_drives_refresh. returns 1, whatever buInit gives */
int card_drives_start(void);
/* hand the backup library back to 1ST_READ.BIN: card_init restarts it on
 * save_drive alone, with 1ST_READ.BIN's work and callbacks */
void card_drives_end(void);
/* save_drive, the drive alife_load and alife_save use */
int card_get_save_drive(void);
/* copy each drive's present into was_present, then set present to 1 where
 * buIsReady holds, and to 0 elsewhere */
void card_drives_refresh(void);
/* poll buStat(save_drive) until a status ends the wait as a failure, and
 * return 0. each ready status reads save_drive again and polls on, so it
 * never returns while the drive is ready or busy. no code calls it */
int fun_0c90288c(void);

/* read the a-life file, SONICADV_ALF, from save_drive's card, and wait on
 * the read. it then copies the file's data over chao_get_file_data's bytes
 * and chao_get_records's first 24 records, and returns 1. it returns 0,
 * copying nothing, when buIsExistFile or buLoadFileEx fails or the wait
 * ends on a failure */
int alife_load(void);
/* queue the a-life file's write to save_drive's card with buSaveFile, and
 * return 1 once it is queued. the file holds chao_get_file_data's bytes and
 * chao_get_records's first 24 records, in an image in AL_MAIN.PRS. the
 * write reads the image while buStat(save_drive) is busy, so AL_MAIN.PRS
 * must stay loaded until the write ends. a new file needs buGetDiskFree to
 * give as many blocks as the image takes. it returns 0, writing nothing,
 * without that room, and 0 when buSaveFile fails */
int alife_save(void);
/* alife_save, then wait on buStat(save_drive) for the write to end. it
 * returns 1 once the write ends, and 0 where alife_save gives 0 or the wait
 * ends on a failure */
int alife_save_wait(void);

/* card_read_chao to fun_0c902760 read and write a card's executable file,
 * which keeps a Chao and an egg in a block each. the file's first block
 * holds the mark CHAO ACCEPT 0x1e0 bytes in. a halfword after the mark
 * gives a byte offset into the file, in the Chao's block, and the egg's
 * block follows the Chao's. each of the functions finds the file with
 * buFindExecFile. where it finds one, it reads the first block and waits
 * on the read. it refuses a file without the mark, and one whose wait ends
 * on a failure */

/* queue the read of the Chao in the executable file on drive's card into
 * chao, with buLoadFileEx, and return 1 whatever that gives. the read fills
 * chao while buStat(drive) is busy. it returns 0, leaving chao alone, when
 * buFindExecFile fails or the file is refused */
int card_read_chao(Sint32 drive, struct chao *chao);
/* card_read_chao for the egg's block, into egg. no code calls it */
int fun_0c9023ac(Sint32 drive, struct chao *egg);
/* queue the deletion of the executable file on drive's card with
 * buDeleteFile, and return 1 when that gives BUD_ERR_OK. it returns 0,
 * deleting nothing, when buFindExecFile fails or the file is refused, and 0
 * when buDeleteFile fails */
int card_delete_exec(Sint32 drive);
/* queue the write of chao as the Chao of drive's card. a card with an
 * executable file takes chao in its Chao's block, through
 * buRewriteExecFile, which reads chao while buStat(drive) is busy.
 *
 * a card without one, with room for 128 blocks, takes the executable in
 * SET_AREA_1 as SONICADV__VM, through buSaveExecFile. SET_AREA_1 must then
 * hold samp_lcd.PRS's executable, as the gardens load it. chao is copied
 * into that executable 0x3000 bytes in. the new file's egg is the one
 * SET_AREA_1 holds: samp_lcd.PRS's, or the last egg card_write_egg wrote into
 * a new file.
 *
 * returns 1 once the write is queued, whatever it gives. returns 0, writing
 * nothing, when the file is refused or the room is short, and when
 * buFindExecFile gives another error */
int card_write_chao(Sint32 drive, const struct chao *chao);
/* card_write_chao with the placeholder record DUMMY, which it sets first. the
 * record's type is CHAO_NOTHING and its marks DAME. its skills from swim
 * to power are 160, 270, 380 and 490, and unk_17a is 0 */
int card_clear_chao(Sint32 drive);
/* card_write_chao for egg as drive's egg. a rewrite takes the block after the
 * Chao's. a new file holds egg after a Chao of type CHAO_NOTHING, whose
 * other 511 bytes are what the stack held */
int card_write_egg(Sint32 drive, const struct chao *egg);
/* card_clear_chao for drive's egg. a rewrite takes the block after the
 * Chao's, but a new file takes the record as its Chao */
int fun_0c902760(Sint32 drive);
/* take in each drive's change, in turn, clearing its arrived first.
 *
 * for a card new to the drive, the call sets was_present and arrived. if the
 * card has an executable file that isn't refused, exec_marked is set, and the
 * file's Chao and egg load into the slots with buLoadFileEx. a loaded slot
 * whose type card_admit_type rejects empties, and the egg slot also empties at
 * any type but CHAO_EGG and CHAO_PARTNER. a partner breeds with the card's
 * Chao: chao_breed's egg takes the egg slot, and buRewriteExecFile writes it to
 * the card. for a new card without an executable file, or with a refused one,
 * the call clears exec_marked and empties both slots.
 *
 * for a card gone from the drive, the call clears was_present and exec_marked,
 * and empties both slots. it also plays sound 22, the sound the put menu
 * closes with.
 *
 * the call waits on buStat after each load and write. when a wait ends on a
 * failure, the call returns at once and leaves the later drives as they were */
void card_drives_take_changes(void);

#endif
