#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_PSTM_PAN 0x26
#define PAN_CENTER 0x80

/* params[0]'s channel bits, by target_ch & 3: left, right, none, and both
 * for SDD_PSTM_ALL_CH */
static Uint8 pan_ch_bits[4] = {0x20, 0x10, 0x00, 0x30};

/* for SDD_PSTM_ALL_CH, -1, pan[target_ch] is the entry before pan[0], the last
 * of unk_14 */
SDE_ERR sdPstmSetPan(SDPSTM handle, const Sint8 target_ch, const Sint8 pan,
                     const Sint32 fade_time)
{
    /* @hack spelling: stream_pan_command, static in the function, whose params
     * SHC addresses through a pool word of their own (MOV.L at 0x8c677410). at
     * file scope, SHC addresses them from the command's address, the function
     * runs 36 bytes short and 71 of 165 words differ */
    static struct sd_host_command stream_pan_command;
    SDE_ERR err;
    struct sd_stream *stream;
    Sint32 pans[SDD_PSTM_PORT_CH_MAX]; /* each as the command sends it */
    Sint32 i;
    Sint8 number;
    /* @hack spelling: a local zero for the stores below, which keeps number
     * in R11 and target_ch in R12 (MOV R0,R11 at 0x8c677370, EXTS.B R5,R12 at
     * 0x8c6773a4). with 0 in each store, the two trade and 7 of 165 words
     * differ */
    Uint8 zero;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    number = ((struct sd_port *)handle)->number;
    stream = &sdgWrkPtr->streams[number];
    /* @hack type: pan stored through a byte cursor, at stream plus pan's
     * offset plus target_ch's, which also keeps number in R11 and target_ch in
     * R12 (0x8c677370, 0x8c6773a4). as stream->pan[target_ch], the two trade
     * and 7 of 165 words differ */
    *(Sint32 *)((Uint8 *)stream + (((Uint8 *)stream->pan - (Uint8 *)stream) +
                                   (target_ch * sizeof(stream->pan[0])))) =
        pan; /* NOLINT(bugprone-signed-char-misuse) */
    for (i = 0; i < stream->use_ch; i++) {
        pans[i] = pan; /* NOLINT(bugprone-signed-char-misuse) */
    }
    switch (stream->use_ch) {
    case 1:
        break;
    case 2:
        switch (target_ch) {
        case SDD_PSTM_LEFT_CH:
            pans[0] += PAN_CENTER;
            break;
        case SDD_PSTM_RIGHT_CH:
            pans[1] -= PAN_CENTER;
            if (pans[1] >= -1) {
                pans[1] = -2;
            }
            break;
        }
        for (i = 0; i < stream->use_ch; i++) {
            pans[i] /= 2;
        }
        break;
    }
    for (i = 0; i < stream->use_ch; i++) {
        pans[i] += PAN_CENTER;
    }
    stream_pan_command.command = HOST_PSTM_PAN;
    stream_pan_command.params[0] = pan_ch_bits[target_ch & 3] | number;
    zero = 0;
    switch (target_ch) {
    case SDD_PSTM_LEFT_CH:
        stream_pan_command.params[1] = pans[0];
        stream_pan_command.params[2] = fade_time;
        stream_pan_command.params[3] = fade_time >> 8;
        stream_pan_command.params[5] = zero;
        stream_pan_command.params[6] = zero;
        stream_pan_command.params[7] = zero;
        break;
    case SDD_PSTM_RIGHT_CH:
        stream_pan_command.params[1] = zero;
        stream_pan_command.params[2] = zero;
        stream_pan_command.params[3] = zero;
        /* @bug uninitialized: on a port of one channel, nothing writes
         * pans[1], and the command sends the low byte of whatever its frame
         * slot holds (MOV.L @(4,R4),R3 at 0x8c67747e, MOV.B R3,@R1 at
         * 0x8c677482) */
#ifdef GCC
        /* repairs the unset pan the tag names: on a port of one channel,
         * GCC sends pans[0], the pan of its one channel */
        stream_pan_command.params[5] = stream->use_ch == 1 ? pans[0] : pans[1];
#else
        stream_pan_command.params[5] = pans[1];
#endif
        stream_pan_command.params[6] = fade_time;
        stream_pan_command.params[7] = fade_time >> 8;
        break;
    }
    err = sdSetHostCmd(&stream_pan_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmSetPan, err);
    }
    return err;
}
