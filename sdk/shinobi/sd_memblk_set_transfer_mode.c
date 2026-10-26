#include "shinobi/sd_work.h"
#include <shinobi.h>

SDE_ERR sdMemBlkSetTransferMode(SDE_MEMBLK_TRANSFER_MODE transfer_mode)
{
    if (sdgWrkPtr != NULL) {
        sdgWrkPtr->transfer_mode = transfer_mode;
    }
    return SDE_ERR_NOTHING;
}
