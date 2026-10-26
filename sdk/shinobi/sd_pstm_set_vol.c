#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_PSTM_VOL 0x25
#define VOL_CENTER 0x80

/* params[0]'s channel bits, by target_ch & 3: left, right, none, and both
 * for SDD_PSTM_ALL_CH */
static Uint8 ch_bits[4] = {0x20, 0x10, 0x00, 0x30};

SDE_ERR sdPstmSetVol(SDPSTM handle, const Sint8 target_ch, const Sint8 vol,
                     const Sint32 fade_time)
{
    /* @hack spelling: stream_volume_command, static in the function, whose
     * params SHC addresses through pool words of their own (MOV.L at
     * 0x8c677686). at file scope, SHC addresses them from the command's
     * address, the function runs 16 bytes short and 70 of 72 words differ */
    static struct sd_host_command stream_volume_command;
    SDE_ERR err;
    Sint8 number;
    Sint32 level;
    struct sd_work *work;

    work = sdgWrkPtr;
    /* @hack redundant: a store of NULL to work that never runs. as a use of
     * work, it keeps work in R14 (0x8c67763e); without it, work goes to the
     * frame, the function runs 12 bytes longer and 68 of 72 words differ */
    if (0) {
        work = NULL;
    }
    if (work == NULL) {
        err = SDE_ERR_NO_INIT;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
    } else {
        number = ((struct sd_port *)handle)->number;
        level = vol + VOL_CENTER;
        stream_volume_command.command = HOST_PSTM_VOL;
        stream_volume_command.params[0] = ch_bits[target_ch & 3] | number;
        if (target_ch == SDD_PSTM_LEFT_CH || target_ch == SDD_PSTM_ALL_CH) {
            stream_volume_command.params[1] = level;
            stream_volume_command.params[2] = fade_time;
            stream_volume_command.params[3] = fade_time >> 8;
        }
        if (target_ch == SDD_PSTM_RIGHT_CH || target_ch == SDD_PSTM_ALL_CH) {
            stream_volume_command.params[5] = level;
            stream_volume_command.params[6] = fade_time;
            stream_volume_command.params[7] = fade_time >> 8;
        }
        err = sdSetHostCmd(&stream_volume_command);
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmSetVol, err);
    }
    return err;
}
