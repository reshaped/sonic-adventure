/* Sofdec transfer stages: connect a handle's stages through its buffers, and
 * call the stages through their function tables */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

#define SFTRN_NOBUF SFD_BUF_MAX /* an ibuf or obuf that names no buffer */

/* the stage list has no system, video, audio or user stage for the input to
 * feed */
#define SFTRN_ERR_BUILD ((Sint32)0xff000302)

static Sint32 sftrn_CallTrEntry(struct sfd_trlist *trlist, Sint32 slot);
static void sftrn_InitTrData(struct sfd_trn *trn, const struct sfd_trif *trif);
static Sint32 sftrn_BuildAll(struct sfd_handle *hn,
                             const struct sfd_trif **trif);
static void sftrn_BuildSystem(struct sfd_handle *hn,
                              const struct sfd_trif **trif);
static void sftrn_BuildVideo(struct sfd_handle *hn,
                             const struct sfd_trif **trif);
static void sftrn_BuildAudio(struct sfd_handle *hn,
                             const struct sfd_trif **trif);
static void sftrn_BuildUser(struct sfd_handle *hn,
                            const struct sfd_trif **trif);
static void sftrn_ConnTrnBuf0(struct sfd_handle *hn, Sint32 trno, Sint32 bufno);
static void sftrn_ConnTrnBufV(struct sfd_handle *hn, Sint32 trno, Sint32 bufno);
static void sftrn_ConnTrnBufA(struct sfd_handle *hn, Sint32 trno, Sint32 bufno);
static void sftrn_ConnTrnBufU(struct sfd_handle *hn, Sint32 trno, Sint32 bufno);
static void sftrn_ConnTrnBuf(struct sfd_handle *hn, Sint32 trno, Sint32 out,
                             Sint32 bufno);
static void sftrn_ConnBufTrn(struct sfd_handle *hn, Sint32 bufno, Sint32 trno);

Sint32 SFTRN_Init(struct sfd_trlist *trlist, struct sfd_trlist *src)
{
    Sint32 err;

    *trlist = *src;
    /* @hack spelling: the result through err, which keeps the call a BSR
     * (0x8ced8ea2); returned directly, it becomes a tail jump to
     * sftrn_CallTrEntry and 7 of 15 words differ */
    err = sftrn_CallTrEntry(src, SFD_SLOT_INIT);
    return err;
}

Sint32 SFTRN_Finish(struct sfd_trlist *trlist)
{
    Sint32 err;

    /* @hack spelling: the result through err, which keeps the call a BSR
     * (0x8ced8eb0); returned directly, it becomes a tail jump to
     * sftrn_CallTrEntry and 6 of 6 words differ */
    err = sftrn_CallTrEntry(trlist, SFD_SLOT_FINISH);
    return err;
}

/* call slot of each function table in trlist, with no handle, up to the
 * first NULL. returns the first error, which ends the calls, or 0 */
static Sint32 sftrn_CallTrEntry(struct sfd_trlist *trlist, Sint32 slot)
{
    Sint32 err = 0;
    Sint32 i;
    const struct sfd_trif **trif = trlist->trif;
    const sfd_trfn *fn;

    for (i = 0; i < SFD_TRLIST_MAX; i++) {
        if (trif[i] == NULL) {
            break;
        }
        fn = trif[i]->fn;
        err = fn[slot](NULL, 0, 0, 0);
        if (err != 0) {
            break;
        }
    }
    return err;
}

/* reset each of trn's stages with its function table from prm's list, then
 * connect the stages and buffers. returns 0, or SFTRN_ERR_BUILD through
 * SFLIB_SetErr when no stage can take the input */
Sint32 SFTRN_InitHn(struct sfd_handle *hn, struct sfd_trn *trn,
                    struct sfd_create_param *prm, void *work)
{
    Sint32 i;
    struct sfd_trn *tr;
    struct sfd_trlist *trlist;
    const struct sfd_trif **trif;

