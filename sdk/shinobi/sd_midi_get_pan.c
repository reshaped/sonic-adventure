#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMidiGetPan(SDMIDI handle, Sint8 *pan)
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
     * copies over the handle in R4 (MOV R0,R4 at 0x8c6783ee). passed directly,
     * SHC keeps the handle on the stack, the function runs 8 bytes longer, and
     * 42 of 43 words differ */
    address = SD_MIDI_STATUS(sdgWrkPtr, handle, pan);
    sdSndMemReadSint8(address, &value);
    *pan = value;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiGetPan, err);
    }
    return err;
}
