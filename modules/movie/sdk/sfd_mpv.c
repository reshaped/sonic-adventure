/* the Sofdec video decoder stage. it decodes the MPEG video in its input ring
 * into pictures, in the frames SFD_SetMpvPara set. it hands the pictures to
 * the video output in time code order */
#include <machine.h>
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mpv.h"
#include "movie/sdk/sfd.h"

/* the number of decoders MPV_Init lays out in sfmpv_work. MPV_Create hands
 * out no more than this many at once */
#define SFMPV_NMPV 8

/* the data up to the ring's end decodes whole once it holds this many bytes.
 * shorter data decodes only up to a start code that ends a picture */
#define SFMPV_RUN_SIZE 0x28000

/* the MPV_DELIM_* kinds of start code that can end a run */
#define SFMPV_DLM_MASK                                                         \
    (MPV_DELIM_PICTURE | MPV_DELIM_GOP | MPV_DELIM_SEQUENCE |                  \
     MPV_DELIM_SEQUENCE_END)

#define SFMPV_ERR_INIT ((Sint32)0xff000f01)
#define SFMPV_ERR_FLAP ((Sint32)0xff000f02)
#define SFMPV_ERR_NODLM ((Sint32)0xff000f03)
#define SFMPV_ERR_HEADER ((Sint32)0xff000f04)
#define SFMPV_ERR_PICATR ((Sint32)0xff000f05)
#define SFMPV_ERR_DECODE ((Sint32)0xff000f06)
#define SFMPV_ERR_SKIP ((Sint32)0xff000f07)
#define SFMPV_ERR_CREATE ((Sint32)0xff000f0a)
#define SFMPV_ERR_ERRFN ((Sint32)0xff000f0b)
#define SFMPV_ERR_DESTROY ((Sint32)0xff000f0c)
#define SFMPV_ERR_NOWRITE ((Sint32)0xff000f0d)
#define SFMPV_ERR_NOTOUT ((Sint32)0xff000f0e)
#define SFMPV_ERR_FRM ((Sint32)0xff000f0f)
#define SFMPV_ERR_COND ((Sint32)0xff000f12)
#define SFMPV_ERR_SECTION ((Sint32)0xff000f13)
#define SFMPV_ERR_PARA ((Sint32)0xff000f15)
#define SFMPV_ERR_BITRATE ((Sint32)0xff000f16)
#define SFMPV_ERR_RFBSIZE ((Sint32)0xff000f17)
#define SFMPV_ERR_FRMSIZE ((Sint32)0xff000f18)
#define SFMPV_ERR_NFRM ((Sint32)0xff000f19)

static Sint32 sfmpv_DecodeSomePic(struct sfd_handle *hn);
static Bool sfmpv_IsPrepEnd(struct sfd_handle *hn);
static Sint32 sfmpv_RingGetRead(struct sfd_handle *hn, char **data,
                                Sint32 *len);
static Sint32 sfmpv_CopyFlap(struct sfbuf_ring_inf *inf, Sint32 flapsize,
                             Sint32 *flapped, char **end);
static Sint32 sfmpv_SearchEndDlm(struct sfd_handle *hn,
                                 struct sfbuf_ring_inf *inf, char **end);
static Sint32 sfmpv_RingAddRead(struct sfd_handle *hn, Sint32 len);
static Sint32 sfmpv_DecodeOneUnit(struct sfd_handle *hn, char *data, Sint32 len,
                                  Sint32 *used, Bool *more);
static Bool sfmpv_IsTerm(struct sfd_handle *hn, Sint32 len, Sint32 dlm);
static Sint32 sfmpv_SetMvInf(struct sfd_handle *hn, struct mpv_obj *dec,
                             struct mpv_picture *picture);
static Sint32 sfmpv_ChkBufSiz(struct sfd_handle *hn, struct sfd_mvinf *mvinf);
static Bool sfmpv_IsSkip(struct sfd_handle *hn, struct mpv_picture *picture);
static Bool sfmpv_IsDefect(struct sfd_handle *hn, Sint32 type);
static void sfmpv_SetDefect(struct sfd_handle *hn, Sint32 type, Bool skipped);
static Bool sfmpv_IsPtypeSkip(struct sfd_handle *hn, Sint32 type);
static Bool sfmpv_IsLate(struct sfd_handle *hn, struct mpv_picture *picture,
                         Sint32 type);
static void sfmpv_GetDtime(struct sfd_handle *hn, Sint32 type, Sint32 *time,
                           Sint32 *tunit);
static Sint32 sfmpv_InitInf(struct sfd_handle *hn, struct sfmpv *mpv);
static void sfmpv_ErrFn(void *obj, Sint32 ecode);
static Sint32 sfmpv_DestroySub(struct mpv_obj *dec);
static Bool sfmpv_GetTermSrc(struct sfd_handle *hn);
static Bool sfmpv_GetTermDst(struct sfd_handle *hn);
static void sfmpv_SetTermDst(struct sfd_handle *hn, Bool flag);
static void sfmpv_ChkPrepFlg(struct sfd_handle *hn);
static void sfmpv_SetFrmInf(struct sfd_handle *hn, struct sfmpv_frm *frm,
                            struct sfd_frm **out);
static void sfmpv_PicAtr2Time(struct sfd_handle *hn,
                              struct mpv_picture *picture, Sint32 *time,
                              Sint32 *tunit);
static void sfmpv_InitRfb(struct sfd_handle *hn);
static Sint32 sfmpv_SetFrmPara(struct sfd_handle *hn,
                               struct mpv_picture *picture,
                               struct mpv_request *request,
                               struct sfmpv_frm **frm);
static void sfmpv_TermDec(struct sfd_handle *hn);
static Sint32 sfmpv_GetNumFrm(struct sfd_handle *hn);
static struct sfmpv_frm *sfmpv_AllocFrm(struct sfd_handle *hn);
static void sfmpv_FreeFrm(struct sfmpv_frm *frm);
static void sfmpv_StbyFrm(struct sfmpv_frm *frm);
static struct sfmpv_frm *sfmpv_HoldFrm(struct sfd_handle *hn);
static Bool sfmpv_IsChkFirst(struct sfmpv_frm *first, struct sfmpv_frm *frm);
static Bool sfmpv_IsBigTime(struct mpv_picture *a, struct mpv_picture *b);

