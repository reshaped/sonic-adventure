#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_FX_PROGRAM 0x84
#define HOST_FX_OUTPUT 0x82

SDE_ERR sdSndSetFxPrg(const Sint8 new_fx_prg_num, const Sint8 new_fx_out_num)
{
    /* @hack spelling: command, static in the function, whose params SHC
     * addresses through a pool word of their own (MOV.L at 0x8c6186b8). at file
     * scope, SHC addresses them from the command's address, the function runs
     * 4 bytes short and 48 of 52 words differ */
    static struct sd_host_command command;
    SDE_ERR err;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (new_fx_prg_num < 0 || new_fx_out_num < 0) {
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }
    command.command = HOST_FX_PROGRAM;
    command.params[0] = new_fx_prg_num;
    err = sdSetHostCmd(&command);
    if (err != SDE_ERR_NOTHING) {
        goto error;
    }
    command.command = HOST_FX_OUTPUT;
    command.params[0] = new_fx_out_num;
    err = sdSetHostCmd(&command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdSndSetFxPrg, err);
    }
    return err;
}
