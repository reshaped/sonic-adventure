/* memory block transfer by the CPU */
#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMemBlkCpuTransfer(SDMEMBLK handle, Sint32 dst_ptr)
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
    block = (struct sd_memblk *)handle;
    block->done = false;
    err = sdMemBlkSetStat(handle, SDE_MEMBLK_STAT_TRANSFER_PROGRESS);
    if (err != SDE_ERR_NOTHING) {
        goto error;
    }
    sdgWrkPtr->transfer = handle;
    sdgWrkPtr->transfer_num++;
    /* sdMoveWrkMemToSndMem calls sdMemBlkEndTransferHandller, which ends the
     * transfer, before it returns */
    err = sdMoveWrkMemToSndMem(block->data, dst_ptr, block->size,
                               sdMemBlkEndTransferHandller, block);
    /* clang-format off */
    /* @hack redundant: a goto to the next label. SHC keeps its test, TST
     * R14,R14 at 0x8c670fd6, ahead of the label's at 0x8c670fd8. without it,
     * the function runs 4 bytes short and 41 of 91 words differ */
    /* @hack spelling: the goto unbraced, which has SHC call sdMemBlkSetStat
     * through R3 (0x8c670fa0). braced, R2 and R3 trade and 6 of 91 words
     * differ */
    if (err != SDE_ERR_NOTHING) /* NOLINT(readability-braces-around-statements) */
        goto error;
    /* clang-format on */

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkCpuTransfer, err);
    }
    set_imask(mask);
    return err;
}