/* the work MPV_Init lays its tables and decoders out in */
static Uint8 sfmpv_work[81920];
/* the frames SFD_SetMpvPara sets, which each handle's stage copies when it is
 * created */
static struct sfmpv_para sfmpv_para;

/* set the frames that handles created after the call decode into. rfb and frm
 * are each rounded up to a 32-byte boundary */
void SFD_SetMpvPara(struct sfmpv_para *para)
{
    /* @hack spelling: each field stored through sfmpv_para's address, which SHC
     * loads from the pool twice and copies into R1 and R0 (0x8ced3058 to
     * 0x8ced3062, 0x8ced306c); as sfmpv_para's fields, 28 of 28 words
     * differ */
    (&sfmpv_para)->c_width = para->c_width;
    (&sfmpv_para)->c_height = para->c_height;
    (&sfmpv_para)->y_width = para->y_width;
    (&sfmpv_para)->y_height = para->y_height;
    (&sfmpv_para)->rfb = (Uint8 *)(((Sint32)para->rfb + 31) & ~31);
    (&sfmpv_para)->frm_width = para->frm_width;
    (&sfmpv_para)->frm_height = para->frm_height;
    (&sfmpv_para)->nfrm = para->nfrm;
    (&sfmpv_para)->frm = (Uint8 *)(((Sint32)para->frm + 31) & ~31);
}

Sint32 SFMPV_Init(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    void *work = sfmpv_work;
    Sint32 err;

    /* @hack redundant: a never-run test of work, which makes no code but puts
     * MPV_Init's address and MPV_ERR_SECTION_SEQ in R2, not R3 (0x8ced3094,
     * 0x8ced30a0); without it, 4 of 27 words differ */
    if (0) {
        if (work) {
        }
    }
    err = MPV_Init(SFMPV_NMPV, work);
    if (err != 0) {
        Sint32 code;

        if (err == MPV_ERR_SECTION_SEQ) {
            code = SFMPV_ERR_SECTION;
        } else {
            code = SFMPV_ERR_INIT;
        }
        return SFLIB_SetErr(NULL, code);
    }
    UTY_MemsetDword(&sfmpv_para, 0, sizeof(sfmpv_para) / sizeof(Uint32));
    return 0;
}

Sint32 SFMPV_Finish(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    MPV_Finish();
    return 0;
}

Sint32 SFMPV_ExecServer(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                        Sint32 arg3)
{
    Sint32 err;

    if (sfmpv_GetTermDst(hn) == TRUE) {
        return 0;
    }
    err = sfmpv_DecodeSomePic(hn);
    sfmpv_ChkPrepFlg(hn);
    return err;
}

/* decode unit after unit while sfmpv_DecodeOneUnit asks for more. it asks for
 * none after a decoded picture, so a call decodes one at most. returns 0 or an
 * error code */
static Sint32 sfmpv_DecodeSomePic(struct sfd_handle *hn)
{
    char *data;
    Sint32 len;
    Sint32 used;
    Bool more;
    Sint32 err;

#ifdef GCC
    /* GCC can't see that sfmpv_RingGetRead sets data and len wherever it
     * returns 0 */
    data = NULL;
    len = 0;
#endif
    for (;;) {
        if ((err = sfmpv_RingGetRead(hn, &data, &len)) != 0) {
            break;
        }
        err = sfmpv_DecodeOneUnit(hn, data, len, &used, &more);
        if (err != 0) {
            break;
        }
        if ((err = sfmpv_RingAddRead(hn, used)) != 0) {
            break;
        }
        if (more == FALSE) {
            break;
        }
    }
    return err;
}

/* TRUE once the decoded pictures number SFD_COND_PREP_PICS, or the handle's
 * prm.unk_2c[0] if that is fewer */
static Bool sfmpv_IsPrepEnd(struct sfd_handle *hn)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    /* @hack redundant: para, set and never read, which has SHC add its offset
     * (ADD #16,R5 at 0x8ced315e) to a register it then overwrites; without
     * it, 12 of 28 words differ */
    struct sfmpv_para *para = &mpv->para;
    struct sfd_create_param *prm = &hn->prm;
    Sint32 *cond = hn->cond;
    Sint32 limit = prm->unk_2c[0];
    Sint32 pics = cond[SFD_COND_PREP_PICS];
    Sint32 need = (limit < pics) ? limit : pics;
    Sint32 n = sfmpv_GetNumFrm(hn);

    if (n >= need) {
        return TRUE;
    }
    return FALSE;
}

/* set *data and *len to the run to decode next. the run is the data up to the
 * ring's end once that holds SFMPV_RUN_SIZE bytes. short of that, the run ends
 * past a start code that ends a picture, and is empty until one is found. the
 * flap is the data at the ring's start that sfmpv_CopyFlap copies on past its
 * end, so that a run can cross the end. sfmpv_CopyFlap looks for that code in
 * the flap. sfmpv_SearchEndDlm looks for it once the input has ended. the ring
 * keeps the ends they find and the flap's length between calls, through
 * SFBUF_RingGetDlm and SFBUF_RingSetDlm. returns 0 or an error code. an error
 * from sfmpv_SearchEndDlm is returned without SFLIB_SetErr */
static Sint32 sfmpv_RingGetRead(struct sfd_handle *hn, char **data, Sint32 *len)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfbuf_ring_inf inf;
    char *dlm;
    char *end;
    char *area;
    Sint32 flapped;
    Sint32 flapsize;
    Sint32 arealen;
    Sint32 ibuf = trn->ibuf;
    Sint32 err;

    err = SFBUF_RingGetRead(hn, ibuf, &inf);
    if (err) {
        return err;
    }
    SFBUF_RingGetDlm(hn, ibuf, &dlm, &flapped, &end, &flapsize);
    err = sfmpv_CopyFlap(&inf, flapsize, &flapped, &end);
    if (err) {
        return SFLIB_SetErr(hn, err);
    }
    /* @hack spelling: area and len read through inf's address, which SHC forms
     * in R13 and R4 (ADD #20,R13 at 0x8ced3224, ADD #20,R4 at 0x8ced3228); as
     * inf.area and inf.len, 65 of 111 words differ */
    area = (&inf)->area;
    arealen = (&inf)->len;
    if (arealen >= SFMPV_RUN_SIZE) {
        *len = arealen;
    } else if (end) {
        *len = end - area;
    } else {
        err = sfmpv_SearchEndDlm(hn, &inf, &dlm);
        if (err) {
            return err;
        }
        if (dlm) {
            *len = dlm - area;
        } else {
            *len = 0;
        }
    }
    *data = area;
    SFBUF_RingSetDlm(hn, ibuf, dlm, flapped, end);
    return 0;
}

