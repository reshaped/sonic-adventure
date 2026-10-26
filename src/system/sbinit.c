/* hardware, memory and SDK subsystem initialization */
#include <machine.h>
#include <ninja.h>
#include <sg_syhw.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "system/cache.h"
#include "system/sbinit.h"

/* the heap is main memory's last megabyte. MEMORY_END is main memory's
 * end, in the address region _BSG_END lies in */
#define HEAP_SIZE 0x100000
#define MEMORY_END ((((Uint32)_BSG_END) & 0xe0000000) | 0x0d000000)

#define PAD_BUFFER_SIZE (1024 * 24 * 2)

/* GD filesystem limits: open files, directory entries and init attempts */
#define GD_FILES 8
#define GD_ENTRIES 1920
#define GD_TRIES 8

/* advance to the next 32-byte boundary, even if already aligned.
 * callers reserve 32 extra bytes for this adjustment */
#define ALIGN_32(buffer) ((void *)(((Uint32)(buffer) & ~31) + 32))

/* the end of the last bss section, as the linker puts it */
extern Uint8 *_BSG_END;

static Uint8 pad_recv_buffer[PAD_BUFFER_SIZE + 32];
static Uint8 pad_send_buffer[PAD_BUFFER_SIZE + 32];
static Uint8 gd_work[GDFS_WORK_SIZE(GD_FILES) + 32];
static Uint8 gd_directory[GDFS_DIRREC_SIZE(GD_ENTRIES) + 32];

void sbInitSystem(Int mode, Int frame, Int count)
{
    void *work;
    void *directory;
    Sint32 err;
    int tries;

    set_imask(IMASK_ALL);
    syHwInit();
    cache_ram_init();
    syMallocInit((void *)(MEMORY_END - HEAP_SIZE), HEAP_SIZE);
    njInitSystem(mode, frame, count);
    syHwInit2();
    pdInitPeripheral(PDD_PLOGIC_ACTIVE, pad_recv_buffer, pad_send_buffer);
    syRtcInit();
    set_imask(0);
    work = ALIGN_32(gd_work);
    directory = ALIGN_32(gd_directory);
    /* an open tray or a changed disc exits to the boot ROM at once, and any
     * other failure tries again */
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
    gdFsChangeDir("SONICADV");
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
