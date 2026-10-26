/* the Sofdec system stream demultiplexer stage. it parses the input a whole
 * unit at a time, through the MPS parser. it copies the data of audio, video
 * and private packets to the audio, video and user rings */
#include <machine.h>
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mps.h"
#include "movie/sdk/sfd.h"

/* the parser handles sfmps_libwork has room for */
#define SFMPS_NHN 4

/* a server step parses units until it has taken this many input bytes or
 * more */
#define SFMPS_STEP_BYTES 0x19000

/* sfmps_ChkAvPlay's bits for the kinds of stream the system headers count */
#define SFMPS_HAS_AUDIO 1
#define SFMPS_HAS_VIDEO 2

/* the errors the stage records through SFLIB_SetErr. most name the MPS call
 * that failed */
#define SFMPS_ERR_INIT ((Sint32)0xff000d01)
#define SFMPS_ERR_DECHD ((Sint32)0xff000d03)
/* a unit's headers lead to no packet */
#define SFMPS_ERR_NOPKET ((Sint32)0xff000d05)
#define SFMPS_ERR_PKETHD ((Sint32)0xff000d06)
/* the system headers count no audio stream and no video stream */
#define SFMPS_ERR_NOSTM ((Sint32)0xff000d07)
#define SFMPS_ERR_CREATE ((Sint32)0xff000d08)
#define SFMPS_ERR_ERRFN ((Sint32)0xff000d09)
#define SFMPS_ERR_DESTROY ((Sint32)0xff000d0a)
/* a ring call to the stage, which has no ring of its own */
#define SFMPS_ERR_NORING ((Sint32)0xff000d0b)

/* a packet's copy function. it copies the packet's len bytes of data, at
 * data, to their ring. it returns 1 once the packet is used up, 0 while the
 * ring has no room for all of it, or an error */
typedef Sint32 (*sfmps_copyfn)(struct sfd_handle *hn, Sint32 number, char *data,
                               Sint32 len);

static Sint32 sfmps_DecodeSomeUnit(struct sfd_handle *hn);
static Sint32 sfmps_RingGetRead(struct sfd_handle *hn, char **data,
                                Sint32 *len);
static Sint32 sfmps_RingAddRead(struct sfd_handle *hn, Sint32 len);
static Sint32 sfmps_DecodeOneUnit(struct sfd_handle *hn, char *data, Sint32 len,
                                  Sint32 *used);
static void sfmps_SkipZeroPack(struct sfd_handle *hn, char *data, Sint32 len,
                               Sint32 *used);
static Sint32 sfmps_CopyPketData(struct sfd_handle *hn, char *data, Sint32 len,
                                 Sint32 *used);
static Sint32 sfmps_CopyAudio(struct sfd_handle *hn, Sint32 number, char *data,
                              Sint32 len);
static Sint32 sfmps_CopyVideo(struct sfd_handle *hn, Sint32 number, char *data,
                              Sint32 len);
static Sint32 sfmps_CopyPrvate(struct sfd_handle *hn, Sint32 number, char *data,
                               Sint32 len);
static Sint32 sfmps_CopyPadding(struct sfd_handle *hn, Sint32 number,
                                char *data, Sint32 len);
static Sint32 sfmps_CopyDstBuft(struct sfd_handle *hn, Sint32 bufno, char *src,
                                Sint32 len);
static void sfmps_SwitchCopy(char *dst, char *src, Sint32 len);
static void sfmps_SetTermDst(struct sfd_handle *hn, Bool flag);
static Bool sfmps_GetTermDst(struct sfd_handle *hn);
static Bool sfmps_IsTerm(struct sfd_handle *hn, Sint32 len, Sint32 found);
static void sfmps_ChkPrepFlg(struct sfd_handle *hn);
static Bool sfmps_IsPrepEnd(struct sfd_handle *hn, Sint32 bufno);
static void sfmps_ChkAvPlay(struct sfd_handle *hn);
static void sfmps_InitInf(struct mps_handle **mps);
static void sfmps_ErrFn(void *obj, Sint32 ecode);
static Sint32 sfmps_DestroySub(struct mps_handle *mps);

/* the copy function for each MPS_PKET_* kind but MPS_PKET_OTHER. it is defined
 * after SFD_tr_sd_mps, as the image's constant section holds the two in that
 * order */
static const sfmps_copyfn sfmps_CopyPketFn[MPS_PKET_OTHER];

