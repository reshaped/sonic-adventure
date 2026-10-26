/* LOADER.BIN's start and end of the system: src/system/sbinit.c's, with a
 * fixed heap, no cache setup and no change of directory */
#include <machine.h>
#include <ninja.h>
#include <sg_syhw.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "loader/sbinit.h"
#include "loader/sdk/gdfs.h"

/* the heap is main memory's last megabyte */
#define HEAP ((void *)0x8cf00000)
#define HEAP_SIZE 0x100000

#define PAD_BUFFER_SIZE (1024 * 24 * 2)

/* the most files open at once, the most directory entries, and the tries
 * gdFsInit gets */
#define GD_FILES 8
#define GD_ENTRIES 1920
#define GD_TRIES 8

/* the bytes of a directory of n entries in the loader's 1.20J GD file
 * system: 4 fewer than shinobi.h's GDFS_DIRREC_SIZE, which gives 1.30J's */
#define GD_DIRECTORY_SIZE(n)                                                   \
    (sizeof(GDFS_DIRREC_TBL) + (((n) - 1) * sizeof(GDFS_DIRREC_ENT)))

/* the first 32-byte boundary past buffer, which is buffer + 32 where buffer
 * is on one. each buffer has 32 bytes to spare for it */
#define ALIGN_32(buffer) ((void *)(((Uint32)(buffer) & ~31) + 32))

static Uint8 pad_recv_buffer[PAD_BUFFER_SIZE + 32];
static Uint8 pad_send_buffer[PAD_BUFFER_SIZE + 32];
static Uint8 gd_work[GDFS_WORK_SIZE(GD_FILES) + 32];
static Uint8 gd_directory[GD_DIRECTORY_SIZE(GD_ENTRIES) + 32];

void sbInitSystem(Int mode, Int frame, Int count)
{
    void *work;
    void *directory;
    Sint32 err;
    int tries;

    set_imask(IMASK_ALL);
    syHwInit();
    syMallocInit(HEAP, HEAP_SIZE);
    njInitSystem(mode, frame, count);
    syHwInit2();
    pdInitPeripheral(PDD_PLOGIC_ACTIVE, pad_recv_buffer, pad_send_buffer);
    syRtcInit();
    set_imask(0);
    work = ALIGN_32(gd_work);
    directory = ALIGN_32(gd_directory);
    /* an open tray or a changed disc ends the system and exits to the boot
     * ROM at once. any other error tries again. once GD_TRIES tries have
     * failed, the system ends and exits the same way */
    for (tries = GD_TRIES; tries > 0; tries--) {
        err = gdFsInit(GD_FILES, work, GD_ENTRIES, directory);
        if (err == GDD_ERR_TRAYOPEND || err == GDD_ERR_UNITATTENT) {
            sbExitSystem();
            syBtExit();
        } else if (err == GDD_ERR_OK) {
            break;
        }
    }
    if (tries == 0) {
        sbExitSystem();
        syBtExit();
    }
}

void sbExitSystem(void)
{
    gdFsFinish();
    syRtcFinish();
    pdExitPeripheral();
    njExitSystem();
    syMallocFinish();
    syHwFinish();
    set_imask(IMASK_ALL);
}
