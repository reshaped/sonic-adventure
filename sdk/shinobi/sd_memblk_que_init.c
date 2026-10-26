/* memory block DMA queue initialization */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

static struct sd_memblk_queue queue_default = {NULL, 0};
struct sd_memblk_queue *sdgMemBlkQueBufPtr;
Sint32 sdgMemBlkQueBufNum;
/* address of the work's pointer to the queue */
struct sd_memblk_queue **sdgMemBlkQueDbPtr;

SDE_ERR sdMemBlkQueInit(struct sd_memblk_queue *buffer, Sint32 count)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint32 i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else {
        sdgWrkPtr->queue = buffer;
        sdgWrkPtr->queue_max = count;
        sdgWrkPtr->queue_write = 0;
        sdgWrkPtr->queue_read = 0;
        sdgWrkPtr->queue_num = 0;
        sdgMemBlkQueDbPtr = &sdgWrkPtr->queue;
        sdgMemBlkQueBufPtr = buffer;
        sdgMemBlkQueBufNum = count;
        for (i = 0; i < count; i++) {
            sdgWrkPtr->queue[i] = queue_default;
        }
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkQueInit, err);
    }
    return err;
}