/* the parser's work, laid out as MPS_Init lays it for SFMPS_NHN handles */
static Uint32 sfmps_libwork[(sizeof(struct mps_lib_work) +
                             (sizeof(struct mps_handle) * (SFMPS_NHN - 1))) /
                            sizeof(Uint32)];

/* set the parser up over sfmps_libwork. the stage's functions are sfd_trfns,
 * and none of them reads its three arguments */
Sint32 SFMPS_Init(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    Sint32 err;

    err = MPS_Init(SFMPS_NHN, sfmps_libwork);
    if (err != 0) {
        return SFLIB_SetErr(NULL, SFMPS_ERR_INIT);
    }
    return 0;
}

Sint32 SFMPS_Finish(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    MPS_Finish();
    return 0;
}

/* one server step: parse units of input into the outputs, then check whether
 * the outputs are ready. once every output has ended, a step does nothing. it
 * returns the parse's error */
Sint32 SFMPS_ExecServer(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                        Sint32 arg3)
{
    Sint32 err;

    if (sfmps_GetTermDst(hn) == TRUE) {
        return 0;
    }
    err = sfmps_DecodeSomeUnit(hn);
    sfmps_ChkPrepFlg(hn);
    return err;
}

/* parse units of input until one fails, one can't be used yet, or
 * SFMPS_STEP_BYTES bytes or more are taken. once any of a unit is used, the
 * whole unit is taken off the ring */
static Sint32 sfmps_DecodeSomeUnit(struct sfd_handle *hn)
{
    Sint32 used;
    char *data;
    Sint32 len;
    Sint32 total;
    Sint32 unit;
    Sint32 err;

    total = 0;
    do {
        /* @hack spelling: the result assigned in the test, which SHC tests in
         * R0 (TST R0,R0 at 0x8ced81ba) and copies to R4 in the branch's delay
         * slot. assigned first, it is copied to R4 to be tested, and 3 of 47
         * words differ */
        if ((err = sfmps_RingGetRead(hn, &data, &len)) != 0) {
            break;
        }
        err = sfmps_DecodeOneUnit(hn, data, len, &used);
        if (err != 0) {
            break;
        }
        if (used == 0) {
            break;
        }
        /* @hack spelling: the unit read into a local of its own, then copied to
         * used, which has SHC load it into R4 (MOV.L @(40,R14),R4 at
         * 0x8ced81d6). read into used directly, it goes through R2, and 3 of
         * 47 words differ */
        unit = hn->prm.unit;
        used = unit;
        /* @hack spelling: the result assigned in the test, which SHC tests in
         * R0 (TST R0,R0 at 0x8ced81e0) and copies to R4 in the branch's delay
         * slot. assigned first, it is copied to R4 to be tested, and 3 of 47
         * words differ */
        if ((err = sfmps_RingAddRead(hn, used)) != 0) {
            break;
        }
        total += used;
    } while (total < SFMPS_STEP_BYTES);
    return err;
}

/* the run of input from rp up to the ring's end, in *data and *len */
static Sint32 sfmps_RingGetRead(struct sfd_handle *hn, char **data, Sint32 *len)
{
    struct sfd_trn *trn;
    Sint32 ibuf;
    struct sfbuf_ring_inf inf;
    Sint32 err;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    ibuf = trn->ibuf;
    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced821c) and copies to R5 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 4 of 38 words
     * differ */
    if ((err = SFBUF_RingGetRead(hn, ibuf, &inf)) != 0) {
        return err;
    }
    /* @hack spelling: the reads through (&inf)->, which form inf's address
     * afresh for each (ADD #12,R3 at 0x8ced8230, ADD #12,R2 at 0x8ced823a).
     * without it, SHC reads inf on the stack directly, the object runs 12 bytes
     * short, and 18 of 38 words differ */
    *data = (&inf)->area;
    *len = (&inf)->len;
    return 0;
}

static Sint32 sfmps_RingAddRead(struct sfd_handle *hn, Sint32 len)
{
    struct sfd_trn *trn;
    Sint32 ibuf;
    Sint32 err;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    ibuf = trn->ibuf;
    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced8288) and copies to R5 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 4 of 26 words
     * differ */
    if ((err = SFBUF_RingAddRead(hn, ibuf, len)) != 0) {
        return err;
    }
    return 0;
}

