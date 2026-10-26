#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMidiGetSpeed(SDMIDI handle, Sint16 *speed)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint32 address;
    Sint32 value;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    /* @hack spelling: the address through a local, which SHC computes in R0 and
     * copies over the handle in R4 (MOV R0,R4 at 0x8c6784ce). passed directly,
     * SHC keeps the handle on the stack, the function runs 8 bytes longer, and
     * 42 of 43 words differ */
    address = SD_MIDI_STATUS(sdgWrkPtr, handle, speed);
    sdSndMemReadSint32(address, &value);
    *speed = value;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiGetSpeed, err);
    }
    return err;
}