/* extend the flap from its *flapped bytes up to flapsize at most. the copy
 * stops after a start code in SFMPV_DLM_MASK, and *end is set past that code
 * when it ends a picture. a sequence header or a sequence end always ends a
 * picture. a picture or group start ends a picture when the nearest code in
 * SFMPV_DLM_MASK before it is a picture start. nothing is copied once *end is
 * set, or once the flap holds all the wrapped data. returns 0, or
 * SFMPV_ERR_FLAP once flapsize bytes hold no start code */
static Sint32 sfmpv_CopyFlap(struct sfbuf_ring_inf *inf, Sint32 flapsize,
                             Sint32 *flapped, char **end)
{
    char *area;
    char *wrap;
    char *dst;
    char *src;
    char *stop;
    char *prev;
    Sint32 arealen;
    Sint32 wraplen;
    Sint32 n;
    Sint32 copied;
    Sint32 dlm;
    int len;

    if (*end != NULL) {
        return 0;
    }
    area = inf->area;
    arealen = inf->len;
    wrap = inf->wrap;
    wraplen = inf->wraplen;
    if (wraplen <= *flapped) {
        return 0;
    }
    n = (wraplen - *flapped < flapsize - *flapped) ? wraplen - *flapped
                                                   : flapsize - *flapped;
    src = wrap + *flapped;
    dst = *flapped + arealen + area;
    stop = (char *)MPV_CopyStm((Uint8 *)dst, (Uint8 *)src, n, SFMPV_DLM_MASK);
    copied = stop - src;
    *flapped += copied;
    dlm = MPV_CheckDelim((Uint8 *)(stop - 4));
    switch (dlm) {
    case MPV_DELIM_SEQUENCE:
    case MPV_DELIM_SEQUENCE_END:
        *end = copied + dst;
        break;
    case MPV_DELIM_PICTURE:
    case MPV_DELIM_GOP:
        /* the search back starts 3 bytes into this copy, so that it also
         * finds a code that runs into the copy */
        len = dst - area + 3;
        prev =
            (char *)MPV_BsearchDelim((Uint8 *)(dst + 3), len, SFMPV_DLM_MASK);
        if (prev && MPV_CheckDelim((Uint8 *)prev) == MPV_DELIM_PICTURE) {
            *end = copied + dst;
        }
        break;
    default:
        if (*flapped >= flapsize) {
            return SFMPV_ERR_FLAP;
        }
    }
    return 0;
}

/* once the input has ended and nothing wraps, set *end, unless it is set
 * already. *end points past the first sequence end, or else past the last
 * start code that ends a picture. of the codes in SFMPV_DLM_MASK, a sequence
 * header ends a picture. a group start ends a picture unless the code before
 * it is a sequence header. a picture start ends a picture when it is the first
 * code, or when the code before it is a picture start. returns 0, or
 * SFMPV_ERR_NODLM when no code ends a picture */
static Sint32 sfmpv_SearchEndDlm(struct sfd_handle *hn,
                                 struct sfbuf_ring_inf *inf, char **end)
{
    Sint32 arealen;
    Sint32 wraplen;
    Sint32 rest;
    Sint32 dlm;
    Sint32 prev;
    Sint32 step;
    char *area;
    char *p;
    char *found;
    char *last;

    if (*end) {
        return 0;
    }
    if (!sfmpv_GetTermSrc(hn)) {
        return 0;
    }
    area = inf->area;
    arealen = inf->len;
    wraplen = inf->wraplen;
    if (wraplen) {
        return 0;
    }
    prev = MPV_DELIM_PICTURE;
    last = NULL;
    /* @hack spelling: p assigned before rest, which loads area into R6 and
     * arealen into R5 (0x8ced33b2, 0x8ced33ba); with rest first, the two
     * trade and 4 of 97 words differ */
    p = area;
    rest = arealen;
    while (rest >= 4) {
        found = (char *)MPV_SearchDelim((Uint8 *)p, rest, SFMPV_DLM_MASK);
        if (!found) {
            break;
        }
        dlm = MPV_CheckDelim((Uint8 *)found);
        switch (dlm) {
        case MPV_DELIM_SEQUENCE:
            last = found;
            prev = MPV_DELIM_SEQUENCE;
            break;
        case MPV_DELIM_GOP:
            if (prev != MPV_DELIM_SEQUENCE) {
                last = found;
            }
            prev = MPV_DELIM_GOP;
            break;
        case MPV_DELIM_PICTURE:
            if (prev == MPV_DELIM_PICTURE) {
                last = found;
            }
            prev = MPV_DELIM_PICTURE;
            break;
        case MPV_DELIM_SEQUENCE_END:
            *end = found + 4;
            return 0;
        }
        step = found - p + 4;
        p += step;
        rest -= step;
    }
    if (!last) {
        return SFMPV_ERR_NODLM;
    }
    *end = last + 4;
    return 0;
}

static Sint32 sfmpv_RingAddRead(struct sfd_handle *hn, Sint32 len)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    Sint32 ibuf = trn->ibuf;
    Sint32 err;

    if ((err = SFBUF_RingAddRead(hn, ibuf, len)) != 0) {
        return err;
    }
    return 0;
}

/* decode the unit the run starts with: headers, a picture, or bytes up to the
 * next start code in SFMPV_DLM_MASK, which it skips. at the stream's end, as
 * sfmpv_IsTerm finds it, it marks the stream ended instead. *used gets the
 * bytes taken. *more gets TRUE after headers, a skipped picture or skipped
 * bytes, so that the caller decodes the next unit at once. it stays FALSE
 * otherwise, as after a decoded picture. returns 0 or an error code */
