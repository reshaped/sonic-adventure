/* memory block table initialization */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* an empty block: unused, not transferred, with no data, a size of -1 and
 * no callback */
static struct sd_memblk memblk_default = {false,
                                          SDE_MEMBLK_STAT_TRANSFER_REMAIN,
                                          NULL,
                                          -1,
                                          SDD_MEMBLK_NO_FUNC,
                                          NULL,
                                          false,
                                          0};
/* address of the work's pointer to the block table */
struct sd_memblk **sdgMemBlkDbPtr;

SDE_ERR sdMemBlkInit(struct SDS_MEM_BLK *buffer, Sint32 count)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint32 i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else {
        sdgWrkPtr->memblk = (struct sd_memblk *)buffer;
        sdgWrkPtr->memblk_max = count;
        sdgWrkPtr->memblk_num = 0;
        sdgWrkPtr->transfer_num = 0;
        sdgWrkPtr->transfer_end_num = 0;
        sdgWrkPtr->transfer = NULL;
        sdgMemBlkDbPtr = &sdgWrkPtr->memblk;
        for (i = 0; i < count; i++) {
            sdgWrkPtr->memblk[i] = memblk_default;
        }
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkInit, err);
    }
    return err;
}
