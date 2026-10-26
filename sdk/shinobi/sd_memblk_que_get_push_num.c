/* memory block DMA queue length */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMemBlkQueGetPushNum(Sint32 *push_num)
{
    struct sd_work *work = sdgWrkPtr;
    SDE_ERR err = SDE_ERR_NOTHING;

    if (work == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    *push_num = work->queue_num;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkQueGetPushNum, err);
    }
    return err;
}
