/* sound driver download and start */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* the header before the driver's ARM7 program in its memory block */
struct driver_header {
    SDE_DATA_TYPE type;
    Sint8 major;
    Sint8 minor;
    Sint8 unk_06[2];
    Sint32 size;
    Uint32 unk_0c[5];
};

/* driver versions the library accepts: 1.0 and 1.1 */
#define DRIVER_MAJOR 1
#define DRIVER_MINOR_MAX 1

/* the passes of the empty loop that gives the driver time to start */
#define DRIVER_START_DELAY 0x200000

SDE_ERR sdDrvInit(SDMEMBLK handle)
{
    /* @hack spelling: driver_block, static in the function, whose address SHC
     * loads from the pool at each use (0x8c618d4a to 0x8c618d94). at file
     * scope, SHC keeps it in R12 throughout and 66 of 91 words differ */
    static SDMEMBLK driver_block = NULL;
    SDE_ERR err = SDE_ERR_NOTHING;
    struct driver_header *header;
    Sint32 delay;

    if (handle == NULL) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    header = ((struct sd_memblk *)handle)->data;
    if (header->type != SDE_DATA_TYPE_SND_DRV) {
        err = SDE_ERR_DATA_ILLEGAL_TYPE;
        goto error;
    }
    if (header->major != DRIVER_MAJOR || header->minor > DRIVER_MINOR_MAX) {
        err = SDD_ERR_SND_DRV_ILLEGAL_VER;
    } else {
        sdARM7ResetKeep();
        sdSndMemClear();
        sdMemBlkCreate(&driver_block);
        sdMemBlkSetPrm(driver_block, header + 1, header->size,
                       SDD_MEMBLK_SYNC_FUNC, NULL);
        sdMemBlkTransfer(driver_block, 0);
        sdARM7ResetRel();
        for (delay = DRIVER_START_DELAY; delay > 0; delay--) {
        }
        sdgExistSndDrv = true;
        sdDrvGetInfo();
        sdMemBlkExecuteCallback(handle);
    }

error:
    /* @hack spelling: the test and the destroy as a conditional expression
     * whose value goes unused, where an if would do. SHC loads driver_block
     * again for the call (MOV.L @R3,R4 at 0x8c618da0); as an if, it passes the
     * tested load, and 8 of 91 words differ */
    driver_block != NULL ? sdMemBlkDestroy(driver_block) : SDE_ERR_NOTHING;
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdDrvInit, err);
    }
    return err;
}
