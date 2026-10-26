/* memory block transfer completion and start of the next queued block */
#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>

void sdMemBlkEndTransferHandller(void *first_arg)
{
    int mask;
    Sint32 push_num;
    Sint32 next_dst;
    SDMEMBLK next;
    struct sd_memblk *block = first_arg;

    mask = get_imask();
    set_imask(IMASK_ALL);
    block->stat = SDE_MEMBLK_STAT_TRANSFER_FINISHED;
    sdgWrkPtr->transfer_end_num++;
    sdMemBlkExecuteCallback((SDMEMBLK)block);
    sdgWrkPtr->transfer = NULL;
    /* the same end counted a second time, so transfer_end_num rises by 2 for
     * each transfer */
    sdgWrkPtr->transfer_end_num++;
    sdMemBlkQueGetPushNum(&push_num);
    if (push_num > 0) {
        sdMemBlkQuePop(&next, &next_dst);
        switch (sdgWrkPtr->transfer_mode) {
        case SDE_MEMBLK_TRANSFER_MODE_CPU:
            sdMemBlkTransfer(next, next_dst);
            break;
        case SDE_MEMBLK_TRANSFER_MODE_DMA:
            sdMemBlkDmaTransfer(next, next_dst);
            break;
        }
    }
    set_imask(mask);
}
