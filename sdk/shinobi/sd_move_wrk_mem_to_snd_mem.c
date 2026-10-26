#include "shinobi/sd_work.h"
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMoveWrkMemToSndMem(void *src_ptr, Sint32 dst_ptr, Sint32 size,
                             SD_MEMBLK_CALLBACK_FUNC callback_func,
                             void *callback_first_arg)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    if (((Uint32)src_ptr & 3) != 0 || (size & 3) != 0) {
        err = SDE_ERR_UNKNOW_NUM;
    } else {
        sdgG2LibResult = syG2Write(src_ptr, (void *)(SD_SND_MEM + dst_ptr),
                                   SYE_G2_DATA_WIDTH_32BIT, size / 4,
                                   SYE_G2_ADR_MODE_ADD, SYE_G2_ADR_MODE_ADD);
        if (callback_func != SDD_MEMBLK_NO_FUNC &&
            callback_func != SDD_MEMBLK_SYNC_FUNC) {
            callback_func(callback_first_arg);
        }
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMoveWrkMemToSndMem, err);
    }
    return err;
}
