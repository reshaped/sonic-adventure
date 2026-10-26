#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_MIDI_PLAY 0x01
#define PRIORITY_MAX 15
#define PRIORITY_SHIFT 3

SDE_ERR sdMidiPlay(SDMIDI handle, const Sint8 bank_num, const Sint8 data_num,
                   const Sint8 priority)
{
    static struct sd_host_command play_command;
    struct sd_port *port = (struct sd_port *)handle;
    SDE_ERR err;

    /* @hack redundant: the else after the goto, which puts PRIORITY_MAX in R2
     * for the priority's test (MOV #15,R2 at 0x8c6189c6). without it, R2 and R3
     * trade and 17 of 53 words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (port == NULL) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    if (priority < 0 || priority > PRIORITY_MAX) {
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }
    play_command.command = HOST_MIDI_PLAY;
    play_command.params[0] = port->number;
    play_command.params[1] = bank_num;
    play_command.params[2] = data_num;
    play_command.params[3] = priority << PRIORITY_SHIFT;
    err = sdSetHostCmd(&play_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiPlay, err);
    }
    return err;
}
