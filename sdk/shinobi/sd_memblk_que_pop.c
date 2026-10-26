/* memory block DMA queue removal, with interrupts masked */
#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* take the oldest entry off the queue. on an empty queue it gives a NULL
 * handle and offset 0, and queue_num goes negative */
SDE_ERR sdMemBlkQuePop(SDMEMBLK *handle, Sint32 *dst_ptr)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    int mask;

    mask = get_imask();
    set_imask(IMASK_ALL);
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else {
        *handle = sdgWrkPtr->queue[sdgWrkPtr->queue_read].handle;
        *dst_ptr = sdgWrkPtr->queue[sdgWrkPtr->queue_read].dst_ptr;
        sdgWrkPtr->queue[sdgWrkPtr->queue_read].handle = NULL;
        sdgWrkPtr->queue[sdgWrkPtr->queue_read].dst_ptr = 0;
        sdgWrkPtr->queue_read++;
        if (sdgWrkPtr->queue_read == sdgWrkPtr->queue_max) {
            sdgWrkPtr->queue_read = 0;
        }
        sdgWrkPtr->queue_num--;
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkQuePop, err);
    }
    set_imask(mask);
    return err;
}
