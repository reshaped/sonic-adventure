/* reads from the disc: whole files, PRS files, a stage's programs and files,
 * and a queue of reads in the background */
#ifndef SYSTEM_LOADER_H
#define SYSTEM_LOADER_H
#include <sg_xpt.h>

/* a GD-ROM sector's bytes, and the shift that divides by SECTOR_SIZE */
#define SECTOR_SIZE 2048
#define SECTOR_SHIFT 11
/* the sectors that size bytes take, the last one in part */
#define SECTORS(size) (((size) + (SECTOR_SIZE - 1)) >> SECTOR_SHIFT)

/* a 2 MB workspace, up to WORK_AREA_END, that many uses share. each use
 * writes over what the last left there. among them are loader_read_prs's
 * input, the memory cards' file images and the flash history's work */
#define WORK_AREA ((Uint8 *)0x0cd00000)
#define WORK_AREA_END ((Uint8 *)0x0cf00000)

/* where a stage's module, or an overlay, loads */
#define MODULE_AREA ((Uint8 *)0x0c900000)

/* where loader_read_character_motions reads the character's motions */
#define MOTION_AREA ((Uint8 *)0x0cc00000)

/* where Sonic's and Tails' board motions load, s_sbmot and m_sbmot */
#define SONIC_BOARD_AREA ((Uint8 *)0x0cb08000)
#define TAILS_BOARD_AREA ((Uint8 *)0x0cadc000)

/* where a stage's set files and camera files load, an area for each act.
 * each area holds 16 KB. areas 0 to 2 lie 32 KB apart, and areas 3 to 5
 * fill the 16 KB after each */
#define SET_AREA_0 ((Uint8 *)0x0cb68000)
#define SET_AREA_1 ((Uint8 *)0x0cb70000)
#define SET_AREA_2 ((Uint8 *)0x0cb78000)
#define SET_AREA_3 ((Uint8 *)0x0cb6c000)
#define SET_AREA_4 ((Uint8 *)0x0cb74000)
#define SET_AREA_5 ((Uint8 *)0x0cb7c000)
#define CAMERA_AREA_0 ((Uint8 *)0x0cb50000)
#define CAMERA_AREA_1 ((Uint8 *)0x0cb58000)
#define CAMERA_AREA_2 ((Uint8 *)0x0cb60000)
#define CAMERA_AREA_3 ((Uint8 *)0x0cb54000)
#define CAMERA_AREA_4 ((Uint8 *)0x0cb5c000)
#define CAMERA_AREA_5 ((Uint8 *)0x0cb64000)

/* exit to the boot ROM once loader_note_error has noted a disc error,
 * unless a memory card is saving. otherwise, while the drive is open or
 * busy, ask it for its status again */
void loader_poll_drive(void);
/* the GD file system's error callback. it sets loader_error_flag for an
 * opened tray or a changed disc, and ignores any other error */
void loader_note_error(void *obj, Sint32 err);

/* the reads declared from here to loader_file_size share one file handle,
 * so none may start while another runs.
 *
 * each comment below gives the shipped read's results. under NONMATCHING a
 * read differs once its file has opened. a failed size query returns as a
 * failed open does. a failed read doesn't return, as loader_failed says.
 * there loader_read_alloc's block holds the file's whole sectors */

/* the game can't go on without some of what it reads: a program it then
 * runs, the data a stage or a character plays from, a file's sectors once
 * the file has opened. under NONMATCHING the calls below read those, and
 * none returns from a failure. a source reads a file's sectors by
 * LOADER_MUST_READ, which is loader_must_read there and as shipped gdFsRead,
 * whose result those sources leave unread. must_load.h gives a source the
 * other two */
#ifdef NONMATCHING
#include <shinobi.h>
/* a file the game can't go on without failed. what says how in capitals, as
 * port_stop takes it, name is the file's and error GDFS's, or 0. at an
 * opened tray or a changed disc the game leaves for the boot ROM, as it
 * does at a frame's end, once any card save has ended. at any other
 * failure it stops, by port_stop. it doesn't return */
void loader_failed(const char *what, const char *name, Sint32 error)
    __attribute__((noreturn));