    /* @hack spelling: trn copied into tr, which SHC moves into R4 before it
     * adds it to R13 (MOV R5,R4 at 0x8ced8f30); indexed through trn, the
     * function runs 2 bytes short and 45 of 53 words differ */
    tr = trn;
    trlist = prm->trlist;
    trif = trlist->trif;
    for (i = 0; i < SFD_TR_MAX; i++) {
        sftrn_InitTrData(&tr[i], trif[i]);
    }
    if (sftrn_BuildAll(hn, trif) != 0) {
        return SFLIB_SetErr(hn, SFTRN_ERR_BUILD);
    }
    return 0;
}

static void sftrn_InitTrData(struct sfd_trn *trn, const struct sfd_trif *trif)
{
    Sint32 i;

    trn->term = FALSE;
    trn->prep = FALSE;
    trn->trif = trif;
    trn->ibuf = SFTRN_NOBUF;
    for (i = 0; i < SFTRN_NOUT; i++) {
        trn->obuf[i] = SFTRN_NOBUF;
    }
}

/* the input feeds the system stream demultiplexer. without one, it feeds the
 * first video, audio or user stage the list has. SFSET_SetCond then turns off
 * whichever of video and audio that stage doesn't carry. returns 0, or -1 when
 * the list has none of these four */
static Sint32 sftrn_BuildAll(struct sfd_handle *hn,
                             const struct sfd_trif **trif)
{
    if (trif[SFD_TR_SYSTEM] != NULL) {
        sftrn_ConnTrnBuf0(hn, SFD_TR_INPUT, SFD_BUF_INPUT);
        sftrn_BuildSystem(hn, trif);
    } else if (trif[SFD_TR_VIDEO] != NULL) {
        sftrn_ConnTrnBuf0(hn, SFD_TR_INPUT, SFD_BUF_VIDEO);
        sftrn_BuildVideo(hn, trif);
        SFSET_SetCond(hn, SFD_COND_AUDIO, 0);
    } else if (trif[SFD_TR_AUDIO] != NULL) {
        sftrn_ConnTrnBuf0(hn, SFD_TR_INPUT, SFD_BUF_AUDIO);
        sftrn_BuildAudio(hn, trif);
        SFSET_SetCond(hn, SFD_COND_VIDEO, 0);
    } else if (trif[SFD_TR_USER] != NULL) {
        sftrn_ConnTrnBuf0(hn, SFD_TR_INPUT, SFD_BUF_USER);
        sftrn_BuildUser(hn, trif);
        SFSET_SetCond(hn, SFD_COND_AUDIO, 0);
        SFSET_SetCond(hn, SFD_COND_VIDEO, 0);
    } else {
        return -1;
    }
    return 0;
}

static void sftrn_BuildSystem(struct sfd_handle *hn,
                              const struct sfd_trif **trif)
{
    sftrn_ConnBufTrn(hn, SFD_BUF_INPUT, SFD_TR_SYSTEM);
    if (trif[SFD_TR_VIDEO] != NULL) {
        sftrn_ConnTrnBufV(hn, SFD_TR_SYSTEM, SFD_BUF_VIDEO);
        sftrn_BuildVideo(hn, trif);
    }
    if (trif[SFD_TR_AUDIO] != NULL) {
        sftrn_ConnTrnBufA(hn, SFD_TR_SYSTEM, SFD_BUF_AUDIO);
        sftrn_BuildAudio(hn, trif);
    }
    if (trif[SFD_TR_USER] != NULL) {
        sftrn_ConnTrnBufU(hn, SFD_TR_SYSTEM, SFD_BUF_USER);
        sftrn_BuildUser(hn, trif);
    }
}

/* the video stage decodes the video buffer into frames, which reach the video
 * output through the post-decoder stage when the list has one */
