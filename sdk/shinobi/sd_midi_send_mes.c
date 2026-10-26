#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_MIDI_MESSAGE 0x86

SDE_ERR sdMidiSendMes(SDMIDI handle, const SDS_MIDI_MES *midi_mes_ptr,
                      const Sint8 priority)
{
    static struct sd_host_command message_command;
    struct sd_port *port = (struct sd_port *)handle;
    SDE_ERR err;

    /* @hack redundant: the else after the goto, which loads HOST_MIDI_MESSAGE
     * into R3 (MOV.W at 0x8c618a54). without it, R2 and R3 trade and 14 of 43
     * words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (port == NULL) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    message_command.command = HOST_MIDI_MESSAGE;
    message_command.params[2] = midi_mes_ptr->m_Member[0];
    message_command.params[3] = midi_mes_ptr->m_Member[1];
    message_command.params[4] = midi_mes_ptr->m_Member[2];
    message_command.params[5] = midi_mes_ptr->m_Member[3];
    err = sdSetHostCmd(&message_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiSendMes, err);
    }
    return err;
}
