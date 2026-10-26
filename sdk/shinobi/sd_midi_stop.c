#include "shinobi/sd_work.h"
#include <shinobi.h>

#define HOST_MIDI_STOP 0x02

SDE_ERR sdMidiStop(SDMIDI handle)
{
    static struct sd_host_command stop_command;
    struct sd_port *port = (struct sd_port *)handle;
    SDE_ERR err;

    /* @hack redundant: the else after the goto, which loads stop_command's
     * address into R2 (0x8c618cbc). without it, R2 and R3 trade and 8 of 34
     * words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (port == NULL) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    stop_command.command = HOST_MIDI_STOP;
    stop_command.params[0] = port->number;
    err = sdSetHostCmd(&stop_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiStop, err);
    }
    return err;
}
