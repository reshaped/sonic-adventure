/* VMS file system of the backup library, as far as the sources use it */
#ifndef VMSFS_H
#define VMSFS_H
#include <sg_xpt.h>
#include <shinobi.h>

/* drives: the two expansion sockets of each of the four controller ports */
#define VMSFS_DRIVES 8

/* a card's layout. vmsio_get_media_info fills it from the card's media and
 * device information, but leaves user_blocks as it was and sets exec_size to
 * 128 and last_byte to 511 on every card. each _size is in blocks */
struct vmsfs_media {
    Sint32 last_block; /* the card's last block's number, not a count */
    Sint32 partition;
    Sint32 system_block;
    Sint32 fat_block;
    Sint32 fat_size;
    Sint32 dir_block;
    Sint32 dir_size;
    Sint32 spare_block;
    Sint32 spare_size;
    Sint32 icon;
    Sint32 unk_28;
    Sint32 last_byte; /* a block's last byte's number, not a count */
    Sint32 user_blocks;
    Sint32 exec_block;
    Sint32 exec_size;
    Sint32 read_phases; /* the phases in which the card reads a block */
    Sint32 write_phases;
    Sint32 read_bytes; /* the bytes in each phase */
    Sint32 write_bytes;
};

/* a card's capacity, as vmsfs_context.capacity and vmsfs_drive.capacity hold
 * it, and as an index into buCapsFats, buCapsDirs and buProgMaxTbl */
enum { CARD_128KB, CARD_256KB, CARD_512KB, CARD_1MB, CARD_SIZES };

struct vmsfs_context {
    Sint32 device_type; /* sets the byte order of the cards' words */
    /* buInit's capacity as a CARD_ index, not as its BUD_CAPACITY_ code.
     * nothing reads it */
    Sint32 capacity;
    /* whether each drive gets its work area from vmsfs_setwork, not buInit */
    Sint32 dynamic_work;
    Uint16 (*get_word)(void *buf, Sint32 offset);
    void (*set_word)(void *buf, Sint32 offset, Uint16 value);
};
/* the context, and _vm, which vmsfs_init points at it. SHC lays out vmsphys.c's
 * bss in the order its names are first declared. these two are first declared
 * here, in the order vmsphys.c defines them */
extern struct vmsfs_context buVmsContext;
extern struct vmsfs_context *_vm;

struct vmsfs_drive {
    Sint16 number;
    char has_work;
    Sint32 capacity; /* the card's CARD_ index */
    char mounted;
    char ready;
    char sys_cached;
    char fat_cached;
    char dir_cached;
    Sint16 sys_blocks;
    Sint16 fat_blocks;
    Sint16 dir_blocks;
    Sint16 gap;
    Sint32 find_slot;
    struct vmsfs_media *media;
    Uint8 *sys;
    Uint8 *fat;
    Uint8 *dir;
    Uint8 *work; /* the card's system block, FAT and directory, after a gap */
    Sint32 progress; /* the running command's progress, out of progress_max */
    Sint32 progress_max;
    char fat_unsaved[16];
    char dir_unsaved[256];
};
/* @hack type: declared without a size, though vmsphys.c defines it with
 * VMSFS_DRIVES, so that SHC multiplies a drive's number by the record's size
 * with MUL.L, in 32 bits. with a size, it multiplies with MULS.W, in 16 bits,
 * and vms_rw.c's and vmsio.c's code each grows 8 bytes, count unmeasured */
extern struct vmsfs_drive buVmsDrive[];

typedef struct {
    char open;
    Uint8 copyflag;
    Uint16 hdrofs;
    Sint32 mode;
    Sint32 type;
    Sint32 index;
    char fname[16];
    Uint8 time[8];
    Sint32 drive;
    Sint32 slot;
    Sint32 written;
    Sint32 remaining;
    Sint32 first_block;
    Sint32 prev_block;
    Sint32 next_block;
    Sint32 unk_44;
} BUS_FILE;
typedef BUS_FILE *BUFILE;

/* file system calls that return at once */
Sint32 vmsfs_init(Sint32 capacity, Sint32 drives, void *work);
Sint32 vmsfs_exit(void);
Uint32 vmsfs_isready(void);
Sint32 vmsfs_ismount(Sint32 drive);
Sint32 vmsfs_unmount(Sint32 drive);
Sint32 vmsfs_isformat(Sint32 drive);
Sint32 vmsfs_getinfo(Sint32 drive, BUS_DISKINFO *info);
/* the free blocks a file of type can take. the executable file can take the
 * free blocks from block 0 up to the first that isn't free, at most exec_size.
 * any other file can take every free user block, even one a file's directory
 * entry starts at: vmsphys.c's buRealFree, which would leave those out, is 0.
 * BUD_ERR_NO_DISK for a drive without a work area or a mounted card, and
 * BUD_ERR_UNFORMAT for an unformatted card */
