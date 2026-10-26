#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdPstmSetUseCh(SDPSTM handle, const Sint8 use_ch)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    struct sd_stream *stream;
    Sint8 number;
    Sint32 i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
    } else {
        /* @hack type: use_ch widened through count, which SHC keeps in
         * R12 (EXTS.B R5,R12 at 0x8c6775a6). tested directly, R12 and R13
         * trade and 14 of 79 words differ */
        Sint32 count = use_ch; /* NOLINT(bugprone-signed-char-misuse) */

        /* @hack spelling: the upper bound tested apart, with a goto of its own,
         * which puts SDD_PSTM_PORT_CH_MAX in R1 (MOV #2,R1 at 0x8c6775ae).
         * tested with the lower bound, it goes to R0 and 2 of 79 words
         * differ */
        if (count < 1) {
            goto over_range;
        } else if (count > SDD_PSTM_PORT_CH_MAX) {
            goto over_range;
        } else {
            number = ((struct sd_port *)handle)->number;
            stream = &sdgWrkPtr->streams[number];
            if (use_ch < 0 || stream->ch_num < use_ch) {
            over_range:
                err = SDE_ERR_PRM_OVER_RANGE;
            } else {
                stream->use_ch = use_ch;
                /* @hack spelling: an if around a do, whose test SHC places
                 * ahead of sdPstmSetPan's address (CMP/PL R12 at 0x8c6775da,
                 * MOV.L at 0x8c6775e0). as a for, the load comes first and 4 of
                 * 79 words differ */
                i = 0;
                if (i < use_ch) {
                    do {
                        sdPstmSetPan(handle, i, stream->pan[i], 0);
                        i++;
                    } while (i < use_ch);
                }
            }
        }
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPstmSetUseCh, err);
    }
    return err;
}
