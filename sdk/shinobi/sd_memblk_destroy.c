/* memory block release */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMemBlkDestroy(SDMEMBLK handle)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    /* @hack spelling: block, a copy of handle, which keeps it in R5 (MOV R4,R5
     * at 0x8c61846a); through handle, 36 of 39 words differ */
    struct sd_memblk *block = (struct sd_memblk *)handle;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else if (handle == NULL) {
        err = SDE_ERR_HANDLE_ILLEGAL_VALUE;
    } else {
        /* @hack spelling: a switch of one case and a default, where an if
         * would do. SHC clears used through R2 (MOV #0,R2 at 0x8c618482); as
         * an if, R2 and R3 trade, and 8 of 39 words differ */
        switch (block->used) {
        case true:
            block->used = false;
            sdgWrkPtr->memblk_num--;
            break;
        default:
            err = SDE_ERR_HANDLE_ILLEGAL_VALUE;
        }
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkDestroy, err);
    }
    return err;
}
