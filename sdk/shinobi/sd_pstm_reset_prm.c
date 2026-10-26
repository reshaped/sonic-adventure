#include "shinobi/sd_work.h"
#include <shinobi.h>

#define HOST_PSTM_RESET_PRM 0x2e

SDE_ERR sdPstmResetPrm(SDPSTM handle)
{
    /* @hack spelling: stream_reset_command, static in the function, whose
     * params SHC addresses through a pool word of their own (MOV.L at
     * 0x8c67719c). at file scope, SHC addresses them from the command's
     * address, the function runs 4 bytes short and 20 of 34 words differ */
    static struct sd_host_command stream_reset_command;
    SDE_ERR err;

    /* @hack redundant: the else after the first goto, which puts
     * stream_reset_command's address in R2 (MOV.L at 0x8c677198). without it,
     * R2 and R3 trade and 8 of 34 words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    stream_reset_command.command = HOST_PSTM_RESET_PRM;
    stream_reset_command.params[0] = ((struct sd_port *)handle)->number;
    err = sdSetHostCmd(&stream_reset_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmResetPrm, err);
    }
    return err;
}
