/* memory block transfer by G2 DMA */
#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* count of failed G2 DMA setups and starts */
Uint32 sdgMemBlkG2DmaErrNum = 0;

SDE_ERR sdMemBlkDmaTransfer(SDMEMBLK handle, Sint32 dst_ptr)
{
    SDE_MEMBLK_STAT stat;
    int mask;
    struct sd_memblk *block;
    SDE_ERR err;

    mask = get_imask();
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
    block = (struct sd_memblk *)handle;
    block->done = false;
    sdgG2LibResult =
        syG2DmaSetPrm(sdgWrkPtr->dma, (void *)(SD_SND_MEM + dst_ptr),
                      block->data, block->size, SYE_G2_DMA_DIR_ROOT_TO_G2, true,
                      true, sdMemBlkEndTransferHandller, block);
    if (sdgG2LibResult != SYE_G2_ERR_NOTHING) {
        sdMemBlkG2DmaErrTrap();
    }
    err = sdMemBlkSetStat(handle, SDE_MEMBLK_STAT_TRANSFER_PROGRESS);
    if (err != SDE_ERR_NOTHING) {
        goto error;
    }
    sdgWrkPtr->transfer = handle;
    sdgWrkPtr->transfer_num++;
    sdgG2LibResult = syG2DmaTrigger(sdgWrkPtr->dma, SYE_G2_DMA_TRIGGER_CPU);
    if (sdgG2LibResult != SYE_G2_ERR_NOTHING) {
        sdMemBlkG2DmaErrTrap();
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkDmaTransfer, err);
    }
    set_imask(mask);
    return err;
}

/* sdMemBlkTransfer calls it on each pass of its wait for a synchronous
 * block. it does nothing */
void sdMemBlkWaitSync(void)
{
}

void sdMemBlkG2DmaErrTrap(void)
{
    sdgMemBlkG2DmaErrNum++;
}