Sint32 vmsfs_diskfree(Sint32 drive, Sint32 type);
Sint32 vmsfs_findfirst(Sint32 drive, char *fname);
Sint32 vmsfs_findnext(Sint32 drive, char *fname);
Sint32 vmsfs_findfirst_ex(Sint32 drive, char *fname, BUS_FILEINFO *finfo);
Sint32 vmsfs_findnext_ex(Sint32 drive, char *fname, BUS_FILEINFO *finfo);
Sint32 vmsfs_xfind(Sint32 drive, char *fname);
Sint32 vmsfs_fopen(Sint32 drive, const char *fname, BUFILE *file);
/* create a file and open it for writing in *file, and return BUD_ERR_OK. a
 * file of the same name is deleted first, in the caches alone. the card loses
 * it with the drive's next writes of the directory and FAT, such as
 * vmsfs_fclose's. the new directory entry is written when vmsfs_fclose closes
 * the file. BUD_ERR_CANNOT_CREATE when the directory's first user_blocks slots
 * are all used, and BUD_ERR_CANNOT_OPEN when 8 files are open. either leaves
 * the old file deleted in the caches. BUD_ERR_NO_DISK for a drive without a
 * work area or a mounted card, and BUD_ERR_UNFORMAT for an unformatted card.
 * any failure leaves *file NULL */
Sint32 vmsfs_fcreate(Sint32 drive, const char *fname, BUS_TIME *time,
                     BUFILE *file);
/* create the executable file and open it for writing in *file, and return
 * BUD_ERR_OK. a card holds at most one, and its blocks run in order from block
 * 0, which must be free. its directory entry is written when vmsfs_fclose
 * closes it. BUD_ERR_CANNOT_CREATE for a card without an executable area, a
 * block 0 that isn't free, or a directory whose first user_blocks slots are
 * all used. BUD_ERR_FILE_EXIST when a file has the name, BUD_ERR_EXECFILE_EXIST
 * when the card holds an executable file, and BUD_ERR_CANNOT_OPEN when 8 files
 * are open. BUD_ERR_NO_DISK for a drive without a work area or a mounted card,
 * and BUD_ERR_UNFORMAT for an unformatted card. any failure leaves *file
 * NULL */
Sint32 vmsfs_xcreate(Sint32 drive, const char *fname, BUS_TIME *time,
                     BUFILE *file);
Sint32 vmsfs_fsetparam(BUFILE file, Uint16 hdrofs, Uint8 copyflag);
Sint32 vmsfs_fisexist(Sint32 drive, const char *fname);
Sint32 vmsfs_fgetinfo(Sint32 drive, const char *fname, BUS_FILEINFO *finfo);
Sint32 vmsfs_fgetsize(Sint32 drive, const char *fname);
/* give drive the work area at addr, of size bytes, and return BUD_ERR_OK. size
 * must cover BUD_WORK_GAP bytes, then a block each for the system block and
 * the FAT and directory blocks, which buCapsFats and buCapsDirs count by the
 * drive's capacity. call vmsfs_connect first, which sets that capacity from the
 * card. it is CARD_128KB until then, and again after vmsfs_unmount. the drive
 * is then ready and mounted, with nothing cached, and vmsfs_mount must load
 * the caches. BUD_ERR_INVALID_PARAM when buInit was given a work area, addr is
 * NULL or size falls short */
Sint32 vmsfs_setwork(Sint32 drive, void *addr, Sint32 size);
extern void (*_vmsfs_dbg_out)(const char *str);
/* format a message and print it through _vmsfs_dbg_out, if set */
void vmsfs_dpf(const char *format, ...);

/* file system calls that read or write the card, which run on a drive's fiber
 * as its command */
