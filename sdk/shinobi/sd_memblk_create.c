/* memory block allocation */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* a block just taken: used, not transferred, with no data, a size of -1 and
 * no callback */
static struct sd_memblk memblk_created = {true,
                                          SDE_MEMBLK_STAT_TRANSFER_REMAIN,
                                          NULL,
                                          -1,
                                          SDD_MEMBLK_NO_FUNC,
                                          NULL,
                                          false,
                                          0};

SDE_ERR sdMemBlkCreate(SDMEMBLK *handle)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    /* @bug memory: block's initializer reads sdgWrkPtr->memblk before the test
     * of sdgWrkPtr, so with no work it reads through NULL: MOV.L @(R0,R4),R14
     * at 0x8c6183a2, in the slot of the test's BF/S */
    struct sd_memblk *block = sdgWrkPtr->memblk;
    Sint32 i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else {
        sdDebugPrintf("  Create:");
        /* @hack spelling: the found case below the error case, which a goto out
         * of the loop reaches (BF/S at 0x8c6183c4). with a break and a test of
         * i, the function runs 8 bytes longer and 52 of 82 words differ */
        for (i = 0; i < SD_MEMBLK_MAX; i++, block++) {
            sdDebugPrintf("%02X ", i);
            if (block->used != true) {
                goto found;
            }
        }
        sdDebugPrintf("Error.\n");
        err = SDE_ERR_HANDLE_NO_ENOUGH;
        *handle = NULL;
        goto done;
    found:
        sdDebugPrintf("Success Handle:%08X\n", block);
        *block = memblk_created;
        *handle = (SDMEMBLK)block;
        sdgWrkPtr->memblk_num++;
    }
done:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkCreate, err);
    }
    return err;
}
