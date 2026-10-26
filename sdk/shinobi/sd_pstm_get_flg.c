#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdPstmGetFlg(SDPSTM handle, const Sint8 target_ch, Uint32 *flg)
{
    struct sd_work *work = sdgWrkPtr;
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint32 address;
    Uint8 value;

    if (work == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    if (flg == NULL) {
        err = SDE_ERR_PTR_NULL;
        goto error;
    }
    /* @hack spelling: the address through a local, which SHC computes in R0 and
     * copies over the handle in R4 (MOV R0,R4 at 0x8c676e78). passed directly,
     * SHC keeps the handle and the channel on the stack, the function runs 12
     * bytes longer, and 53 of 56 words differ */
    address = SD_PSTM_STATUS(work, handle, target_ch, flg);
    sdSndMemReadUint8(address, &value);
    *flg = value;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmGetFlg, err);
    }
    return err;
}