Sint32 vmsfs_mount(Sint32 drive);
/* format a card. a zeroed system block is written first, then the directory
 * and FAT, and last the system block with the format mark. every block below
 * the system block is marked free in the FAT, and with erase it is first
 * written with zeros. a block whose write fails with BUD_ERR_WRITE_ERROR is
 * marked bad instead. one that fails with BUD_ERR_GENERIC is marked free all
 * the same.
 *
 * the result is that of reading the caches back from the card, whatever the
 * writes of the directory, the FAT and the last system block return.
 * BUD_ERR_ILLEGAL_DISK when the zeroed system block's write fails with
 * BUD_ERR_WRITE_ERROR, and BUD_ERR_UNKNOWN_DISK for a layout the file system
 * refuses. BUD_ERR_NO_DISK for a drive without a work area or a mounted card,
 * and for a zeroed system block's write that fails otherwise. a failed media
 * information read, or an erase write's BUD_ERR_NO_DISK, also returns
 * BUD_ERR_NO_DISK, and leaves the zeroed system block on the card */
Sint32 vmsfs_format(Sint32 drive, const Uint8 *volume, Sint32 icon,
                    const BUS_TIME *time, Sint32 erase);
/* close file, and free it whatever the result. a file open for writing gets
 * its directory entry, and the directory's changed blocks and the FAT go to the
 * card. with report, such a close first starts the drive's progress over for
 * BUD_OP_CLOSEFILE. the result is the directory write's, and a failed FAT write
 * is ignored. BUD_ERR_INVALID_PARAM for a NULL file. on a drive without a work
 * area or a mounted, formatted card, nothing is written and the result is
 * BUD_ERR_OK */
Sint32 vmsfs_fclose(BUFILE file, Sint32 report);
Sint32 vmsfs_fread(BUFILE file, void *buf, Sint32 nblock);
Sint32 vmsfs_fwrite(BUFILE file, const void *buf, Sint32 nblock);
Sint32 vmsfs_fdelete(Sint32 drive, const char *fname);
Sint32 vmsfs_fdelete_ex(Sint32 drive, const char **fnames, Sint32 nfiles);
Sint32 vmsfs_fsetattr(Sint32 drive, const char *fname, Uint16 hdrofs,
                      Uint8 copyflag);
Sint32 vmsfs_frename(Sint32 drive, const char *from, const char *to);
/* move the files' blocks up, to free the low blocks for an executable file.
 * the lowest used user block moves to the highest free one above it, until no
 * free block lies above the lowest used one. buf must hold 512 bytes of
 * scratch. each move copies its block on the card, and changes the FAT and
 * directory only in the caches. after the last, the directory and then the FAT
 * are written, and the first error is the result. a failed block read or write
 * returns its error at once, with the card's directory and FAT as they were.
 * BUD_ERR_ACCESS_DENIED when the card takes no executable file or holds one, or
 * when a FAT link leaves its area. BUD_ERR_NO_DISK for a drive without a work
 * area or a mounted card, and BUD_ERR_UNFORMAT for an unformatted card */
Sint32 vmsfs_xdefrag(Sint32 drive, void *buf);
Sint32 vmsfs_read_disk_image(Sint32 drive, void *buf);
Sint32 vmsfs_write_disk_image(Sint32 drive, const void *buf);
Sint32 vmsfs_load_file(Sint32 drive, const char *fname, void *buf,
                       Uint32 nblock);
Sint32 vmsfs_load_file_ex(Sint32 drive, const char *fname, void *buf,
                          Uint32 start, Uint32 nblock);
Sint32 vmsfs_save_file(Sint32 drive, const char *fname, const void *buf,
                       Uint32 nblock, BUS_TIME *time, Sint32 flag);
Sint32 vmsfs_save_exec_file(Sint32 drive, const char *fname, const void *buf,
                            Uint32 nblock, BUS_TIME *time, Sint32 flag);
/* read a card's media information and its block 0, without mounting it, and
 * set the drive's capacity, by which vmsfs_setwork sizes the drive's work area.
 * return the card's BUD_CAPACITY_ code, which counts the blocks of work area
 * its drive needs. BUD_ERR_NO_DISK when the card doesn't answer, and
 * BUD_ERR_UNKNOWN_DISK for a layout the file system refuses */
Sint32 vmsfs_connect(Sint32 drive);
/* write nblock blocks over the executable file, from its block start. the
 * file's blocks run in order from block 0, so each of its blocks is the card
 * block of the same number. BUD_ERR_ACCESS_DENIED for a file that isn't the
 * executable, and BUD_ERR_INVALID_PARAM for blocks past its end */
Sint32 vmsfs_rewrite_exec_file(Sint32 drive, const char *fname, const void *buf,
                               Uint32 start, Uint32 nblock);

/* read and write the cards' words big-endian for a type with bit 0 set, and
 * little-endian for one with bit 0 clear */
