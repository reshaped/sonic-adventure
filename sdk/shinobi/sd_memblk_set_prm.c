#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMemBlkSetPrm(SDMEMBLK handle, const void *src_blk_ptr,
                       const Sint32 src_blk_sz,
                       const SD_MEMBLK_CALLBACK_FUNC callback_func,
                       const void *callback_first_arg)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    SDE_MEMBLK_STAT stat;

    /* @hack redundant: the else after the goto, which has SHC call
     * sdMemBlkGetStat through R3 (0x8c6184ec). without it, SHC calls
     * sdMemBlkGetStat through R2, and 2 of 64 words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (handle == NULL) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    sdMemBlkGetStat(handle, &stat);
    if (stat == SDE_MEMBLK_STAT_TRANSFER_PROGRESS) {
        err = SDE_ERR_UNKNOW_NUM;
    } else {
        sdMemBlkSetStat(handle, SDE_MEMBLK_STAT_TRANSFER_REMAIN);
        sdMemBlkSetAdr(handle, src_blk_ptr);
        sdMemBlkSetSz(handle, src_blk_sz);
        sdMemBlkSetCallback(handle, callback_func, callback_first_arg);
        sdMemBlkSetStat(handle, SDE_MEMBLK_STAT_TRANSFER_REMAIN);
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkSetPrm, err);
    }
    return err;
}
