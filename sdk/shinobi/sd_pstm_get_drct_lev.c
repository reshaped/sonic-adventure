#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* each error is returned without going to sdSysErrTrap */
SDE_ERR sdPstmGetDrctLev(SDPSTM handle, const Sint8 target_ch, Sint8 *drct_lev)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint32 address;
    Sint8 value;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    /* @hack redundant: the else after the goto, which has SHC test drct_lev in
     * R2 (0x8c677bfe). without it, SHC tests drct_lev in R3, and 2 of 50 words
     * differ */
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    } else if (drct_lev == NULL) {
        err = SDE_ERR_PTR_NULL;
        goto error;
    }
    /* @hack spelling: the address through a local, which SHC computes in R0 and
     * copies over the handle in R4 (MOV R0,R4 at 0x8c677c28). passed directly,
     * SHC keeps the handle and the channel on the stack, the function runs 12
     * bytes longer, and 47 of 50 words differ */
    address = SD_PSTM_STATUS(sdgWrkPtr, handle, target_ch, drct_lev);
    sdSndMemReadSint8(address, &value);
    *drct_lev = value;

error:
    return err;
}