void vmsfs_setdevicetype(Sint32 type);
/* keep only the capacity, for drives that get their work from vmsfs_setwork */
Sint32 vmsfs_init_work_new(Sint32 capacity, Sint32 drives, Uint8 *work);
/* divide buInit's work area among the drives */
Sint32 vmsfs_init_work_normal(Sint32 capacity, Sint32 drives, Uint8 *work);
Sint32 vmsfs_resetwork(Sint32 drive); /* zero a drive's record */
Sint32 vmsfs_mount2(Sint32 drive); /* mount on the area from vmsfs_setwork */
/* the free blocks among all the blocks the FAT has words for */
Sint32 vmsfs_sysdiskfree(Sint32 drive);
/* write file's next block to the highest free user block that no file starts
 * at. when that write fails with BUD_ERR_NO_DISK, the call returns it. after
 * any other failure, each failed write marks its block bad in the FAT, whatever
 * its error, and the write moves to the highest free user block left. that
 * block can be one a file starts at. BUD_ERR_DISK_FULL when no free user block
 * is left */
Sint32 vmsfs_fwrite1(BUFILE file, const void *buf);
/* write the executable file's next block, the one whose number is the count
 * already written. BUD_ERR_DISK_FULL when that block isn't free, or isn't
 * below exec_size. a failed write returns its error, with no other block
 * tried */
Sint32 vmsfs_xwrite1(BUFILE file, const void *buf);
/* read a file's next block, and return the blocks it has left. at the file's
 * end it reads nothing and returns 0. BUD_ERR_GENERIC when the chain ends
 * before the file's length, and BUD_ERR_FILE_BROKEN when it reaches a free or
 * bad FAT word */
Sint32 vmsfs_fread1(BUFILE file, void *buf);
Sint32 vmsfs_fsettime(Sint32 drive, const char *fname, const BUS_TIME *time);
Sint32 vmsfs_fgettime(Sint32 drive, const char *fname, BUS_TIME *time);
Sint32 vmsfs_getvolume(Sint32 drive, Uint8 *volume);
Sint32 vmsfs_setvolume(Sint32 drive, const Uint8 *volume);
Sint32 vmsfs_stat(void); /* does nothing, and returns 0 */
/* saves that read the file back and compare it. vmsfs_save_file_verify writes
 * the file as vmsfs_save_file does, then reads each block back and compares
 * it. a block that differs returns BUD_ERR_VERIFY, and the file stays on the
 * card as written */
Sint32 vmsfs_save_file_verify(Sint32 drive, const char *fname, const void *buf,
                              Uint32 nblock, BUS_TIME *time, Sint32 flag);
Sint32 vmsfs_save_exec_file_verify(Sint32 drive, const char *fname,
                                   const void *buf, Uint32 nblock,
                                   BUS_TIME *time, Sint32 flag);
/* move file's next block to its first block, then step block blocks along its
 * chain. each step takes one from file->remaining, which the call doesn't reset
 * first. it returns file->remaining after the steps. only on a file not yet
 * read or sought is that the count of blocks from the new position to the
 * file's end. at the chain's end it stops, and returns 0 when file->remaining
 * is 0 there and BUD_ERR_GENERIC otherwise. it returns BUD_ERR_GENERIC when
 * file->remaining reaches 0 before the chain's end. origin is never read */
Sint32 vmsfs_fseek(BUFILE file, Uint32 block, Sint32 origin);
/* store the chain of the file fname in chain, and return its directory slot.
 * chain must have room for one word more than the file has blocks: each
 * block's number, then the last block's FAT word. the words are stored before
 * the chain is checked, so a broken chain fills them too, and the check's
 * error is returned. BUD_ERR_FILE_NOT_FOUND when no file has the name */
Sint32 vmsfs_scanfile(Sint32 drive, const char *fname, Uint16 *chain);

/* a drive's command, which the drive's fiber runs */
struct vmsfs_command {
    Sint32 unk_00;
    Sint32 busy;
    Sint32 op;
    Sint32 unk_0c;
    Sint32 drive;
    Sint32 param1;
    Sint32 param2;
    Sint32 param3;
    Sint32 param4;
    Sint32 param5;
    Sint32 param6;
    char fname[16];
    char second_name[16];
    Uint8 volume[32];
    BUS_TIME time;
    BU_COMPLETECALLBACK complete;
    BU_PROGRESSCALLBACK progress;
    Uint32 param;
};
/* SHC lays out vmscmd.c's bss in the order its names are first declared.
 * these two are first declared here, in the order vmscmd.c defines them */
