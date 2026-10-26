#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* vol isn't tested for NULL */
SDE_ERR sdPstmGetVol(SDPSTM handle, const Sint8 target_ch, Sint8 *vol)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint32 address;
    Sint8 value;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    /* @hack spelling: the address through a local, which SHC computes in R0 and
     * copies over the handle in R4 (MOV R0,R4 at 0x8c678040). passed directly,
     * SHC keeps the handle and the channel on the stack, the function runs 12
     * bytes longer, and 47 of 50 words differ */
    address = SD_PSTM_STATUS(sdgWrkPtr, handle, target_ch, vol);
    sdSndMemReadSint8(address, &value);
    *vol = value;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmGetVol, err);
    }
    return err;
}