/* parse the headers at the start of the unit at data, and copy the packet they
 * lead to. a unit with no headers is skipped whole. *used gets the bytes used.
 * it is 0 at the stream's end, and while the packet isn't all in or doesn't
 * fit its ring. it is 0 too when the headers lead to no packet, or the copy
 * fails */
static Sint32 sfmps_DecodeOneUnit(struct sfd_handle *hn, char *data, Sint32 len,
                                  Sint32 *used)
{
    struct sfd_trn *trn;
    struct mps_handle *mps;
    Sint32 hdlen;
    Sint32 found;
    Sint32 copied;
    Sint32 skipped;
    Sint32 result;
    Sint32 err;

    err = 0;
    *used = 0;
    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    mps = trn->u.mps;
    /* @hack spelling: MPS_DecHd's result through a local, which SHC copies to
     * R4 to test (MOV R0,R4 at 0x8ced82d8). tested directly, it stays in R0,
     * the object runs 4 bytes short, and 66 of 97 words differ */
    result = MPS_DecHd(mps, (Uint8 *)data, len, &hdlen, &found);
    if (result != 0) {
        err = SFLIB_SetErr(hn, SFMPS_ERR_DECHD);
    }
    if (sfmps_IsTerm(hn, len, found)) {
        sfmps_SetTermDst(hn, TRUE);
    } else if (found == 0) {
        sfmps_SkipZeroPack(hn, data, len, &skipped);
        *used = skipped;
    } else if ((found & MPS_DLM_PKET) == 0) {
        err = SFLIB_SetErr(hn, SFMPS_ERR_NOPKET);
    } else {
        data += hdlen;
        len -= hdlen;
        err = sfmps_CopyPketData(hn, data, len, &copied);
        if (copied != 0) {
            *used = hdlen + copied;
        }
    }
    return err;
}

/* skip a unit with no headers. *used gets the whole unit once it is all in,
 * or else 0 */
