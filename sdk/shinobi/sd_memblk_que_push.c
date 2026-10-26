/* memory block DMA queue insertion, with interrupts masked */
#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMemBlkQuePush(SDMEMBLK handle, Sint32 dst_ptr)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    int mask;

    mask = get_imask();
    set_imask(IMASK_ALL);
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (sdgWrkPtr->queue_max <= sdgWrkPtr->queue_num) {
        err = SDE_ERR_QUEUE_NO_ENOUGH;
        goto error;
    }
    sdgWrkPtr->queue[sdgWrkPtr->queue_write].handle = handle;
    sdgWrkPtr->queue[sdgWrkPtr->queue_write].dst_ptr = dst_ptr;
    sdgWrkPtr->queue_write++;
    if (sdgWrkPtr->queue_write == sdgWrkPtr->queue_max) {
        sdgWrkPtr->queue_write = 0;
    }
    sdgWrkPtr->queue_num++;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkQuePush, err);
    }
    set_imask(mask);
    return err;
}
