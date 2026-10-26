#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMemBlkTransfer(SDMEMBLK handle, Sint32 dst_ptr)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    int mask = get_imask();
    /* @hack spelling: block, a copy of handle, which keeps handle in R11 and
     * block in R13 (0x8c65dd1a, 0x8c65dd22); through handle, 129 of 137 words
     * differ */
    struct sd_memblk *block = (struct sd_memblk *)handle;
    SDE_MEMBLK_STAT stat;
    SDMEMBLK next;
    Sint32 next_dst;

    set_imask(IMASK_ALL);
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (handle == NULL) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    sdMemBlkGetStat(handle, &stat);
    if (stat == SDE_MEMBLK_STAT_TRANSFER_PROGRESS) {
        err = SDE_ERR_MEMBLK_ALREADY_TRANSFER;
        goto error;
    }
    sdgG2LibResult = SYE_G2_ERR_NOTHING;
    block->done = false;
    switch (sdgWrkPtr->transfer_mode) {
    case SDE_MEMBLK_TRANSFER_MODE_CPU:
        err = sdMemBlkCpuTransfer(handle, dst_ptr);
        /* clang-format off */
        /* @hack redundant: a goto to error, where the break leads anyway. SHC
         * then branches past the trap when err is 0 (BT at 0x8c65dd82).
         * without it, the function runs 8 bytes short and 101 of 137 words
         * differ */
        /* @hack spelling: the goto unbraced, which has SHC call
         * sdMemBlkCpuTransfer through R3 (0x8c65dd78). braced, R2 and R3 trade
         * and 12 of 137 words differ */
        if (err != SDE_ERR_NOTHING) /* NOLINT(readability-braces-around-statements) */
            goto error;
        /* clang-format on */
        break;
    case SDE_MEMBLK_TRANSFER_MODE_DMA:
        err = sdMemBlkQuePush(handle, dst_ptr);
        if (err != SDE_ERR_NOTHING) {
            goto error;
        }
        /* while a transfer is under way, the block waits in the queue for
         * sdMemBlkEndTransferHandller to start it */
        if (sdgWrkPtr->transfer != NULL) {
            goto error;
        }
        sdMemBlkQuePop(&next, &next_dst);
        err = sdMemBlkDmaTransfer(next, next_dst);
        if (err != SDE_ERR_NOTHING) {
            goto error;
        }
        /* restore the interrupt mask before the wait for a synchronous
         * block. mask -1 tells the exit that it is restored */
        set_imask(mask);
        mask = -1;
        if (block->callback == SDD_MEMBLK_SYNC_FUNC) {
            while (block->done != true) {
                sdMemBlkWaitSync();
            }
        }
        break;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkTransfer, err);
    }
    if (mask >= 0) {
        set_imask(mask);
    }
    return err;
}
