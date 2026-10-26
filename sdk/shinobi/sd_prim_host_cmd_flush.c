#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdPrimaryHostCmdFlush(void)
{
    if (sdgWrkPtr->host.primary_num != 0) {
        sdgWrkPtr->host.busy = true;
        /* round an odd count up, so the transfer is a whole number of 32-byte
         * pairs of commands */
        if (sdgWrkPtr->host.primary_num & 1) {
            sdgWrkPtr->host.primary_num++;
        }
        sdMemBlkSetPrm(sdgWrkPtr->primary_block, sdgWrkPtr->host.primary,
                       sdgWrkPtr->host.primary_num *
                           sizeof(struct sd_primary_command),
                       sdPrimaryHostCmdFlushCallback, &sdgWrkPtr->host);
        sdMemBlkTransfer(sdgWrkPtr->primary_block, sdgWrkPtr->host_command);
    }
    return SDE_ERR_NOTHING;
}

void sdPrimaryHostCmdFlushCallback(void *first_arg)
{
    Sint32 i;

    sdSndMemWriteUint8(sdgWrkPtr->status, LIST_WAITING);
    for (i = 0; i < sdgWrkPtr->host.primary_num; i++) {
        sdgWrkPtr->host.primary[i].command = 0;
    }
    sdgWrkPtr->host.primary_num = 0;
    sdgWrkPtr->host.busy = false;
}
