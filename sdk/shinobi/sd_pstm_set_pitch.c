#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_PSTM_PITCH 0x28
#define PITCH_MAX 6399
#define PITCH_CENTER 0x8000

SDE_ERR sdPstmSetPitch(SDPSTM handle, const Sint16 pitch,
                       const Sint32 fade_time)
{
    /* @hack spelling: command, static in the function, whose params SHC
     * addresses through pool words of their own (MOV.L at 0x8c67750c). at file
     * scope, SHC addresses them from the command's address, the function runs
     * 16 bytes short and 41 of 58 words differ */
    static struct sd_host_command command;
    SDE_ERR err;
    Sint32 value;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    if (pitch < -PITCH_MAX || pitch > PITCH_MAX) {
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }
    /* @hack spelling: the sum through value, which SHC keeps in R5 (ADD R14,R5
     * at 0x8c67750a). summed in each store, the function runs 4 bytes longer
     * and 21 of 58 words differ */
    value = pitch + PITCH_CENTER;
    command.command = HOST_PSTM_PITCH;
    command.params[0] = ((const struct sd_port *)handle)->number;
    command.params[2] = value;
    command.params[3] = value >> 8;
    command.params[4] = fade_time;
    command.params[5] = fade_time >> 8;
    err = sdSetHostCmd(&command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmSetPitch, err);
    }
    return err;
}
