/* MPEG system stream start code classification */
#include <sg_xpt.h>

#include "movie/sdk/mps.h"

Sint32 MPSDEL_CheckDelim(Uint8 *buf)
{
    Uint8 *p = buf;
    /* @hack redundant: register on code, which loads MPS_DLM_PKET into R5 at
     * 0x8cedc928, ahead of the tests; without it, 41 of 57 words differ */
    register Sint32 code;

    code = *p++;
    code <<= 8;
    code |= *p++;
    code <<= 8;
    code |= *p++;
    code <<= 8;
    code |= *p;
    switch (code) {
    case 0x1ba:
        code = MPS_DLM_PACK;
        break;
    case 0x1bb:
        code = MPS_DLM_SYS;
        break;
    case 0x1b9:
        code = MPS_DLM_END;
        break;
    case 0x1bd:
    case 0x1be:
    case 0x1bf:
        code = MPS_DLM_PKET;
        break;
    default:
        /* the packet of an audio stream, 0x1c0 to 0x1df, or of a video stream,
         * 0x1e0 to 0x1ef */
        if (code >= 0x1c0 && code <= 0x1ef) {
            code = MPS_DLM_PKET;
        } else {
            code = MPS_DLM_NONE;
        }
        break;
    }
    return code;
}