extern Sint32 buLastError[VMSFS_DRIVES];
extern struct vmsfs_command buVmsCmd[VMSFS_DRIVES];

/* command setup. each drive holds one command: vmsfs_c_setfunc sets its
 * operation, and vmsfs_c_setparam its arguments. vmsfs_c_synch runs the fiber
 * server once, and each drive's fiber runs its command until it sleeps or
 * ends */
Sint32 vmsfs_c_init(Sint32 drives);
Sint32 vmsfs_c_exit(void);
void vmsfs_c_synch(void);
Sint32 vmsfs_c_getstate(Sint32 drive);
void vmsfs_c_setfunc(Sint32 drive, Sint32 op);
void vmsfs_c_setparam(Sint32 drive, Sint32 param1, Sint32 param2, Sint32 param3,
                      Sint32 param4, Sint32 param5, Sint32 param6);
void vmsfs_c_setvolume(Sint32 drive, const Uint8 *volume);
void vmsfs_c_setfname(Sint32 drive, const char *fname, const char *second_name);
void vmsfs_c_settime(Sint32 drive, const BUS_TIME *time);
void vmsfs_c_progress(Sint32 drive, Sint32 op, Sint32 count, Sint32 max);
void vmsfs_c_execcallback(Sint32 drive, Sint32 op, Sint32 status);
void vmsfs_c_setcompletecallback(BU_COMPLETECALLBACK func);
void vmsfs_c_setprogresscallback(BU_PROGRESSCALLBACK func);
void vmsfs_c_setcallbackparam(Uint32 param);

/* card block access over the Maple bus, through Kaede. vmsio_get_media_info
 * recognizes the card in a drive. vmsio_block_read and vmsio_block_write
 * return BUD_ERR_NO_DISK unless the drive still holds that card. these three
 * calls send the card their commands, and send a command again when the card
 * answers KDD_RES_TRANSMITAGAIN. they sleep the calling fiber until Kaede
 * queues each command, and again until the card responds. the other calls
 * here return at once */
void vmsio_init(void);
void vmsio_exit(void);
/* fill every field of media but user_blocks for the card in drive, and
 * recognize the card for the block calls. returns BUD_ERR_OK, or
 * BUD_ERR_NO_DISK when the drive holds no storage device or kdDevGetStorageInfo
 * fails. it also returns BUD_ERR_NO_DISK when the card answers anything but its
 * media information, or changes during the call. a failure can leave media
 * partly filled */
Sint32 vmsio_get_media_info(Sint32 drive, struct vmsfs_media *media);
/* read block of drive's card into the 512 bytes at buf, in read_phases
 * commands of read_bytes each, as the drive's media gives them. returns
 * BUD_ERR_OK, or BUD_ERR_GENERIC when the card answers a command with
 * KDD_RES_FILEERROR. it returns BUD_ERR_NO_DISK when the drive no longer
 * holds the card or the card gives any other answer. a failure can leave buf
 * partly filled */
Sint32 vmsio_block_read(Sint32 drive, Sint32 block, void *buf);
/* write the 512 bytes at buf to block of drive's card, in write_phases
 * commands of write_bytes each, as the drive's media gives them. it then
 * asks the card for the write's result. returns BUD_ERR_OK, or
 * BUD_ERR_WRITE_ERROR when the result is KDD_RES_FILEERROR. it returns
 * BUD_ERR_NO_DISK when the drive no longer holds the card or the card gives
 * any other answer */
Sint32 vmsio_block_write(Sint32 drive, Sint32 block, const void *buf);
Sint32 vmsio_isready(Sint32 drive);
Sint32 vmsio_ismount(Sint32 drive);
/* -1 if the two 512-byte blocks differ, else 0 */
Sint32 vmsfs_memcmp(const void *buf1, const void *buf2);

/* the vmsio media info and block calls, retried after a failure: count times,
 * or until success for -1 */
Sint32 vmsfs_setretry(Sint32 count);
Sint32 vmsfs_mediainfo_retry(Sint32 drive, struct vmsfs_media *media);
Sint32 vmsfs_read_retry(Sint32 drive, Sint32 block, void *buf);
Sint32 vmsfs_write_retry(Sint32 drive, Sint32 block, const void *buf);

/* file system fibers: one for each drive, on one fiber server */
struct rf_server *vmsfb_init(void);
Sint32 vmsfb_create(void (*func)(void), void *stack, Sint32 size);
void vmsfb_exec(void);
void vmsfb_exit(void);
void vmsfb_sleep(void);
void vmsfb_end(void);

#endif