static Sint32 sfmpv_DecodeOneUnit(struct sfd_handle *hn, char *data, Sint32 len,
                                  Sint32 *used, Bool *more)
{
    struct sfd_trn *trn;
    struct sfmpv *mpv;
    struct mpv_obj *dec;
    struct mpv_picture picture;
    struct mpv_stream stm;
    struct mpv_request request;
    struct sfmpv_frm *frm;
    struct sfd_plyinf *plyinf;
    Sint32 err = 0;
    Sint32 dlm;
    Sint32 result;
    char *next;

    *used = 0;
    *more = FALSE;
    trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    mpv = &trn->u.mpv;
    dec = mpv->dec;
    /* @hack spelling: dlm set in an if and its else, which gives data R9 and
     * dlm R10 (MOV R6,R9 at 0x8ced34be, MOV R0,R10 at 0x8ced34e6); set by a ?:,
     * the two trade and 182 of 214 words differ */
    if (len < 4) {
        dlm = 0;
    } else {
        dlm = MPV_CheckDelim((Uint8 *)data);
    }
    /* @hack spelling: data and size stored through stm's address, which SHC
     * forms in R3 and R2 (ADD #56,R3 at 0x8ced34ea, ADD #56,R2 at 0x8ced34f0);
     * as stm.data and stm.size, 180 of 214 words differ */
    (&stm)->data = (Uint8 *)data;
    (&stm)->size = len;
    if (sfmpv_IsTerm(hn, len, dlm)) {
        sfmpv_TermDec(hn);
        goto done;
    }
    if (len <= 4) {
        goto done;
    }
    if (dlm & (MPV_DELIM_PICTURE | MPV_DELIM_GOP | MPV_DELIM_SEQUENCE)) {
        if (MPV_DecodePicAtr(dec, &stm, used) != 0) {
            err = SFLIB_SetErr(hn, SFMPV_ERR_HEADER);
        } else {
            *more = TRUE;
        }
    } else if (dlm & MPV_DELIM_FIRST_SLICE) {
        /* MPV_GetPicAtr's failure is recorded, not returned, and the picture
         * is used as it is */
        if (MPV_GetPicAtr(dec, &picture) != 0) {
            SFLIB_SetErr(hn, SFMPV_ERR_PICATR);
        }
        err = sfmpv_SetMvInf(hn, dec, &picture);
        if (err != 0) {
            goto done;
        }
        if (sfmpv_IsSkip(hn, &picture)) {
            if (MPV_SkipFrm(dec, &stm, used) != 0) {
                err = SFLIB_SetErr(hn, SFMPV_ERR_SKIP);
            } else {
                *more = TRUE;
                /* @hack spelling: type read through picture's address, which
                 * SHC forms in R6 (ADD #64,R6 at 0x8ced3596); as picture.type,
                 * 12 of 214 words differ */
                SFPLY_AddSkipPic(hn, 1, (&picture)->type);
            }
        } else {
            if (sfmpv_SetFrmPara(hn, &picture, &request, &frm) == 0) {
                /* @hack spelling: MPV_DecodeFrm's result through result,
                 * which SHC copies into R4 (MOV R0,R4 at 0x8ced35ee); tested
                 * in place, 80 of 214 words differ */
                result = MPV_DecodeFrm(dec, &stm, used, &request);
                if (result != 0) {
                    sfmpv_FreeFrm(frm);
                    err = SFLIB_SetErr(hn, SFMPV_ERR_DECODE);
                } else if (*used > 0) {
                    sfmpv_StbyFrm(frm);
                    plyinf = &hn->plyinf;
                    MPV_GetDctCnt(dec, &plyinf->dctcnt0, &plyinf->dctcnt1);
                    /* @hack spelling: type read through picture's address,
                     * which SHC forms in R6 (ADD #64,R6 at 0x8ced3624); as
                     * picture.type, 61 of 214 words differ */
                    SFPLY_AddDecPic(hn, 1, (&picture)->type);
                    mpv->late = FALSE;
                } else {
                    sfmpv_FreeFrm(frm);
                }
            }
        }
    } else if (dlm == 0) {
        next = (char *)MPV_SearchDelim((Uint8 *)data, len, SFMPV_DLM_MASK);
        if (next) {
            *used = next - data;
            *more = TRUE;
        }
    }
done:
    return err;
}

/* TRUE at a sequence end, or once the input has ended with 4 bytes or fewer
 * left */
static Bool sfmpv_IsTerm(struct sfd_handle *hn, Sint32 len, Sint32 dlm)
{
    if (dlm == MPV_DELIM_SEQUENCE_END) {
        return TRUE;
    }
    if (len <= 4) {
        if (sfmpv_GetTermSrc(hn) == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

static Sint32 sfmpv_SetMvInf(struct sfd_handle *hn, struct mpv_obj *dec,
                             struct mpv_picture *picture)
{
    struct sfd_mvinf *mvinf = &hn->mvinf;
    Sint32 bitrate;

    if (mvinf->bitrate) {
        return 0;
    }
    mvinf->width = picture->width;
    mvinf->height = picture->height;
    mvinf->mb_width = picture->mb_width;
    mvinf->mb_height = picture->mb_height;
    if (MPV_GetBitRate(dec, &bitrate)) {
        return SFLIB_SetErr(hn, SFMPV_ERR_BITRATE);
    }
    mvinf->bitrate = bitrate;
    return sfmpv_ChkBufSiz(hn, mvinf);
}

/* check the movie's size against the frames. returns 0, SFMPV_ERR_RFBSIZE
 * when it doesn't fit the reference frames, or SFMPV_ERR_FRMSIZE when it has
 * more pixels than a picture frame. with no picture frames, it returns
 * SFMPV_ERR_NFRM */
static Sint32 sfmpv_ChkBufSiz(struct sfd_handle *hn, struct sfd_mvinf *mvinf)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    struct sfmpv_para *para = &mpv->para;
    Sint32 width = mvinf->width;
    Sint32 height = mvinf->height;

    if (width > para->y_width || width > para->c_width * 2 ||
        height > para->y_height || height > para->c_height * 2) {
        return SFLIB_SetErr(hn, SFMPV_ERR_RFBSIZE);
    }
    if (width * height * 3 / 2 > para->frm_width * para->frm_height * 3 / 2) {
        return SFLIB_SetErr(hn, SFMPV_ERR_FRMSIZE);
    }
    if (para->nfrm <= 0) {
        return SFLIB_SetErr(hn, SFMPV_ERR_NFRM);
    }
    return 0;
}

/* TRUE to skip a picture of a type the handle doesn't decode, one that lacks
 * its reference, or one too late to decode. with SFD_COND_DEC_ALL set, it
 * skips none. otherwise each answer updates mpv->defect */
static Bool sfmpv_IsSkip(struct sfd_handle *hn, struct mpv_picture *picture)
{
    Sint32 type;
    Bool skip;

    if (hn->cond[SFD_COND_DEC_ALL]) {
        return FALSE;
    }
    type = picture->type;
    if (sfmpv_IsPtypeSkip(hn, type) || sfmpv_IsDefect(hn, type) ||
        sfmpv_IsLate(hn, picture, type)) {
        skip = TRUE;
    } else {
        skip = FALSE;
    }
    sfmpv_SetDefect(hn, type, skip);
    return skip;
}

static Bool sfmpv_IsDefect(struct sfd_handle *hn, Sint32 type)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    Sint32 defect = mpv->defect;
    Bool lacks = FALSE;

    switch (defect) {
    case SFMPV_DEFECT_PB:
        if (type == MPV_PTYPE_P || type == MPV_PTYPE_B) {
            lacks = TRUE;
        }
        break;
    case SFMPV_DEFECT_B:
        if (type == MPV_PTYPE_B) {
            lacks = TRUE;
        }
        break;
    case SFMPV_DEFECT_NONE:
        break;
    }
    return lacks;
}

/* update mpv->defect once a picture is tested. a skipped I or P picture sets
 * it to SFMPV_DEFECT_PB. a picture not skipped moves it from SFMPV_DEFECT_PB
 * to SFMPV_DEFECT_B, or from SFMPV_DEFECT_B to SFMPV_DEFECT_NONE */
static void sfmpv_SetDefect(struct sfd_handle *hn, Sint32 type, Bool skipped)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    Sint32 defect = mpv->defect;

    if (skipped == TRUE) {
        if (type == MPV_PTYPE_I || type == MPV_PTYPE_P) {
            defect = SFMPV_DEFECT_PB;
        }
    } else {
        if (defect == SFMPV_DEFECT_PB) {
            defect = SFMPV_DEFECT_B;
        } else if (defect == SFMPV_DEFECT_B) {
            defect = SFMPV_DEFECT_NONE;
        }
    }
    mpv->defect = defect;
}

