#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_PSTM_DRCT_LEV 0x2b
#define LEVEL_CENTER 0x80

SDE_ERR sdPstmSetDrctLev(SDPSTM handle, const Sint8 drct_lev)
{
    /* @hack spelling: stream_direct_level_command, static in the function,
     * whose params SHC addresses through a pool word of their own (MOV.L at
     * 0x8c677286). at file scope, SHC addresses them from the command's
     * address, the function runs 4 bytes short and 24 of 39 words differ */
    static struct sd_host_command stream_direct_level_command;
    SDE_ERR err;
    Sint32 level;

    /* @hack redundant: the else after the first goto, which puts LEVEL_CENTER
     * in R3 (MOV.W at 0x8c677278). without it, R2 and R3 trade and 6 of 39
     * words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    level = drct_lev + LEVEL_CENTER;
    stream_direct_level_command.command = HOST_PSTM_DRCT_LEV;
    stream_direct_level_command.params[0] = ((struct sd_port *)handle)->number;
    stream_direct_level_command.params[1] = level;
    err = sdSetHostCmd(&stream_direct_level_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmSetDrctLev, err);
    }
    return err;
}
