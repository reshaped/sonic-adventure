#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* 4 bytes the object holds that no code reads. their type is unknown */
static Uint8 dat_8c6880b4[4] = {
#include "shinobi/sd_midi_get_total_beat_time/dat_8c6880b4.inc"
};

SDE_ERR sdMidiGetTotalBeatTime(SDMIDI handle, Uint32 *cur_beat_time)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint32 address;
    Uint32 value;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    /* @hack spelling: the address through a local, which SHC computes in R0 and
     * copies over the handle in R4 (MOV R0,R4 at 0x8c67853e). passed directly,
     * SHC keeps the handle on the stack, the function runs 8 bytes longer, and
     * 42 of 43 words differ */
    address = SD_MIDI_STATUS(sdgWrkPtr, handle, total);
    sdSndMemReadUint32(address, &value);
    *cur_beat_time = value;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiGetTotalBeatTime, err);
    }
    return err;
}
