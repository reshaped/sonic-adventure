#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* a stereo channel's offset from half the port's pan */
#define STEREO_PAN 64

SDE_ERR sdPstmGetPan(SDPSTM handle, const Sint8 target_ch, Sint8 *pan)
{
    struct sd_work *work = sdgWrkPtr;
    SDE_ERR err = SDE_ERR_NOTHING;
    struct sd_stream *stream;
    Sint8 number;
    Sint8 value;
    Sint32 pan_lev;

    if (work == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    if (pan == NULL) {
        err = SDE_ERR_PTR_NULL;
        goto error;
    }
    number = ((struct sd_port *)handle)->number;
    stream = &work->streams[number];
    sdSndMemReadSint8(SD_PSTM_PORT_STATUS(work, number, target_ch, pan),
                      &value);
    pan_lev = value; /* NOLINT(bugprone-signed-char-misuse) */
    switch (stream->use_ch) {
    case 1:
        break;
    case 2:
        switch (target_ch) {
        case SDD_PSTM_LEFT_CH:
            pan_lev += STEREO_PAN;
            break;
        case SDD_PSTM_RIGHT_CH:
            if (pan_lev == 0) {
                pan_lev = -1;
            }
            pan_lev -= STEREO_PAN;
            break;
        }
        pan_lev *= 2;
        break;
    }
    *pan = pan_lev;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmGetPan, err);
    }
    return err;
}
