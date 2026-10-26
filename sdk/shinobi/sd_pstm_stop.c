#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_PSTM_STOP 0x22

SDE_ERR sdPstmStop(SDPSTM handle)
{
    /* @hack spelling: command, static in the function, whose params SHC
     * addresses through a pool word of their own (MOV.L at 0x8c67776c). at file
     * scope, SHC addresses them from the command's address, the function runs
     * 4 bytes short and 23 of 71 words differ */
    static struct sd_host_command command;
    SDE_ERR err;
    struct sd_stream *stream;
    Sint8 number;
    Sint8 ch_num;
    int i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    number = ((const struct sd_port *)handle)->number;
    stream = &sdgWrkPtr->streams[number];
    ch_num = stream->ch_num;
    for (i = 0; i < ch_num; i++) {
        stream->stopped[i] = true;
        stream->unk_14[i] = 0;
        stream->unk_24[i] = 0;
    }
    command.command = HOST_PSTM_STOP;
    command.params[0] = number;
    err = sdSetHostCmd(&command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmStop, err);
    }
    return err;
}
