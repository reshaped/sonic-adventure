#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_MIDI_VOLUME 0x05
#define VOL_CENTER 0x80

SDE_ERR sdMidiSetVol(SDMIDI handle, const Sint8 vol, const Sint32 fade_time)
{
    static struct sd_host_command volume_command;
    struct sd_port *port = (struct sd_port *)handle;
    SDE_ERR err;
    Sint32 level;

    /* @hack redundant: the else after the goto, which loads VOL_CENTER into R3
     * (MOV.W at 0x8c618c3c). without it, R2 and R3 trade and 10 of 45 words
     * differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (port == NULL) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    /* @hack spelling: the level through a local, which SHC computes before
     * the parameter stores (EXTS.B R5,R5 and ADD R3,R5 at 0x8c618c42 and
     * 0x8c618c46). in its store, the function runs 8 bytes short and 32 of 45
     * words differ */
    level = vol + VOL_CENTER;
    volume_command.command = HOST_MIDI_VOLUME;
    volume_command.params[0] = port->number;
    volume_command.params[1] = level;
    volume_command.params[2] = fade_time;
    volume_command.params[3] = fade_time >> 8;
    err = sdSetHostCmd(&volume_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiSetVol, err);
    }
    return err;
}
