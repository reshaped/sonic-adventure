/* MPEG video picture decoding and skipping */
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mpv.h"

/* the errors MPV_DecodeFrm and MPV_SkipFrm return for a decoder not handed
 * out, and MPV_SkipFrm's for data with no start code after the picture */
#define MPV_ERR_DECODEFRM_HN ((Sint32)0xff030209)
#define MPV_ERR_SKIPFRM_HN ((Sint32)0xff03020a)
#define MPV_ERR_SKIPFRM_DELIM ((Sint32)0xff030305)

/* the start codes that can follow a picture: the next picture, a group, a
 * sequence, or the sequence end */
#define PICTURE_END                                                            \
    (MPV_DELIM_PICTURE | MPV_DELIM_GOP | MPV_DELIM_SEQUENCE |                  \
     MPV_DELIM_SEQUENCE_END)

Sint32 MPV_DecodeFrm(struct mpv_obj *mpv, struct mpv_stream *stm, Sint32 *used,
                     struct mpv_request *request)
{
    /* @hack spelling: a copy of mpv, which SHC sets in MPVLIB_CheckHn's
     * delay slot, MOV R4,R14 at 0x8ced55c4; through mpv, the function runs 2
     * bytes long and 110 of its 117 words differ */
    struct mpv_obj *hn = mpv;
    /* UTY_PushFr's save of FR12 to FR15 and the second bank, which the decoder
     * changes */
    /* @hack type: the words in a struct, not an array, which keeps the other
     * locals in their frame slots, used at sp+4 and data at sp+16 (0x8ced55c0,
     * 0x8ced55e4); as Sint32 fr[21], 12 of 117 words differ */
    struct {
        Sint32 words[21];
    } fr;
    struct mpv_err_info *err;
    Uint8 *data;
    Sint32 size;
    Sint32 misses;
    Sint32 recoveries;
    Sint32 result;

    if (MPVLIB_CheckHn(mpv) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_DECODEFRM_HN);
    }
    UTY_PushFr(&fr);
    data = stm->data;
    err = &hn->err;
    size = stm->size;
    misses = err->misses;
    recoveries = err->recoveries;
    hn->request = *request;
    MPVCMC_InitMcOiRt(hn);
    result = MPVDEC_DecPicture(hn, data, size, used);
    *request->picture = hn->picture;
    request->misses = err->misses - misses;
    request->recoveries = err->recoveries - recoveries;
    UTY_PopFr(&fr);
    return result;
}

Sint32 MPV_SkipFrm(struct mpv_obj *mpv, struct mpv_stream *stm, Sint32 *used)
{
    /* @hack spelling: a copy of mpv, which SHC keeps in R14 across
     * MPVLIB_CheckHn, MOV R4,R14 at 0x8ced56ac; through mpv, which then goes
     * to the stack, the function runs 2 bytes short and the PSG 4, and 24 of
     * its 44 words differ */
    struct mpv_obj *hn = mpv;
    Uint8 *data;
    Sint32 size;
    Uint8 *end;
    Sint32 code;

    if (MPVLIB_CheckHn(hn) != 0) {
        return MPVERR_SetCode(NULL, MPV_ERR_SKIPFRM_HN);
    }
    data = stm->data;
    size = stm->size;
    end = MPV_SearchDelim(data, size, PICTURE_END);
    if (end == NULL) {
        *used = size;
        code = MPV_ERR_SKIPFRM_DELIM;
    } else {
        *used = end - data;
        code = 0;
    }
    return MPVERR_SetCode(hn, code);
}

void MPVFRM_Init(void)
{
}
