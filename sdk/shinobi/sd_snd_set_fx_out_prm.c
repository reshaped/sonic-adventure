#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_FX_OUT_PRM 0x83
#define FX_OUT_MAX 15
#define LEVEL_CENTER 0x80
#define FINE_LEVEL_VERSION 39

SDE_ERR sdSndSetFxOutPrm(const Sint8 out_num, const Sint8 out_pan,
                         const Sint8 out_lev)
{
    /* @hack spelling: fx_out_command, static in the function, whose params SHC
     * addresses through a pool word of their own (MOV.L at 0x8c618630). at file
     * scope, SHC addresses them from the command's address, the function runs
     * 4 bytes short and 39 of 57 words differ */
    static struct sd_host_command fx_out_command;
    SDE_ERR err;
    Sint32 level;
    Sint32 pan;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (out_num < 0 || out_num > FX_OUT_MAX) {
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }
    level = out_lev; /* NOLINT(bugprone-signed-char-misuse) */
    pan = out_pan; /* NOLINT(bugprone-signed-char-misuse) */
    if (sdgWrkPtr->drv_ver.m_DebugVer < FINE_LEVEL_VERSION) {
        level >>= 3;
    }
    fx_out_command.command = HOST_FX_OUT_PRM;
    fx_out_command.params[0] = out_num;
    fx_out_command.params[1] = level + LEVEL_CENTER;
    fx_out_command.params[2] = pan + LEVEL_CENTER;
    err = sdSetHostCmd(&fx_out_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdSndSetFxOutPrm, err);
    }
    return err;
}