static void sfmps_SkipZeroPack(struct sfd_handle *hn, char *data, Sint32 len,
                               Sint32 *used)
{
    Sint32 unit;

    unit = hn->prm.unit;
    if (len >= unit) {
        *used = unit;
    } else {
        *used = 0;
    }
    /* @hack redundant: a return where control returns anyway, which gives the
     * arm for a whole unit its own RTS (0x8ced8368). without it, the arm
     * branches to the function's RTS, and 1 of 9 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* copy the data of the packet whose headers end at data, once all of it is
 * in. *used gets the packet's data length once its copy function has used it
 * up, or else 0 */
static Sint32 sfmps_CopyPketData(struct sfd_handle *hn, char *data, Sint32 len,
                                 Sint32 *used)
{
    struct sfd_trn *trn;
    struct mps_handle *mps;
    struct mps_pket_hd pkethd;
    Sint32 kind;
    Sint32 number;
    Sint32 pketlen;
    Sint32 err;
    Sint32 result;

    err = 0;
    *used = 0;
    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    mps = trn->u.mps;
    if (MPS_GetPketHd(mps, &pkethd) != 0) {
        err = SFLIB_SetErr(hn, SFMPS_ERR_PKETHD);
    }
    /* @hack spelling: the reads through (&pkethd)->, which form pkethd's
     * address afresh for each (ADD #16,R3 at 0x8ced83d0, ADD #16,R14 at
     * 0x8ced83de). without it, SHC reads pkethd on the stack directly, the
     * object runs 12 bytes short, and 45 of 69 words differ */
    kind = (&pkethd)->kind;
    number = (&pkethd)->number;
    pketlen = (&pkethd)->data_len;
    if (len < pketlen) {
        return 0;
    }
    result = sfmps_CopyPketFn[kind](hn, number, data, pketlen);
    /* @hack spelling: case 1 written before case 0, which leaves case 1's arm a
     * BRA with a NOP in its slot (0x8ced840a). in the order 0, 1, the arm's
     * store fills the slot, the object runs 4 bytes short, and 21 of 69 words
     * differ */
    switch (result) {
    case 1:
        *used = pketlen;
        break;
    case 0:
        break;
    default:
        err = result;
        break;
    }
    return err;
}

/* sfmps_CopyAudio uses up a packet without a copy while the handle plays no
 * audio, and sfmps_CopyVideo while it plays no video */
static Sint32 sfmps_CopyAudio(struct sfd_handle *hn, Sint32 number, char *data,
                              Sint32 len)
{
    struct sfd_trn *trn;
    Sint32 obuf;

    if (SFSET_GetCond(hn, SFD_COND_AUDIO) == 0) {
        return 1;
    }
    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    obuf = trn->obuf[SFD_SYS_AUDIO];
    return sfmps_CopyDstBuft(hn, obuf, data, len);
}

static Sint32 sfmps_CopyVideo(struct sfd_handle *hn, Sint32 number, char *data,
                              Sint32 len)
{
    struct sfd_trn *trn;
    Sint32 obuf;

    if (SFSET_GetCond(hn, SFD_COND_VIDEO) == 0) {
        return 1;
    }
    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    obuf = trn->obuf[SFD_SYS_VIDEO];
    return sfmps_CopyDstBuft(hn, obuf, data, len);
}

static Sint32 sfmps_CopyPrvate(struct sfd_handle *hn, Sint32 number, char *data,
                               Sint32 len)
{
    struct sfd_trn *trn;
    Sint32 obuf;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    obuf = trn->obuf[SFD_SYS_USER];
    return sfmps_CopyDstBuft(hn, obuf, data, len);
}

/* a padding packet is used up without a copy */
static Sint32 sfmps_CopyPadding(struct sfd_handle *hn, Sint32 number,
                                char *data, Sint32 len)
{
    return 1;
}

/* copy the len bytes at src into ring bufno, in two runs where they wrap, but
 * only once all of them fit. it returns 1 once they are copied, 0 while they
 * don't fit, or an error */
static Sint32 sfmps_CopyDstBuft(struct sfd_handle *hn, Sint32 bufno, char *src,
                                Sint32 len)
{
    struct sfbuf_ring_inf inf;
    char *area;
    Sint32 arealen;
    char *wrap;
    Sint32 wraplen;
    Sint32 written;
    Sint32 err;

    err = SFBUF_RingGetWrite(hn, bufno, &inf);
    if (err != 0) {
        return err;
    }
    /* @hack spelling: the reads through (&inf)->, which form inf's address
     * afresh for each (ADD #16,R4 at 0x8ced8504, ADD #16,R14 at 0x8ced850c).
     * without it, SHC reads inf on the stack directly, the object runs 20 bytes
     * short, and 57 of 72 words differ */
    area = (&inf)->area;
    arealen = (&inf)->len;
    wrap = (&inf)->wrap;
    wraplen = (&inf)->wraplen;
    written = (&inf)->written;
    if (len > arealen + wraplen) {
        return 0;
    }
    if (len <= arealen) {
        sfmps_SwitchCopy(area, src, len);
    } else {
        sfmps_SwitchCopy(area, src, arealen);
        sfmps_SwitchCopy(wrap, src + arealen, len - arealen);
    }
    err = SFBUF_RingAddWrite(hn, bufno, len, written);
    if (err != 0) {
        return err;
    }
    return 1;
}

/* copy in words, halfwords or bytes: the widest unit to which both addresses
 * and the length are aligned */
static void sfmps_SwitchCopy(char *dst, char *src, Sint32 len)
{
    Sint32 bits;

    if (len != 0) {
        prefetch(src);
        bits = (Sint32)dst | (Sint32)src | len;
        if ((bits & 1) == 0) {
            if ((bits & 3) == 0) {
                UTY_MemcpyDword(dst, src, (Uint32)(len >> 2));
            } else {
                UTY_MemcpyWord((Uint16 *)dst, (const Uint16 *)src,
                               (Uint32)(len >> 1));
            }
        } else {
            UTY_MemcpyByte((Uint8 *)dst, (const Uint8 *)src, (Uint32)len);
        }
    }
}

static void sfmps_SetTermDst(struct sfd_handle *hn, Bool flag)
{
    struct sfd_trn *trn;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    SFBUF_SetTermFlg(hn, trn->obuf[SFD_SYS_AUDIO], flag);
    SFBUF_SetTermFlg(hn, trn->obuf[SFD_SYS_VIDEO], flag);
    SFBUF_SetTermFlg(hn, trn->obuf[SFD_SYS_USER], flag);
}

/* TRUE once every output has ended */
static Bool sfmps_GetTermDst(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Bool audio;
    Bool video;
    Bool user;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    audio = SFBUF_GetTermFlg(hn, trn->obuf[SFD_SYS_AUDIO]);
    video = SFBUF_GetTermFlg(hn, trn->obuf[SFD_SYS_VIDEO]);
    user = SFBUF_GetTermFlg(hn, trn->obuf[SFD_SYS_USER]);
    return audio & video & user;
}

/* whether the stream ends at the unit at hand. it ends at a unit that opens
 * with the end code, and at less than a whole unit once the input has ended */
static Bool sfmps_IsTerm(struct sfd_handle *hn, Sint32 len, Sint32 found)
{
    /* @hack spelling: hn copied to hn2 for the unit's read, which SHC makes in
     * R6 (MOV R4,R6 at 0x8ced8660). without it, the unit is read through R4,
     * and 27 of 31 words differ */
    struct sfd_handle *hn2;
    struct sfd_trn *trn;
    Sint32 ibuf;
    Sint32 unit;

    if (found == MPS_DLM_END) {
        return TRUE;
    }
    hn2 = hn;
    unit = hn2->prm.unit;
    if (len >= unit) {
        return FALSE;
    }
    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    ibuf = trn->ibuf;
    if (SFBUF_GetTermFlg(hn, ibuf) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

/* once the input ring is ready and enough input is in, mark the outputs ready.
 * then set which outputs the handle plays from the system headers */
static void sfmps_ChkPrepFlg(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Sint32 ibuf;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    ibuf = trn->ibuf;
    if (SFBUF_GetPrepFlg(hn, ibuf) == TRUE && sfmps_IsPrepEnd(hn, ibuf)) {
        SFBUF_SetPrepFlg(hn, trn->obuf[SFD_SYS_AUDIO], TRUE);
        SFBUF_SetPrepFlg(hn, trn->obuf[SFD_SYS_VIDEO], TRUE);
        SFBUF_SetPrepFlg(hn, trn->obuf[SFD_SYS_USER], TRUE);
        sfmps_ChkAvPlay(hn);
    }
}

/* whether enough input is in. that is SFD_COND_PREP_SIZE bytes written to the
 * input ring in all, or the ring's whole share of the work area if less */
static Bool sfmps_IsPrepEnd(struct sfd_handle *hn, Sint32 bufno)
{
    /* @hack spelling: hn copied to hn2 for the share's read, which SHC makes in
     * R6 (MOV R4,R6 at 0x8ced8702). without it, the share is read through R4,
     * the object runs 4 bytes short, and 35 of 37 words differ */
    struct sfd_handle *hn2;
    Sint32 *cond;
    Sint32 size;
    Sint32 bufsize;
    Sint32 need;
    Sint32 written;
    struct sfbuf_ring_inf inf;

    hn2 = hn;
    cond = hn->cond;
    size = cond[SFD_COND_PREP_SIZE];
    bufsize = hn2->prm.bufsize[SFD_BUF_INPUT];
    need = bufsize < size ? bufsize : size;
    if (SFBUF_RingGetRead(hn, bufno, &inf) == 0) {
        /* @hack spelling: written read through (&inf)->, which forms inf's
         * address for it (ADD #4,R4 at 0x8ced8724). without it, SHC reads inf
         * on the stack directly, the object runs 4 bytes short, and 18 of 37
         * words differ */
        written = (&inf)->written;
        if (written >= need) {
            return TRUE;
        }
    }
    return FALSE;
}

/* set which outputs the handle plays from the system headers the parser kept,
 * whose audio_bound and video_bound count the stream's audio and video
 * streams. an output with none is turned off. SFD_COND_END_OUT then names the
 * output left, or keeps its setting when both are left. with neither, the stage
 * records SFMPS_ERR_NOSTM, and either output's end ends the handle */
static void sfmps_ChkAvPlay(struct sfd_handle *hn)
{
    Sint32 audio;
    Sint32 video;
    struct sfd_trn *trn;
    struct mps_handle **mpsp;
    struct mps_handle *mps;
    Sint32 no;
    Sint32 has;
    Sint32 endout;
    struct mps_sys_hd syshd;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    audio = 0;
    video = 0;
    /* @hack spelling: the parser read through mpsp, its address, which has SHC
     * add u.mps's offset on its own (ADD #16,R4 at 0x8ced8782). read as
     * trn->u.mps, it takes the offset in its load, and 80 of 98 words
     * differ */
    mpsp = &trn->u.mps;
    mps = *mpsp;
    for (no = 0; no < MPS_NSYS; no++) {
        MPS_GetSysHd(mps, &syshd, no);
        /* @hack spelling: the reads through (&syshd)->, which copy syshd's
         * address, R15, for each (MOV R15,R2 at 0x8ced8792, MOV R15,R1 at
         * 0x8ced879a). without it, SHC reads syshd on the stack directly, the
         * object runs 8 bytes short, and 74 of 98 words differ */
        audio = audio > (&syshd)->audio_bound ? audio : (&syshd)->audio_bound;
        video = video > (&syshd)->video_bound ? video : (&syshd)->video_bound;
    }
    has = 0;
    if (audio == 0) {
        SFSET_SetCond(hn, SFD_COND_AUDIO, 0);
    } else {
        has = SFMPS_HAS_AUDIO;
    }
    if (video == 0) {
        SFSET_SetCond(hn, SFD_COND_VIDEO, 0);
    } else {
        has |= SFMPS_HAS_VIDEO;
    }
    switch (has) {
    case SFMPS_HAS_AUDIO:
        endout = SFD_END_OUT_AUDIO;
        break;
    case SFMPS_HAS_VIDEO:
        endout = SFD_END_OUT_VIDEO;
        break;
    case SFMPS_HAS_AUDIO | SFMPS_HAS_VIDEO:
        endout = SFSET_GetCond(hn, SFD_COND_END_OUT);
        break;
    default:
        SFLIB_SetErr(hn, SFMPS_ERR_NOSTM);
        endout = SFD_END_OUT_EITHER;
        break;
    }
    SFSET_SetCond(hn, SFD_COND_END_OUT, endout);
}

/* create the stage's parser, which records its errors as hn's. the parser
 * stays NULL on failure */
Sint32 SFMPS_Create(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    struct sfd_trn *trn;
    struct mps_handle **mpsp;
    struct mps_handle *mps;
    Sint32 err;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    mpsp = &trn->u.mps;
    sfmps_InitInf(mpsp);
    mps = MPS_Create();
    if (mps == NULL) {
        return SFLIB_SetErr(hn, SFMPS_ERR_CREATE);
    }
    err = MPS_SetErrFn(mps, sfmps_ErrFn, hn);
    if (err != 0) {
        sfmps_DestroySub(mps);
        return SFLIB_SetErr(hn, SFMPS_ERR_ERRFN);
    }
    *mpsp = mps;
    return 0;
}

static void sfmps_InitInf(struct mps_handle **mps)
{
    *mps = NULL;
}

static void sfmps_ErrFn(void *obj, Sint32 ecode)
{
    SFLIB_SetErr((struct sfd_handle *)obj, ecode);
}

Sint32 SFMPS_Destroy(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    struct sfd_trn *trn;
    struct mps_handle *mps;
    Sint32 err;

    trn = &hn->trntbl.trn[SFD_TR_SYSTEM];
    mps = trn->u.mps;
    err = sfmps_DestroySub(mps);
    if (err != 0) {
        return SFLIB_SetErr(hn, SFMPS_ERR_DESTROY);
    }
    return 0;
}

static Sint32 sfmps_DestroySub(struct mps_handle *mps)
{
    return MPS_Destroy(mps);
}

Sint32 SFMPS_Standby(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

Sint32 SFMPS_Start(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFMPS_Stop(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

Sint32 SFMPS_Pause(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    return 0;
}

/* each ring call records SFMPS_ERR_NORING as hn's error, and returns it */
Sint32 SFMPS_GetWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFMPS_ERR_NORING);
}

Sint32 SFMPS_AddWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFMPS_ERR_NORING);
}

Sint32 SFMPS_GetRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFMPS_ERR_NORING);
}

Sint32 SFMPS_AddRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFMPS_ERR_NORING);
}

const struct sfd_trif SFD_tr_sd_mps = {{
    SFMPS_Init,
    SFMPS_Finish,
    SFMPS_ExecServer,
    SFMPS_Create,
    SFMPS_Destroy,
    SFMPS_Standby,
    SFMPS_Start,
    SFMPS_Stop,
    SFMPS_Pause,
    SFMPS_GetWrite,
    SFMPS_AddWrite,
    SFMPS_GetRead,
    SFMPS_AddRead,
}};

/* MPS_PKET_OTHER has no entry, and needs none: the parser reads a packet's
 * header only for a stream of the four kinds before it */
static const sfmps_copyfn sfmps_CopyPketFn[MPS_PKET_OTHER] = {
    sfmps_CopyAudio,
    sfmps_CopyVideo,
    sfmps_CopyPrvate,
    sfmps_CopyPadding,
};