/* TRUE for a picture of a type SFD_COND_DEC_I, _P or _B turns off, and for
 * any other type */
static Bool sfmpv_IsPtypeSkip(struct sfd_handle *hn, Sint32 type)
{
    Sint32 *cond = hn->cond;
    Sint32 dec;

    switch (type) {
    case MPV_PTYPE_I:
        dec = cond[SFD_COND_DEC_I];
        break;
    case MPV_PTYPE_P:
        dec = cond[SFD_COND_DEC_P];
        break;
    case MPV_PTYPE_B:
        dec = cond[SFD_COND_DEC_B];
        break;
    default:
        return TRUE;
    }
    if (dec == 0) {
        return TRUE;
    }
    return FALSE;
}

/* TRUE when a picture is too late to decode. a function set with
 * SFD_SetUsrIsSkipFn decides in its place. otherwise an I picture's time
 * first updates the interval between I pictures. a picture's deadline is its
 * time less SFD_COND_DEC_TIME. an I or P picture's deadline counts from the
 * time expected for the next I picture instead. the picture is late when the
 * handle's time is past its deadline, and a late picture sets mpv->late. no
 * picture is late while mpv->late is TRUE, while SFD_COND_SKIP_LATE is 0, or
 * while the handle's time is negative */
static Bool sfmpv_IsLate(struct sfd_handle *hn, struct mpv_picture *picture,
                         Sint32 type)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    struct sftim *tim = &hn->tim;
    Sint32 time;
    Sint32 tunit;
    Sint32 now;
    Sint32 now_tunit;
    Sint32 dtime;
    Sint32 dtunit;
    sfd_skipfn skipfn;

    sfmpv_PicAtr2Time(hn, picture, &time, &tunit);
    skipfn = tim->skipfn;
    if (skipfn) {
        return skipfn(hn, type, time, tunit);
    }
    if (type == MPV_PTYPE_I) {
        SFTIM_UpdateItime(tim, time);
    }
    if (type == MPV_PTYPE_I || type == MPV_PTYPE_P) {
        time = SFTIM_GetNextItime(tim);
    }
    if (mpv->late == TRUE) {
        return FALSE;
    }
    if (hn->cond[SFD_COND_SKIP_LATE] == 0) {
        return FALSE;
    }
    SFTIM_GetTime(hn, &now, &now_tunit);
    if (now < 0) {
        return FALSE;
    }
    sfmpv_GetDtime(hn, type, &dtime, &dtunit);
    time -= tunit * dtime / dtunit;
    if (UTY_CmpTime(now, now_tunit, time, tunit) != FALSE) {
        return FALSE;
    }
    /* @hack redundant: a switch on type that does nothing, which puts tim in
     * R11 and mpv in R12 (0x8ced38c8, 0x8ced38d2); without it, the two trade
     * and 37 of 110 words differ */
    switch (type) {
    case MPV_PTYPE_I:
    case MPV_PTYPE_P:
    case MPV_PTYPE_B:
        break;
    }
    mpv->late = TRUE;
    return TRUE;
}

/* set *time to the time a picture takes to decode, and *tunit to its unit.
 * they are SFD_COND_DEC_TIME and SFD_COND_DEC_TUNIT, whatever the picture's
 * type */
static void sfmpv_GetDtime(struct sfd_handle *hn, Sint32 type, Sint32 *time,
                           Sint32 *tunit)
{
    Sint32 *cond = hn->cond;
    /* @hack spelling: the decode time through dtime, which SHC loads into R4
     * over hn (MOV.L @(R0,R4),R4 at 0x8ced399c); stored directly, it takes R1
     * and 2 of 9 words differ */
    Sint32 dtime;

    *tunit = cond[SFD_COND_DEC_TUNIT];
    dtime = cond[SFD_COND_DEC_TIME];
    *time = dtime;
}

/* set condition no of the handle's decoder to val, as MPV_SetCond does. hn
 * must be a valid handle. returns 0 or SFMPV_ERR_COND */
