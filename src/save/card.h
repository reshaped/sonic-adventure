/* the memory card: its drives, the saves, the mail and the downloads */
#ifndef SAVE_CARD_H
#define SAVE_CARD_H
#include <sg_xpt.h>
#include <shinobi.h>

struct save;

/* the backup library's drives: two sockets on each of the four ports */
#define CARD_DRIVES 8

/* a drive as card_complete and card_progress record it */
struct card_drive {
    Uint16 mounted; /* 1 from a card's mount to its unmount */
    Uint16 info_read; /* 1 once a mount has read info, until the unmount */
    Sint32 error; /* the last command's status, or 0 once a card mounts */
    Sint32 count; /* the blocks the running command has done */
    Sint32 max; /* the blocks the running command does in all */
    Uint32 op; /* the running command, once it reports progress, or 0 */
    BUS_DISKINFO info;
};

/* the kinds of file save_write_file writes, each with its own comments and
 * icon */
enum save_file_kind {
    SAVE_FILE_NET,
    SAVE_FILE_CHAO,
    SAVE_FILE_RECORD,
    SAVE_FILE_CART,
    SAVE_FILE_EVENT,
    SAVE_FILE_KINDS
};

/* the save the game plays, as an index into saves */
extern int save_slot;
/* the drive, below CARD_DRIVES, of the card the game saves to */
extern int save_drive;
/* TRUE while the game plays without a memory card, as it starts out. the
 * functions below call this saving disabled */
extern Bool save_disabled;
/* nonzero while a file write runs and for three frames after. it says
 * nothing of whether the write succeeded */
extern int card_saving;

/* the writes of the saves are save_reset_all_write to save_update_write
 * below. each raises the missing-card notice, MEMORY CARD NOT FOUND, when it
 * finds no card mounted in save_drive. the notice holds the game until B or
 * X is held or a card is mounted. B or X gives the write up, and a mounted
 * card takes it */

/* start the backup library on save_drive alone, with work for a 128 KB
 * card, and clear the drives' records, card_saving and the notice */
void card_init(void);
/* wait while save_drive is busy, then keep the missing-card notice off
 * until card_init */
void card_finish(void);
/* -1 while the missing-card notice holds the game, otherwise 0 */
int card_notice_busy(void);
/* run once a frame. it steps the missing-card notice while that is up, and
 * otherwise shows NOW SAVING and sets card_saving while a file write runs */
void card_frame_update(void);
/* the backup library's callbacks, which keep a struct card_drive for each
 * drive and return BUD_CBRET_OK. card_complete records a card's mount or
 * unmount, or a command's status. card_progress records the running
 * command and its count */
Sint32 card_complete(Sint32 drive, Sint32 op, Sint32 status, Uint32 param);
Sint32 card_progress(Sint32 drive, Sint32 op, Sint32 count, Sint32 max);
/* the free blocks of save_drive's card. it ignores buGetDiskInfo's
 * result, so while the drive is busy, or holds no formatted card, it
 * returns uninitialized stack bytes */
int card_free_blocks(void);

/* the highest number of a download on drive: a file named SONICADV_ and
 * its number in three hex digits. it returns -1 with saving disabled or
 * with none found. it reads names as card_find does, so a failed first
 * find leaves it testing uninitialized bytes */
int card_newest_download(int drive);
/* TRUE if save_drive's card holds the saves' file, SONICADV_SYS, and FALSE
 * without it or with saving disabled. it finds the file as card_find does,
 * so on an empty card it compares uninitialized bytes with the name */
Bool save_file_exists(void);
/* reset every save, or the one the game plays, then write the saves as
 * save_write does, with its results. the reset happens even with saving
 * disabled */
int save_reset_all_write(void);
int save_reset_write(void);
/* store the progress in the save the game plays, wait while save_drive is
 * busy, and start writing the saves to the card. the store happens even
 * with saving disabled.
 *
 * each returns 0 once the card takes the write. it returns -1 with saving
 * disabled, or if the file's header or the write request fails. with no
 * card mounted, each raises the missing-card notice instead: save_write and
 * the reset writes then return 4, and the others -1. once a write starts,
 * the _wait functions wait until the drive is idle, without checking the
 * write's result. the _update functions store only the play time,
 * character and stage, as save_update does */
int save_write(void);
int save_write_wait(void);
int save_update_write_wait(void);
int save_update_write(void);
/* read the saves from save_drive's card into saves, and load the one the
 * game plays into the progress. it returns 0, or -3 with saving disabled,
 * -1 without the file, or -2 if a save fails its checksum. on failure,
 * saves and the progress stay as they were. the load's own result goes
 * unchecked */
int save_read(void);
/* load the saves' file into card.c's file buffer, and return the saves in
 * it, unchecked: NULL without the file or with saving disabled. they last
 * until another call uses the buffer: a save's or a light file's read or
 * write, or card_read_xzxz */
struct save *save_peek(void);
/* 0 if save_drive's card holds a file named name, -1 with saving disabled,
 * and otherwise the last strcmp result. each find's name is compared
 * whether or not that find succeeds. a failed find writes no name, so if
 * the first fails, as on an empty card, the compare reads uninitialized
 * bytes */
int card_find(char *name);
/* write size bytes from WORK_AREA to save_drive's card as the file name,
 * with kind's comments and icon. kind must be below SAVE_FILE_KINDS. the
 * file's image is built 1 MB into WORK_AREA.
 *
 * it returns 0 once the card takes the write, or -1 with saving disabled
 * or if the file's header or the write request fails. for every kind but
 * SAVE_FILE_EVENT it then waits until the drive is idle, without checking
 * the write's result. an event's image must stay as it is until its write
 * ends */
int save_write_file(char *name, int size, int kind);
/* load the file name from save_drive's card 1 MB into WORK_AREA, and copy
 * its data, past a header with one icon frame, to WORK_AREA. it returns
 * the bytes copied, the file's whole blocks less the header, or -1 with
 * saving disabled or without the file. the load's result goes unchecked */
int save_read_file(char *name);
/* write WORK_AREA, which must hold a whole file image, to save_drive's
 * card as the file name, then wait until the drive is idle. size is in
 * bytes, rounded up to whole blocks, which must all be readable. it
 * returns 0 once the card takes the write, or -1 with saving disabled or
 * if the request fails. the write's result goes unchecked */
int card_write_work(char *name, int size);
/* load the first block of the file XZXZXZXZ into card.c's file buffer,
 * and return 0. it loads whether or not a find sees the file, even with
 * saving disabled. the name's array holds no null, so the find and the
 * load read past it */
int card_read_xzxz(void);

#endif
