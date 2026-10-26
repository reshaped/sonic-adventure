#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

void *sdgPrimaryBufPtr;

SDE_ERR sdPrimaryHostCmdInitList(void *buffer)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else {
        sdgWrkPtr->host.primary = buffer;
        sdgPrimaryBufPtr = buffer;
        memset(buffer, 0,
               SD_PRIMARY_COMMAND_MAX * sizeof(struct sd_primary_command));
        sdgWrkPtr->host.busy = false;
        /* @hack spelling: err assigned in the test, which tests
         * sdMemBlkCreate's R0 (TST R0,R0 at 0x8c670e8c). assigned ahead of it,
         * the function runs 4 bytes short and 37 of 40 words differ */
        /* @hack redundant: the test's goto to the next label, after which SHC
         * tests err again (TST R14,R14 at 0x8c670e8e). with neither, as a call
         * alone, 16 of 40 words differ */
        if ((err = sdMemBlkCreate(&sdgWrkPtr->primary_block)) !=
            SDE_ERR_NOTHING) {
            goto error;
        }
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPrimaryHostCmdInitList, err);
    }
    return err;
}
