/* memory block completion callback */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMemBlkExecuteCallback(SDMEMBLK handle)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    /* @hack spelling: block, a copy of handle, which tests handle in R4 and
     * keeps block in R14 (0x8c65dc42, 0x8c65dc4c); through handle, it moves to
     * R14 first and 3 of 33 words differ */
    struct sd_memblk *block = (struct sd_memblk *)handle;

    if (handle == NULL) {
        err = SDE_ERR_HANDLE_NULL;
    } else if (block->callback != SDD_MEMBLK_NO_FUNC) {
        /* a block sent with SDD_MEMBLK_SYNC_FUNC is marked done, with nothing
         * to call */
        if (block->callback == SDD_MEMBLK_SYNC_FUNC) {
            block->done = true;
        } else {
            block->callback(block->callback_first_arg);
        }
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkExecuteCallback, err);
    }
    return err;
}
