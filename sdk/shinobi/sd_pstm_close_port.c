#include "shinobi/sd_work.h"
#include <shinobi.h>

SDE_ERR sdPstmClosePort(SDPSTM handle)
{
    SDE_ERR err;

    /* @hack redundant: the else after the first goto, which has SHC call
     * sdModuleClosePort through R3 (0x8c676d00). without it, SHC calls
     * sdModuleClosePort through R2, and 4 of 28 words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    err = sdModuleClosePort((struct sd_port *)handle);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmClosePort, err);
    }
    return err;
}