static void sftrn_BuildVideo(struct sfd_handle *hn,
                             const struct sfd_trif **trif)
{
    sftrn_ConnBufTrn(hn, SFD_BUF_VIDEO, SFD_TR_VIDEO);
    sftrn_ConnTrnBuf0(hn, SFD_TR_VIDEO, SFD_BUF_FRAME);
    if (trif[SFD_TR_VIDEO_POST] != NULL) {
        sftrn_ConnBufTrn(hn, SFD_BUF_FRAME, SFD_TR_VIDEO_POST);
        sftrn_ConnTrnBuf0(hn, SFD_TR_VIDEO_POST, SFD_BUF_FRAME_POST);
        sftrn_ConnBufTrn(hn, SFD_BUF_FRAME_POST, SFD_TR_VIDEO_OUT);
    } else {
        sftrn_ConnBufTrn(hn, SFD_BUF_FRAME, SFD_TR_VIDEO_OUT);
    }
}

static void sftrn_BuildAudio(struct sfd_handle *hn,
                             const struct sfd_trif **trif)
{
    sftrn_ConnBufTrn(hn, SFD_BUF_AUDIO, SFD_TR_AUDIO);
    sftrn_ConnTrnBuf0(hn, SFD_TR_AUDIO, SFD_BUF_PCM);
    if (trif[SFD_TR_AUDIO_POST] != NULL) {
        sftrn_ConnBufTrn(hn, SFD_BUF_PCM, SFD_TR_AUDIO_POST);
        sftrn_ConnTrnBuf0(hn, SFD_TR_AUDIO_POST, SFD_BUF_PCM_POST);
        sftrn_ConnBufTrn(hn, SFD_BUF_PCM_POST, SFD_TR_AUDIO_OUT);
    } else {
        sftrn_ConnBufTrn(hn, SFD_BUF_PCM, SFD_TR_AUDIO_OUT);
    }
}

static void sftrn_BuildUser(struct sfd_handle *hn, const struct sfd_trif **trif)
{
    sftrn_ConnBufTrn(hn, SFD_BUF_USER, SFD_TR_USER);
}

/* sftrn_ConnTrnBuf0 has stage trno write buffer bufno through output 0, a
 * stage's first. sftrn_ConnTrnBufV, sftrn_ConnTrnBufA and sftrn_ConnTrnBufU
 * have it write bufno through the system stage's video, audio and user
 * outputs */
static void sftrn_ConnTrnBuf0(struct sfd_handle *hn, Sint32 trno, Sint32 bufno)
{
    sftrn_ConnTrnBuf(hn, trno, 0, bufno);
}

static void sftrn_ConnTrnBufV(struct sfd_handle *hn, Sint32 trno, Sint32 bufno)
{
    sftrn_ConnTrnBuf(hn, trno, SFD_SYS_VIDEO, bufno);
}

static void sftrn_ConnTrnBufA(struct sfd_handle *hn, Sint32 trno, Sint32 bufno)
{
    sftrn_ConnTrnBuf(hn, trno, SFD_SYS_AUDIO, bufno);
}

static void sftrn_ConnTrnBufU(struct sfd_handle *hn, Sint32 trno, Sint32 bufno)
{
    sftrn_ConnTrnBuf(hn, trno, SFD_SYS_USER, bufno);
}

static void sftrn_ConnTrnBuf(struct sfd_handle *hn, Sint32 trno, Sint32 out,
                             Sint32 bufno)
{
    struct sftrn_tbl *trntbl;
    struct sfd_trn *trn;
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;

    trntbl = &hn->trntbl;
    trn = &trntbl->trn[trno];
    trn->obuf[out] = bufno;
    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    buf->wtrno = trno;
}

/* have stage trno read buffer bufno */
static void sftrn_ConnBufTrn(struct sfd_handle *hn, Sint32 bufno, Sint32 trno)
{
    struct sfbuf_tbl *buftbl;
    struct sfbuf *buf;
    struct sftrn_tbl *trntbl;
    struct sfd_trn *trn;

    buftbl = &hn->buftbl;
    buf = &buftbl->buf[bufno];
    buf->rtrno = trno;
    trntbl = &hn->trntbl;
    trn = &trntbl->trn[trno];
    trn->ibuf = bufno;
}