Sint32 SFD_SetMpvCond(struct sfd_handle *hn, Sint32 no, Sint32 val)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    struct mpv_obj *dec = mpv->dec;
    Sint32 err = MPV_SetCond(dec, no, val);

    if (err) {
        return SFLIB_SetErr(hn, SFMPV_ERR_COND);
    }
    return 0;
}

/* give the handle's stage a decoder over the frames SFD_SetMpvPara set.
 * returns 0 or an error code */
Sint32 SFMPV_Create(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    struct mpv_obj *dec;
    Sint32 err = sfmpv_InitInf(hn, mpv);

    if (err) {
        return err;
    }
    dec = MPV_Create();
    if (!dec) {
        return SFLIB_SetErr(hn, SFMPV_ERR_CREATE);
    }
    err = MPV_SetErrFunc(dec, sfmpv_ErrFn, hn);
    if (err) {
        sfmpv_DestroySub(dec);
        return SFLIB_SetErr(hn, SFMPV_ERR_ERRFN);
    }
    /* decode macroblocks. don't search on for a missing start code. predict
     * both half-pel offsets from four samples */
    MPV_SetCond(dec, MPV_COND_DECODE, 1);
    MPV_SetCond(dec, MPV_COND_RECOVER, 0);
    MPV_SetCond(dec, MPV_COND_HALF_PEL_2, 0);
    mpv->dec = dec;
    return 0;
}

static Sint32 sfmpv_InitInf(struct sfd_handle *hn, struct sfmpv *mpv)
{
    /* @hack spelling: rfb and frm read through sfmpv_para's address, which SHC
     * loads from the pool for each (0x8ced3a82, 0x8ced3a8c); as
     * sfmpv_para.rfb and .frm, 31 of 35 words differ */
    if (!(&sfmpv_para)->rfb || !(&sfmpv_para)->frm) {
        return SFLIB_SetErr(hn, SFMPV_ERR_PARA);
    }
    mpv->para = sfmpv_para;
    mpv->dec = NULL;
    mpv->held = NULL;
    mpv->defect = SFMPV_DEFECT_NONE;
    sfmpv_InitRfb(hn);
    mpv->late = FALSE;
    return 0;
}

/* the decoder's errors become the handle's */
static void sfmpv_ErrFn(void *obj, Sint32 ecode)
{
    SFLIB_SetErr((struct sfd_handle *)obj, ecode);
}

Sint32 SFMPV_Destroy(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    struct mpv_obj *dec = mpv->dec;
    Sint32 err = sfmpv_DestroySub(dec);

    if (err) {
        return SFLIB_SetErr(hn, SFMPV_ERR_DESTROY);
    }
    return 0;
}

static Sint32 sfmpv_DestroySub(struct mpv_obj *dec)
{
    return MPV_Destroy(dec);
}

Sint32 SFMPV_Standby(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

Sint32 SFMPV_Start(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFMPV_Stop(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFMPV_Pause(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

/* the stage takes no writes: both write slots record SFMPV_ERR_NOWRITE */
Sint32 SFMPV_GetWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFMPV_ERR_NOWRITE);
}

Sint32 SFMPV_AddWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFMPV_ERR_NOWRITE);
}

/* arg1 is a struct sfd_frm **. store through it the sfd_frm of the decoded
 * picture first in time code order, or NULL. a picture is handed out once two
 * are decoded, or one after the stream's end. it stays decoded until
 * SFMPV_AddRead frees it. once the stream has ended and every picture is
 * free, the output ends */
Sint32 SFMPV_GetRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;
    /* @hack redundant: para, set and never read, which has SHC add its offset
     * (ADD #16,R4 at 0x8ced3b5e) to a register the call then overwrites;
     * without it, 30 of 38 words differ */
    struct sfmpv_para *para = &mpv->para;
    struct sfmpv_frm *frm;
    Sint32 n;

    n = sfmpv_GetNumFrm(hn);
    if (n == -1) {
        sfmpv_SetTermDst(hn, TRUE);
        *(struct sfd_frm **)arg1 = NULL;
    } else if ((frm = sfmpv_HoldFrm(hn)) == NULL) {
        *(struct sfd_frm **)arg1 = NULL;
    } else {
        sfmpv_SetFrmInf(hn, frm, (struct sfd_frm **)arg1);
    }
    return 0;
}

static Bool sfmpv_GetTermSrc(struct sfd_handle *hn)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    Sint32 ibuf = trn->ibuf;

    return SFBUF_GetTermFlg(hn, ibuf);
}

static Bool sfmpv_GetTermDst(struct sfd_handle *hn)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    Sint32 obuf = trn->obuf[0];

    return SFBUF_GetTermFlg(hn, obuf);
}

static void sfmpv_SetTermDst(struct sfd_handle *hn, Bool flag)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    Sint32 obuf = trn->obuf[0];

    SFBUF_SetTermFlg(hn, obuf, flag);
}

/* mark the output ready once the input is ready and sfmpv_IsPrepEnd holds */
static void sfmpv_ChkPrepFlg(struct sfd_handle *hn)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    Sint32 ibuf = trn->ibuf;
    Sint32 obuf = trn->obuf[0];

    if (SFBUF_GetPrepFlg(hn, ibuf) == TRUE) {
        if (sfmpv_IsPrepEnd(hn)) {
            SFBUF_SetPrepFlg(hn, obuf, TRUE);
        }
    }
}

/* hand frm out through the first slot of the output's picture queue. the
 * slot's sfd_frm describes it, and *out points there. frm stays in mpv->held
 * until SFMPV_AddRead frees it */
static void sfmpv_SetFrmInf(struct sfd_handle *hn, struct sfmpv_frm *frm,
                            struct sfd_frm **out)
{
    struct mpv_picture *picture;
    struct sfmpv *mpv;
    Sint32 obuf;
    struct sfbuf_tbl *buftbl;
    struct sfbuf_vfrm_que *slot;
    Sint32 time;
    Sint32 tunit;

    picture = &frm->picture;
    mpv = &hn->trntbl.trn[SFD_TR_VIDEO].u.mpv;
    obuf = hn->trntbl.trn[SFD_TR_VIDEO].obuf[0];
    buftbl = &hn->buftbl;
    slot = &buftbl->buf[obuf].u.vfrm.que[0];
    *out = &slot->frm;
    slot->stat = TRUE;
    mpv->held = frm;
    sfmpv_PicAtr2Time(hn, picture, &time, &tunit);
    (*out)->width = picture->width;
    (*out)->height = picture->height;
    (*out)->mb_width = picture->mb_width;
    (*out)->mb_height = picture->mb_height;
    (*out)->type = picture->type;
    (*out)->time = time;
    (*out)->tunit = tunit;
    (*out)->unk_1c = hn->prm.unk_2c[3];
    (*out)->data = frm->data;
    (*out)->unk_24 = 0;
    (*out)->unk_28 = 0;
}

