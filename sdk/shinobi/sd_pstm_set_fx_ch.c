#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_PSTM_FX_CH 0x29
#define LEVEL_CENTER 0x80

SDE_ERR sdPstmSetFxCh(SDPSTM handle, const Sint8 fx_in_ch,
                      const Sint8 fx_base_lev)
{
    /* @hack spelling: stream_fx_command, static in the function, whose params
     * SHC addresses through a pool word of their own (MOV.L at 0x8c6772f8). at
     * file scope, SHC addresses them from the command's address, the function
     * runs 8 bytes short and 26 of 41 words differ */
    static struct sd_host_command stream_fx_command;
    SDE_ERR err;
    Sint32 level;

    /* @hack redundant: the else after the first goto, which puts LEVEL_CENTER
     * in R3 (MOV.W at 0x8c6772e8). without it, R2 and R3 trade and 9 of 41
     * words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    level = fx_base_lev + LEVEL_CENTER;
    stream_fx_command.command = HOST_PSTM_FX_CH;
    stream_fx_command.params[0] = ((struct sd_port *)handle)->number;
    stream_fx_command.params[1] = fx_in_ch;
    stream_fx_command.params[2] = level;
    err = sdSetHostCmd(&stream_fx_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmSetFxCh, err);
    }
    return err;
}