/* call slot of each of the handle's stages, with hn, until one returns an
 * error. the video decoder, post-decoder and output stages are skipped while
 * SFD_COND_VIDEO is 0, and the audio ones while SFD_COND_AUDIO is. returns
 * that error or 0 */
Sint32 SFTRN_CallTrSetup(struct sfd_handle *hn, Sint32 slot)
{
    Sint32 err = 0;
    Sint32 i;
    struct sfd_trn *trns;
    struct sfd_trn *trn;
    Sint32 video;
    Sint32 audio;
    const struct sfd_trif *trif;
    const sfd_trfn *fn;

    /* @hack spelling: the stages' address set in trns, then copied into trn,
     * which SHC forms in R4 and moves to R11 (ADD R12,R4 at 0x8ced91e0, MOV
     * R4,R11 at 0x8ced91e2); set in trn alone, it forms it in R11 and 74 of
     * 90 words differ */
    trns = hn->trntbl.trn;
    trn = trns;
    video = SFSET_GetCond(hn, SFD_COND_VIDEO);
    audio = SFSET_GetCond(hn, SFD_COND_AUDIO);
    for (i = 0; i < SFD_TR_MAX; i++, trn++) {
        if (video == 0 && (i == SFD_TR_VIDEO || i == SFD_TR_VIDEO_POST ||
                           i == SFD_TR_VIDEO_OUT)) {
            continue;
        }
        if (audio == 0 && (i == SFD_TR_AUDIO || i == SFD_TR_AUDIO_POST ||
                           i == SFD_TR_AUDIO_OUT)) {
            continue;
        }
        trif = trn->trif;
        if (trif == NULL) {
            continue;
        }
        fn = trif->fn;
        err = fn[slot](hn, 0, 0, 0);
        if (err != 0) {
            break;
        }
    }
    return err;
}

/* call slot of stage trno with hn, arg1 and arg2, and return its result. for a
 * stage the handle lacks, it returns 0 */
Sint32 SFTRN_CallTrtTrif(struct sfd_handle *hn, Sint32 trno, Sint32 slot,
                         Sint32 arg1, Sint32 arg2)
{
    struct sftrn_tbl *trntbl;
    const struct sfd_trif *trif;
    const sfd_trfn *fn;
    Sint32 err;

    trntbl = &hn->trntbl;
    trif = trntbl->trn[trno].trif;
    if (trif == NULL) {
        return 0;
    }
    fn = trif->fn;
    /* @hack spelling: the result through err, which keeps the call a JSR
     * (0x8ced92ca); returned directly, it becomes a tail jump and 31 of 35
     * words differ */
    err = fn[slot](hn, arg1, arg2, 0);
    return err;
}

void SFTRN_SetPrepFlg(struct sfd_handle *hn, Sint32 trno, Bool flag)
{
    struct sftrn_tbl *trntbl;
    struct sfd_trn *trn;

    trntbl = &hn->trntbl;
    trn = &trntbl->trn[trno];
    trn->prep = flag;
}

Bool SFTRN_GetPrepFlg(struct sfd_handle *hn, Sint32 trno)
{
    struct sftrn_tbl *trntbl;
    struct sfd_trn *trn;

    trntbl = &hn->trntbl;
    trn = &trntbl->trn[trno];
    return trn->prep;
}

void SFTRN_SetTermFlg(struct sfd_handle *hn, Sint32 trno, Bool flag)
{
    struct sftrn_tbl *trntbl;
    struct sfd_trn *trn;

    trntbl = &hn->trntbl;
    trn = &trntbl->trn[trno];
    trn->term = flag;
}

Bool SFTRN_GetTermFlg(struct sfd_handle *hn, Sint32 trno)
{
    struct sftrn_tbl *trntbl;
    struct sfd_trn *trn;

    trntbl = &hn->trntbl;
    trn = &trntbl->trn[trno];
    return trn->term;
}
