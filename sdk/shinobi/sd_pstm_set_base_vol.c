#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define BASE_VOL_MAX 127

SDE_ERR sdPstmSetBaseVol(SDPSTM handle, const Sint8 base_vol)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint8 number;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    /* @hack redundant: a second test of sdgWrkPtr, whose else never runs: the
     * first test has already gone to error for a NULL sdgWrkPtr. SHC tests it
     * again (TST R6,R6 at 0x8c6771fe) and keeps the else's store (0x8c67722e);
     * without the test and its else, the function runs 8 bytes short and 40
     * of 50 words differ */
    if (sdgWrkPtr != NULL) {
        number = ((const struct sd_port *)handle)->number;
        if (base_vol == BASE_VOL_MAX) {
            sdgWrkPtr->streams[number].base_vol = 0;
        } else {
            sdgWrkPtr->streams[number].base_vol = -1 - (2 * base_vol);
        }
    } else {
        err = SDE_ERR_NO_INIT;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmSetBaseVol, err);
    }
    return err;
}
