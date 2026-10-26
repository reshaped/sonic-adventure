#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdarg.h>

/* the highest driver channel a stream port can use */
#define CH_MAX 15

SDE_ERR sdPstmOpenPort(SDPSTM *handle, const Sint8 use_ch, ...)
{
    SDE_ERR err;
    va_list args;
    Sint8 ch[SDD_PSTM_PORT_CH_MAX];
    struct sd_stream *stream;
    Sint8 number;
    int i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (use_ch < 1 || use_ch > SDD_PSTM_PORT_CH_MAX) {
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }
    va_start(args, use_ch);
    for (i = 0; i < use_ch; i++) {
        ch[i] = va_arg(args, int);
        if (ch[i] < 0 || ch[i] > CH_MAX) {
            /* @hack spelling: va_end, an empty statement here, ahead of the
             * error code: the store and goto after it match the use_ch test's,
             * and SHC keeps one copy for both (MOV.L at 0x8c677040, which the
             * branches at 0x8c676fe2, 0x8c676fe8 and 0x8c677038 reach); placed
             * between them, it gives the use_ch test its own copy, the
             * function is 8 bytes longer and 143 of 162 words differ */
            va_end(args);
            err = SDE_ERR_PRM_OVER_RANGE;
            goto error;
        }
    }
    va_end(args);
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    for (i = use_ch; i < SDD_PSTM_PORT_CH_MAX; i++) {
        ch[i] = -1;
    }
    err = sdModuleOpenPort(handle, SD_PORT_PSTM);
    if (err != SDE_ERR_NOTHING) {
        goto error;
    }
    number = ((struct sd_port *)*handle)->number;
    stream = &sdgWrkPtr->streams[number];
    stream->pcm_type = SDE_PCM_TYPE_16BIT_PCM;
    stream->ch_num = use_ch;
    sdPstmSetUseCh(*handle, use_ch);
    sdPstmSetBaseVol(*handle, 0);
    for (i = 0; i < use_ch; i++) {
        stream->ch[i] = ch[i];
        stream->stopped[i] = true;
        stream->unk_14[i] = 0;
        stream->unk_24[i] = 0;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmOpenPort, err);
    }
    return err;
}