/* free the picture handed out, whose sfd_frm arg1 must be. returns 0,
 * SFMPV_ERR_NOTOUT with none out, or SFMPV_ERR_FRM for another sfd_frm */
Sint32 SFMPV_AddRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    Sint32 imask = get_imask();
    struct sfd_trn *trn;
    struct sfmpv *mpv;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    struct sfmpv_para *para;
    struct sfbuf_tbl *buftbl;
    Sint32 obuf;
    struct sfbuf_vfrm_que *slot;

    set_imask(IMASK_ALL);
    trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    mpv = &trn->u.mpv;
    /* @hack redundant: para, set and never read, which keeps mpv in R13, a
     * register the function saves (MOV.L R13,@-R15 at 0x8ced3cf0, ADD #16,R13
     * at 0x8ced3d26); without it, 71 of 85 words differ */
    para = &mpv->para;
    obuf = trn->obuf[0];
    buftbl = &hn->buftbl;
    slot = &buftbl->buf[obuf].u.vfrm.que[0];
    if (slot->stat != TRUE) {
        set_imask(imask);
        return SFLIB_SetErr(hn, SFMPV_ERR_NOTOUT);
    }
    if (&slot->frm != (struct sfd_frm *)arg1) {
        set_imask(imask);
        return SFLIB_SetErr(hn, SFMPV_ERR_FRM);
    }
    slot->stat = FALSE;
    sfmpv_FreeFrm(mpv->held);
    set_imask(imask);
    return 0;
}

/* set *time and *tunit to the picture's time code, less the earliest time
 * code the handle has seen. a time code no later than that becomes the
 * earliest, in hn->tim */
static void sfmpv_PicAtr2Time(struct sfd_handle *hn,
                              struct mpv_picture *picture, Sint32 *time,
                              Sint32 *tunit)
{
    struct sftim_tcode tcode;
    struct sftim *tim;
    Sint32 tc_time;
    Sint32 tc_tunit;

    /* @hack spelling: each field stored through tcode's address, which SHC
     * forms again in R3 for each (ADD #20,R3 at 0x8ced3db6 and 0x8ced3dc4); as
     * tcode's fields, 68 of 70 words differ */
    (&tcode)->rate = picture->rate;
    (&tcode)->drop = picture->drop;
    (&tcode)->hour = picture->hour;
    (&tcode)->minute = picture->minute;
    (&tcode)->second = picture->second;
    (&tcode)->frame = picture->frame;
    (&tcode)->nadd = picture->temporal_ref;
    SFTIM_Tc2Time(&tcode, &tc_time, &tc_tunit);
    tim = &hn->tim;
    if (tim->bound_time >= tc_time) {
        tim->bound_time = tc_time;
        tim->bound_tunit = tc_tunit;
        tim->tcode = tcode;
    }
    *time = tc_time - tim->bound_time;
    *tunit = tc_tunit;
}

/* lay the two reference frames out in para->rfb, and describe them in
 * mpv->rfb. the first frame's Cb, Cr and Y planes come first, then the
 * second's Y, Cb and Cr. cut para->frm into nfrm free pictures. the stream
 * then starts over: mpv->first is 0, mpv->second is 1, and mpv->term is
 * FALSE */
static void sfmpv_InitRfb(struct sfd_handle *hn)
{
    Sint32 imask = get_imask();
    struct sfd_trn *trn;
    struct sfmpv *mpv;
    struct sfmpv_para *para;
    Sint32 c_size;
    Sint32 y_size;
    Sint32 c_width;
    Sint32 c_height;
    Sint32 y_width;
    Sint32 y_height;
    Sint32 y0;
    Sint32 y1;
    Sint32 cb1;
    Sint32 i;
    Sint32 frm_width;
    Sint32 frm_height;
    Sint32 frm_size;
    struct mpv_frame *rfb;
    Uint8 *area;

    set_imask(IMASK_ALL);
    trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    mpv = &trn->u.mpv;
    para = &mpv->para;
    c_width = para->c_width;
    c_height = para->c_height;
    y_width = para->y_width;
    y_height = para->y_height;
    frm_width = para->frm_width;
    frm_height = para->frm_height;
    c_size = c_width * c_height;
    y_size = y_width * y_height;
    frm_size = frm_width * frm_height * 3 / 2;
    area = para->rfb;
    rfb = mpv->rfb;
    rfb->c_pitch = c_width;
    rfb[1].c_pitch = c_width;
    rfb->y_pitch = y_width;
    rfb[1].y_pitch = y_width;
    rfb->cb = area;
    rfb->cr = area + c_size;
    /* @hack spelling: y0, y1 and cb1 each assigned inside the store that first
     * uses it, which keeps y0 and y1 in the frame (MOV.L R1,@(4,R15) at
     * 0x8ced3eb6, MOV.L R2,@R15 at 0x8ced3ec0); assigned in statements of
     * their own, SHC saves R12 too and 70 of 120 words differ */
    rfb->y = (y0 = c_size + c_size) + area;
    rfb[1].y = (y1 = y0 + y_size) + area;
    rfb[1].cb = (cb1 = y1 + y_size) + area;
    rfb[1].cr = (cb1 + c_size) + area;
    area = para->frm;
    for (i = 0; i < para->nfrm; i++) {
        mpv->frm[i].stat = SFMPV_FRM_FREE;
        mpv->frm[i].data = area + (i * frm_size);
    }
    mpv->first = 0;
    mpv->second = 1;
    mpv->term = FALSE;
    set_imask(imask);
}

/* take a free picture for the decoder to write into *frm. fill the request
 * with its information, its data and the two reference frames. an I or P
 * picture swaps which frame is first and which second. returns 0, or -1 with
 * no picture free */