/* gdFsRead of file, whose name is name: sectors sectors into dest. it
 * returns once they are read. a read that fails is made again, up to tries
 * in all, each only once the request before it has ended: a failed read can
 * leave its request running, with dest still the drive's to write. where
 * gdFsStopRd can't end the request, the game stops, with dest and file
 * still taken. once every try has failed, loader_failed */
void loader_must_read_tries(GDFS file, const char *name, Sint32 sectors,
                            void *dest, int tries);
/* loader_must_read_tries with one try */
void loader_must_read(GDFS file, const char *name, Sint32 sectors, void *dest);
/* loader_read_prs and loader_read_file, but a file that doesn't open or
 * give its size is loader_failed's. each returns 0 */
int loader_must_read_prs(char *name, void *dest);
int loader_must_read_file(char *name, void *dest);
#define LOADER_MUST_READ(file, name, sectors, dest)                            \
    loader_must_read(file, name, sectors, dest)
#else
#define LOADER_MUST_READ(file, name, sectors, dest)                            \
    gdFsRead(file, sectors, dest)
#endif
/* under NONMATCHING, a call of loader_read_file or loader_read_prs that
 * leaves the result unread doesn't compile: a source either handles a file
 * that won't open, or includes must_load.h */
#ifdef NONMATCHING
#define LOADER_RESULT __attribute__((warn_unused_result))
#else
#define LOADER_RESULT
#endif

/* read the current stage's programs, and each act's set and camera files.
 * for a stage without files, as STAGE_EGG_CARRIER_2 and
 * STAGE_EGG_CARRIER_3 are, it spins forever */
void loader_read_stage_files(void);
/* read character's motions into MOTION_AREA, Tails' from Sonic's s_mot.
 * for Eggman, Tikal or a number past the characters, it spins forever */
void loader_read_character_motions(int character);
/* read the file name into a block malloc allocates to its size, and return
 * the block, for loader_free to free. it returns NULL when the open, the
 * size query, the allocation or the read's own open fails. the read itself
 * goes unchecked. it reads whole sectors, so a size that isn't a multiple
 * of SECTOR_SIZE runs the read past the block. the caller's result goes
 * unread: the read's status overwrites it */
void *loader_read_alloc(char *name, int result);
void loader_free(void *memory);
/* read the file name into dest in whole sectors, so dest needs room for
 * SECTORS(size) of them. it returns -1 when the open fails, and 0
 * otherwise, even when the size query or the read fails */
int loader_read_file(char *name, void *dest) LOADER_RESULT;
/* read the PRS file name into WORK_AREA and decode it into dest. the file
 * must fit WORK_AREA in whole sectors, and dest must not overlap input the
 * decoder has yet to read. it invalidates the instruction cache, for the
 * code dest may now hold. it returns -1 or 0 as loader_read_file does,
 * not the decoded size. the file must hold a valid PRS stream:
 * prs_decompress checks nothing */
int loader_read_prs(char *name, void *dest) LOADER_RESULT;
/* a file's size in bytes, or -1 when the open or the size query fails */
int loader_file_size(char *name);

/* a queue of 16 reads, which read_queue_serve runs in the background one
 * at a time. it reports no read's end or failure */

/* empty the queue and select no read. call it only while no read runs: it
 * neither stops nor closes one */
void read_queue_init(void);
/* the selected read's slot plus one, 1 to 16, or 0 when none is. a waiting
 * read isn't selected until read_queue_serve starts it. one whose open
 * fails stays selected until the next read_queue_serve drops it */
int read_queue_selected_slot(void);
/* queue a read of the file name into dest and return TRUE, or return FALSE
 * when all 16 slots are taken. name may be at most 31 characters: strcpy
 * copies it into a 32-byte slot unchecked. dest must be 32-byte aligned,
 * hold the file in whole sectors, and stay valid until the read ends */
Bool read_queue_add(char *name, void *dest);
/* drop every read and close the selected read's file. gdFsClose ignores
 * gdFsStopRd's result, so a read it fails to stop goes on into its dest */
void read_queue_cancel(void);
/* move the queue on. while the selected read runs, it does nothing. it
 * starts a read again that fails with GDD_STAT_ERR. once the read ends any
 * other way, it frees the read's slot. then, with no read selected, it
 * starts the waiting read in the lowest slot. a read whose open fails stays
 * selected until the next call drops it. the size query and the read
 * request go unchecked */
void read_queue_serve(void);

#endif
