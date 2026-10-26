#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMidiGetDrctLev(SDMIDI handle, Sint8 *drct_lev)
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
     * copies over the handle in R4 (MOV R0,R4 at 0x8c67822e). passed directly,
     * SHC keeps the handle on the stack, the function runs 8 bytes longer, and
     * 42 of 43 words differ */
    address = SD_MIDI_STATUS(sdgWrkPtr, handle, drct_lev);
    sdSndMemReadSint8(address, &value);
    *drct_lev = value;

error:
    if (err != SDE_ERR_NOTHING) {
        /* @bug logic: the trap names sdMidiGetStat, not this function: the
         * pool word at 0x8c678264 holds sdMidiGetStat's address */
        sdSysErrTrap((void *)sdMidiGetStat, err);
    }
    return err;
}
