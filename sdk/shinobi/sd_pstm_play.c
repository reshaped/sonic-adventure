#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_PSTM_PLAY 0x21
#define PLAY_MONO 0x00
#define PLAY_STEREO 0x80
#define PLAY_16BIT_PCM 0x00
#define PLAY_8BIT_PCM 0x10
#define PLAY_4BIT_ADPCM 0x30
#define PRIORITY_MAX 0x0f
#define PRIORITY_SHIFT 3

SDE_ERR sdPstmPlay(SDPSTM handle, const SDE_PCM_TYPE pcm_type,
                   const Uint16 freq, const Sint8 priority)
{
    /* @hack spelling: command, static in the function, whose params SHC
     * addresses through a pool word of their own (MOV.L at 0x8c6780f0). at file
     * scope, SHC addresses them from the command's address, the function runs
     * 12 bytes short and 102 of 110 words differ */
    static struct sd_host_command command;
    SDE_ERR err;
    struct sd_stream *stream;
    Sint8 number;
    Sint32 channels;
    Sint32 samples;
    Sint32 i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    /* @hack type: PRIORITY_MAX cast to Uint32, with which SHC extends
     * priority into R2 for the test (EXTS.B R7,R2 at 0x8c67809e). as an int
     * mask, priority goes to R13, the function runs 2 bytes short and 61 of
     * 110 words differ */
    if (priority & ~(Uint32)PRIORITY_MAX) {
        goto over_range;
    }
    number = ((const struct sd_port *)handle)->number;
    stream = &sdgWrkPtr->streams[number];
    stream->pcm_type = pcm_type;
    switch (stream->use_ch) {
    case 1:
        channels = PLAY_MONO;
        break;
    case 2:
        channels = PLAY_STEREO;
        break;
    default:
        goto over_range;
        /* @hack redundant: a break after the goto, which never runs, and with
         * which SHC zeroes stopped through R3 (MOV #0,R3 at 0x8c6780e2).
         * without it, R2 and R3 trade and 14 of 110 words differ */
        break;
    }
    switch (stream->pcm_type) {
    case SDE_PCM_TYPE_16BIT_PCM:
        samples = PLAY_16BIT_PCM;
        break;
    case SDE_PCM_TYPE_8BIT_PCM:
        samples = PLAY_8BIT_PCM;
        break;
    case SDE_PCM_TYPE_4BIT_ADPCM:
        samples = PLAY_4BIT_ADPCM;
        break;
    default:
    over_range:
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }
    stream->stopped[0] = false;
    stream->stopped[1] = false;
    command.command = HOST_PSTM_PLAY;
    command.params[0] = number | channels | samples;
    command.params[1] = stream->base_vol;
    command.params[2] = freq;
    command.params[3] = freq >> 8;
    command.params[4] = priority << PRIORITY_SHIFT;
    for (i = 0; i < SDD_PSTM_PORT_CH_MAX; i++) {
        command.params[5 + i] = stream->ch[i];
    }
    err = sdSetHostCmd(&command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmPlay, err);
    }
    return err;
}