static Sint32 sfmpv_SetFrmPara(struct sfd_handle *hn,
                               struct mpv_picture *picture,
                               struct mpv_request *request,
                               struct sfmpv_frm **frm)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;

    *frm = sfmpv_AllocFrm(hn);
    if (!*frm) {
        return -1;
    }
    (*frm)->picture = *picture;
    if (picture->type == MPV_PTYPE_I || picture->type == MPV_PTYPE_P) {
        mpv->first ^= 1;
        mpv->second ^= 1;
    }
    /* @hack spelling: rfb, a pointer to mpv->rfb in a scope of its own, which
     * SHC forms once in R4 for both copies (ADD #64,R4 at 0x8ced3fa0); indexed
     * as mpv->rfb, it is formed for each and 35 of 77 words differ */
    {
        struct mpv_frame *rfb = mpv->rfb;

        request->first = rfb[mpv->first];
        request->second = rfb[mpv->second];
    }
    request->output = (*frm)->data;
    request->picture = &(*frm)->picture;
    request->misses = 0;
    request->recoveries = 0;
    return 0;
}

static void sfmpv_TermDec(struct sfd_handle *hn)
{
    struct sfd_trn *trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    struct sfmpv *mpv = &trn->u.mpv;

    mpv->term = TRUE;
}

/* the pictures decoded and not yet freed, or -1 once the stream has ended and
 * every picture is free */
static Sint32 sfmpv_GetNumFrm(struct sfd_handle *hn)
{
    Sint32 imask = get_imask();
    struct sfd_trn *trn;
    struct sfmpv *mpv;
    struct sfmpv_para *para;
    Sint32 nfree;
    Sint32 nready;
    Sint32 i;

    set_imask(IMASK_ALL);
    trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    mpv = &trn->u.mpv;
    para = &mpv->para;
    nfree = 0;
    nready = 0;
    for (i = 0; i < para->nfrm; i++) {
        if (mpv->frm[i].stat == SFMPV_FRM_READY) {
            nready++;
        }
        if (mpv->frm[i].stat == SFMPV_FRM_FREE) {
            nfree++;
        }
    }
    if (mpv->term == TRUE && nfree == para->nfrm) {
        nready = -1;
    }
    set_imask(imask);
    return nready;
}

static struct sfmpv_frm *sfmpv_AllocFrm(struct sfd_handle *hn)
{
    Sint32 imask = get_imask();
    struct sfd_trn *trn;
    struct sfmpv *mpv;
    struct sfmpv_para *para;
    struct sfmpv_frm *frm;
    Sint32 i;

    set_imask(IMASK_ALL);
    trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    mpv = &trn->u.mpv;
    para = &mpv->para;
    frm = NULL;
    for (i = 0; i < para->nfrm; i++) {
        if (mpv->frm[i].stat == SFMPV_FRM_FREE) {
            mpv->frm[i].stat = SFMPV_FRM_DEC;
            frm = &mpv->frm[i];
            break;
        }
    }
    set_imask(imask);
    return frm;
}

static void sfmpv_FreeFrm(struct sfmpv_frm *frm)
{
    frm->stat = SFMPV_FRM_FREE;
}

static void sfmpv_StbyFrm(struct sfmpv_frm *frm)
{
    frm->stat = SFMPV_FRM_READY;
}

/* the decoded picture first in time code order, or NULL. it waits for a
 * second decoded picture, unless the stream has ended */
static struct sfmpv_frm *sfmpv_HoldFrm(struct sfd_handle *hn)
{
    Sint32 imask = get_imask();
    struct sfd_trn *trn;
    struct sfmpv *mpv;
    struct sfmpv_para *para;
    struct sfmpv_frm *first;
    Sint32 n;
    Sint32 i;
    Sint32 stat;
    struct sfmpv_frm *frm;

    set_imask(IMASK_ALL);
    first = NULL;
    trn = &hn->trntbl.trn[SFD_TR_VIDEO];
    mpv = &trn->u.mpv;
    para = &mpv->para;
    n = 0;
    for (i = 0; i < para->nfrm; i++) {
        frm = &mpv->frm[i];
        stat = frm->stat;
        if (stat == SFMPV_FRM_READY) {
            n++;
            if (sfmpv_IsChkFirst(first, &mpv->frm[i])) {
                first = &mpv->frm[i];
            } else {
                /* @hack redundant: an empty else, which loads para->nfrm into
                 * R3 for the loop's test (MOV.L @(28,R9),R3 at 0x8ced4168);
                 * without it, R2 takes it and 2 of 84 words differ */
            }
        }
    }
    if (n < 2 && (n != 1 || mpv->term != TRUE)) {
        first = NULL;
    }
    set_imask(imask);
    return first;
}

/* TRUE when there is no first yet, or frm's time code comes before first's */
static Bool sfmpv_IsChkFirst(struct sfmpv_frm *first, struct sfmpv_frm *frm)
{
    struct mpv_picture *a;
    struct mpv_picture *b;

    if (!first) {
        return TRUE;
    }
    a = &first->picture;
    b = &frm->picture;
    if (sfmpv_IsBigTime(a, b)) {
        return TRUE;
    }
    return FALSE;
}

/* TRUE when a's time code comes after b's. the hours decide, then the
 * minutes, seconds, frames and temporal references */
static Bool sfmpv_IsBigTime(struct mpv_picture *a, struct mpv_picture *b)
{
    if (a->hour > b->hour) {
        return TRUE;
    }
    if (a->hour < b->hour) {
        return FALSE;
    }
    if (a->minute > b->minute) {
        return TRUE;
    }
    if (a->minute < b->minute) {
        return FALSE;
    }
    if (a->second > b->second) {
        return TRUE;
    }
    if (a->second < b->second) {
        return FALSE;
    }
    if (a->frame > b->frame) {
        return TRUE;
    }
    if (a->frame < b->frame) {
        return FALSE;
    }
    if (a->temporal_ref > b->temporal_ref) {
        return TRUE;
    }
    return FALSE;
}

const struct sfd_trif SFD_tr_vd_mpv = {{
    SFMPV_Init,
    SFMPV_Finish,
    SFMPV_ExecServer,
    SFMPV_Create,
    SFMPV_Destroy,
    SFMPV_Standby,
    SFMPV_Start,
    SFMPV_Stop,
    SFMPV_Pause,
    SFMPV_GetWrite,
    SFMPV_AddWrite,
    SFMPV_GetRead,
    SFMPV_AddRead,
}};
