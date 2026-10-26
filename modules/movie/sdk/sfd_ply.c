/* the Sofdec player: the server that steps each handle, and the calls that
 * create, start and stop a handle, feed it and take its pictures */
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/sfd.h"

/* the word after the default conditions. SFPLY_Init records SFPLY_ERR_COND
 * for any other */
#define SFPLY_COND_GUARD 0x5a5a5a5a

/* SFD_COND_END_TIME for a handle with no end time */
#define SFPLY_NO_END_TIME (-4)

/* error codes for a call on an invalid handle */
#define SFD_ERR_DESTROY ((Sint32)0xff000109)
#define SFD_ERR_START ((Sint32)0xff00010a)
#define SFD_ERR_STOP ((Sint32)0xff00010b)
#define SFD_ERR_GETWRITE ((Sint32)0xff00010c)
#define SFD_ERR_INPUT ((Sint32)0xff00010d)
#define SFD_ERR_GETFRM ((Sint32)0xff00010e)
#define SFD_ERR_RELFRM ((Sint32)0xff00010f)
/* errors for default conditions whose guard is wrong, and for a reset that
 * can't create the handle's stages again */
#define SFPLY_ERR_COND ((Sint32)0xff000201)
#define SFPLY_ERR_RESET ((Sint32)0xff000202)

static void sfply_ChkCondDfl(void);
static void sfply_ExecOne(struct sfd_handle *hn);
static Sint32 sfply_TrExecServer(struct sfd_handle *hn);
static Sint32 sfply_StatStop(struct sfd_handle *hn);
static Sint32 sfply_StatPrep(struct sfd_handle *hn);
static Bool sfply_IsPrepEnd(struct sfd_handle *hn);
static Sint32 sfply_StatStby(struct sfd_handle *hn);
static Sint32 sfply_StatPlay(struct sfd_handle *hn);
static Sint32 sfply_StatPause(struct sfd_handle *hn);
static Sint32 sfply_StatFin(struct sfd_handle *hn);
static Sint32 sfply_ChkFin(struct sfd_handle *hn);
static Bool sfply_IsEtime(struct sfd_handle *hn);
static Bool sfply_IsEtrg(struct sfd_handle *hn);
static Sint32 sfply_Fin(struct sfd_handle *hn);
static struct sfd_handle *sfply_SearchFreeHn(void);
static struct sfd_handle *
sfply_InitHn(struct sfd_handle *hn, struct sfd_create_param *prm, void *work);
static void sfply_InitMvInf(struct sfd_mvinf *mvinf);
static void sfply_InitPlyInf(struct sfd_plyinf *plyinf);
static Sint32 sfply_TrCreate(struct sfd_handle *hn);
static Sint32 sfply_TrDestroy(struct sfd_handle *hn);
static Sint32 sfply_Start(struct sfd_handle *hn);
static Sint32 sfply_TrStart(struct sfd_handle *hn);
static Sint32 sfply_TrStop(struct sfd_handle *hn);
/* nothing in MOVIE.BIN calls sfply_ResetHn. the object holds it at 0x8ced4934.
 * it is extern because SHC drops an uncalled static function from the object */
Sint32 sfply_ResetHn(struct sfd_handle *hn);

/* each new handle's conditions, by SFD_COND_* index ten to a row, then
 * SFPLY_COND_GUARD. a handle decodes I, P and B pictures, plays video and
 * audio, and skips a late picture. its time comes from the vsyncs, and it has
 * no end time. its outputs wait for 100 KB of input, and its video output for
 * 3 pictures. either output's end ends it. a picture takes 4000/59939 second
 * to decode */
/* clang-format off */
static const Sint32 sfply_cond_dfl[SFD_COND_MAX + 1] = {
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 1, 1, 0, SFTIM_SRC_VSYNC, 0, 1, -3, 1,
    SFPLY_NO_END_TIME, 1, 102400, 3, 4096, SFD_END_OUT_EITHER, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    -30000, -14000, 4000, 59939,
    SFPLY_COND_GUARD,
};
/* clang-format on */

void SFPLY_Init(void)
{
    sfply_ChkCondDfl();
}

static void sfply_ChkCondDfl(void)
{
    if (sfply_cond_dfl[SFD_COND_MAX] != SFPLY_COND_GUARD) {
        SFLIB_SetErr(NULL, SFPLY_ERR_COND);
    }
}

void SFD_VbIn(void)
{
    SFTIM_CntupVbIn();
}

void SFD_VbOut(void)
{
}

/* step each handle, then return the server state SFPLY_DecideSvrStat
 * decides */
Sint32 SFD_ExecServer(void)
{
    Sint32 i;
    Sint32 stat;
    Sint32 nhn;
    struct sfd_handle *hns;

    /* @hack spelling: nhn and hn read through SFLIB_libwork's address, which
     * SHC loads from the pool for each (0x8ced425a, 0x8ced4260); as
     * SFLIB_libwork.nhn and .hn, 24 of 31 words differ */
    nhn = (&SFLIB_libwork)->nhn;
    hns = (&SFLIB_libwork)->hn;
    for (i = 0; i < nhn; i++) {
        sfply_ExecOne(&hns[i]);
    }
    /* @hack spelling: the result through stat, which keeps the call a BSR
     * (0x8ced427a); returned directly, it becomes a tail jump to
     * SFPLY_DecideSvrStat and 13 of 31 words differ */
    stat = SFPLY_DecideSvrStat();
    return stat;
}

/* step a handle that is stopped, preparing, standing by, playing or paused.
 * its stages run first, unless it is stopped. then its state's step sets its
 * next state */
static void sfply_ExecOne(struct sfd_handle *hn)
{
    Sint32 stat;

    stat = hn->stat;
    if (stat == SFD_STAT_STOP || stat == SFD_STAT_PREP ||
        stat == SFD_STAT_STBY || stat == SFD_STAT_PLAY ||
        stat == SFD_STAT_PAUSE) {
        if (stat == SFD_STAT_PREP || stat == SFD_STAT_STBY ||
            stat == SFD_STAT_PLAY || stat == SFD_STAT_PAUSE) {
            sfply_TrExecServer(hn);
        }
        stat = hn->stat;
        switch (stat) {
        case SFD_STAT_FREE:
            break;
        case SFD_STAT_STOP:
            stat = sfply_StatStop(hn);
            break;
        case SFD_STAT_PREP:
            stat = sfply_StatPrep(hn);
            break;
        case SFD_STAT_STBY:
            stat = sfply_StatStby(hn);
            break;
        case SFD_STAT_PLAY:
            stat = sfply_StatPlay(hn);
            break;
        case SFD_STAT_PAUSE:
            stat = sfply_StatPause(hn);
            break;
        case SFD_STAT_FIN:
            stat = sfply_StatFin(hn);
            break;
        }
        hn->stat = stat;
    }
}

static Sint32 sfply_TrExecServer(struct sfd_handle *hn)
{
    return SFTRN_CallTrSetup(hn, SFD_SLOT_EXEC_SERVER);
}

/* a stopped handle starts preparing on a request for any state from
 * SFD_STAT_PREP to SFD_STAT_FIN */
static Sint32 sfply_StatStop(struct sfd_handle *hn)
{
    Sint32 stat;
    Sint32 req;

    stat = hn->stat;
    req = hn->req;
    switch (req) {
    case SFD_STAT_PREP:
    case SFD_STAT_STBY:
    case SFD_STAT_PLAY:
    case SFD_STAT_PAUSE:
    case SFD_STAT_FIN:
        stat = SFD_STAT_PREP;
        break;
    }
    return stat;
}

/* once its outputs are ready, a preparing handle stands by on a request for
 * SFD_STAT_STBY. it starts playing on a request for SFD_STAT_PLAY,
 * SFD_STAT_PAUSE or SFD_STAT_FIN */
static Sint32 sfply_StatPrep(struct sfd_handle *hn)
{
    Sint32 stat;
    Sint32 req;

    stat = hn->stat;
    req = hn->req;
    if (sfply_IsPrepEnd(hn)) {
        switch (req) {
        case SFD_STAT_PREP:
            stat = SFD_STAT_PREP;
            break;
        case SFD_STAT_STBY:
            stat = SFD_STAT_STBY;
            break;
        case SFD_STAT_PLAY:
        case SFD_STAT_PAUSE:
        case SFD_STAT_FIN:
            sfply_TrStart(hn);
            stat = SFD_STAT_PLAY;
            break;
        }
    }
    return stat;
}

/* TRUE once each output the handle plays is ready */
static Bool sfply_IsPrepEnd(struct sfd_handle *hn)
{
    Bool video;
    Bool audio;

    if (SFSET_GetCond(hn, SFD_COND_VIDEO) == 0) {
        video = TRUE;
    } else {
        video = SFTRN_GetPrepFlg(hn, SFD_TR_VIDEO_OUT);
    }
    if (SFSET_GetCond(hn, SFD_COND_AUDIO) == 0) {
        audio = TRUE;
    } else {
        audio = SFTRN_GetPrepFlg(hn, SFD_TR_AUDIO_OUT);
    }
    if (video && audio) {
        return TRUE;
    }
    return FALSE;
}

static Sint32 sfply_StatStby(struct sfd_handle *hn)
{
    Sint32 stat;
    Sint32 req;

    stat = hn->stat;
    req = hn->req;
    switch (req) {
    case SFD_STAT_PREP:
        stat = SFD_STAT_PREP;
        break;
    case SFD_STAT_STBY:
        stat = SFD_STAT_STBY;
        break;
    case SFD_STAT_PLAY:
    case SFD_STAT_PAUSE:
    case SFD_STAT_FIN:
        sfply_TrStart(hn);
        stat = SFD_STAT_PLAY;
        break;
    }
    return stat;
}

/* a playing handle finishes on a request for SFD_STAT_FIN, at its end time,
 * or once the outputs SFD_COND_END_OUT names have ended. the last two stop it
 * first, and it stays playing if the stop fails */
static Sint32 sfply_StatPlay(struct sfd_handle *hn)
{
    Sint32 stat;
    Sint32 req;

    if (sfply_ChkFin(hn) != 0) {
        return hn->stat;
    }
    stat = hn->stat;
    req = hn->req;
    switch (req) {
    case SFD_STAT_PREP:
    case SFD_STAT_STBY:
    case SFD_STAT_PLAY:
    case SFD_STAT_PAUSE:
        break;
    case SFD_STAT_FIN:
        stat = SFD_STAT_FIN;
        break;
    }
    return stat;
}

static Sint32 sfply_StatPause(struct sfd_handle *hn)
{
    return hn->stat;
}

static Sint32 sfply_StatFin(struct sfd_handle *hn)
{
    return hn->stat;
}

static Sint32 sfply_ChkFin(struct sfd_handle *hn)
{
    if (sfply_IsEtime(hn) || sfply_IsEtrg(hn)) {
        return sfply_Fin(hn);
    }
    return 0;
}

/* TRUE once the handle's time is past SFD_COND_END_TIME, if it has an end
 * time */
static Bool sfply_IsEtime(struct sfd_handle *hn)
{
    Sint32 etime;
    Sint32 etunit;
    Sint32 time;
    Sint32 tunit;

    etime = hn->cond[SFD_COND_END_TIME];
    etunit = hn->cond[SFD_COND_END_TUNIT];
    if (etime == SFPLY_NO_END_TIME) {
        return FALSE;
    }
    SFTIM_GetTime(hn, &time, &tunit);
    if (time < 0) {
        return FALSE;
    }
    if (UTY_CmpTime(time, tunit, etime, etunit)) {
        return FALSE;
    }
    return TRUE;
}

/* TRUE once the outputs SFD_COND_END_OUT names have ended */
static Bool sfply_IsEtrg(struct sfd_handle *hn)
{
    Bool fin;
    Bool video;
    Bool audio;
    Sint32 endout;

    fin = FALSE;
    video = SFTRN_GetTermFlg(hn, SFD_TR_VIDEO_OUT);
    audio = SFTRN_GetTermFlg(hn, SFD_TR_AUDIO_OUT);
    /* @hack spelling: the condition through endout, which SHC copies into R4
     * (MOV R0,R4 at 0x8ced4542); switched on SFSET_GetCond's result, 24 of 47
     * words differ */
    endout = SFSET_GetCond(hn, SFD_COND_END_OUT);
    switch (endout) {
    case SFD_END_OUT_AUDIO:
        fin = audio;
        break;
    case SFD_END_OUT_VIDEO:
        fin = video;
        break;
    case SFD_END_OUT_EITHER:
        fin = video | audio;
        break;
    case SFD_END_OUT_BOTH:
        fin = video & audio;
        break;
    }
    return fin;
}

/* stop the handle and request SFD_STAT_FIN. returns 0, or the stop's error
 * with nothing requested */
static Sint32 sfply_Fin(struct sfd_handle *hn)
{
    Sint32 err;

    /* @hack spelling: err assigned in the test, which SHC tests in R0 and
     * copies to R4 in the branch's delay slot (TST R0,R0 at 0x8ced457c, MOV
     * R0,R4 at 0x8ced4580); assigned first, err is copied to R4 and tested
     * there, and 3 of 20 words differ */
    if ((err = sfply_TrStop(hn)) != 0) {
        return err;
    }
    hn->req = SFD_STAT_FIN;
    return 0;
}

/* decide the server state, record it in SFLIB_libwork.svrstat and return it.
 * it is SFD_SVR_BUSY while any handle runs, and SFD_SVR_FIN once every handle
 * in use has finished or failed. with no handle in use, it is SFD_SVR_IDLE */
Sint32 SFPLY_DecideSvrStat(void)
{
    Sint32 count[SFD_SVR_FIN + 1];
    Sint32 i;
    Sint32 no;
    Sint32 svrstat;
    Sint32 stat;
    Sint32 nhn;
    struct sfd_handle *hns;

    count[SFD_SVR_IDLE] = 0;
    count[SFD_SVR_BUSY] = 0;
    count[SFD_SVR_FIN] = 0;
    /* @hack spelling: nhn and hn read through SFLIB_libwork's address, which
     * SHC loads from the pool for each (0x8ced45a0, 0x8ced45a8); as
     * SFLIB_libwork.nhn and .hn, 66 of 68 words differ */
    nhn = (&SFLIB_libwork)->nhn;
    hns = (&SFLIB_libwork)->hn;
    for (i = 0; i < nhn; i++) {
        stat = hns[i].stat;
        if (stat == SFD_STAT_FREE) {
            no = SFD_SVR_IDLE;
        } else if (stat == SFD_STAT_FIN || stat < 0) {
            no = SFD_SVR_FIN;
        } else {
            no = SFD_SVR_BUSY;
        }
        count[no]++;
    }
    if (count[SFD_SVR_BUSY] != 0) {
        svrstat = SFD_SVR_BUSY;
    } else if (count[SFD_SVR_FIN] != 0) {
        svrstat = SFD_SVR_FIN;
    } else {
        svrstat = SFD_SVR_IDLE;
    }
    /* @hack spelling: the store through SFLIB_libwork's address, at an offset
     * in R0 (MOV #80,R0 at 0x8ced4632); as SFLIB_libwork.svrstat, SHC folds the
     * offset into the pool word and 6 of 68 words differ */
    (&SFLIB_libwork)->svrstat = svrstat;
    return svrstat;
}

/* a free handle, set up from prm and work, or NULL. prm->work is rounded up
 * in place to a 32-byte boundary. NULL means no handle is free, or the
 * handle's stages can't be created */
struct sfd_handle *SFD_Create(struct sfd_create_param *prm, void *work)
{
    struct sfd_handle *hn;

    if ((hn = sfply_SearchFreeHn()) == NULL) {
        return NULL;
    }
    /* @hack spelling: the result through hn, which keeps the call a BSR
     * (0x8ced4658); returned directly, it becomes a tail jump to
     * sfply_InitHn and 6 of 19 words differ */
    hn = sfply_InitHn(hn, prm, work);
    return hn;
}

static struct sfd_handle *sfply_SearchFreeHn(void)
{
    Sint32 i;
    struct sfd_handle *hn;
    Sint32 nhn;
    struct sfd_handle *hns;

    /* @hack spelling: nhn and hn read through SFLIB_libwork's address, which
     * SHC loads from the pool for each (0x8ced4664, 0x8ced4668); as
     * SFLIB_libwork.nhn and .hn, 23 of 24 words differ */
    nhn = (&SFLIB_libwork)->nhn;
    hns = (&SFLIB_libwork)->hn;
    for (i = 0; i < nhn; i++) {
        hn = &hns[i];
        if (hn->stat == SFD_STAT_FREE) {
            return hn;
        }
    }
    return NULL;
}

/* clear hn and set it up from prm and work, stopped, with the default
 * conditions. returns hn, or NULL with hn freed again when its stages can't be
 * created */
static struct sfd_handle *sfply_InitHn(struct sfd_handle *hn,
                                       struct sfd_create_param *prm, void *work)
{
    UTY_MemsetDword(hn, 0, sizeof(struct sfd_handle) / sizeof(Uint32));
    prm->work = (char *)(((Sint32)prm->work + 31) & ~31);
    hn->prm = *prm;
    hn->work = work;
    hn->stat = SFD_STAT_STOP;
    hn->req = SFD_STAT_STOP;
    sfply_InitMvInf(&hn->mvinf);
    sfply_InitPlyInf(&hn->plyinf);
    SFLIB_InitErrInf(&hn->err);
    UTY_MemcpyDword(hn->cond, sfply_cond_dfl, SFD_COND_MAX);
    SFTIM_InitHn(hn, &hn->tim);
    SFBUF_InitHn(hn, &hn->buftbl, prm);
    SFTRN_InitHn(hn, hn->trntbl.trn, prm, work);
    if (sfply_TrCreate(hn) != 0) {
        hn->stat = SFD_STAT_FREE;
        hn->req = SFD_STAT_FREE;
        hn = NULL;
    }
    return hn;
}

static void sfply_InitMvInf(struct sfd_mvinf *mvinf)
{
    mvinf->width = 0;
    mvinf->height = 0;
    mvinf->mb_width = 0;
    mvinf->mb_height = 0;
    mvinf->bitrate = 0;
}

static void sfply_InitPlyInf(struct sfd_plyinf *plyinf)
{
    plyinf->ndec = 0;
    plyinf->nskip = 0;
}

/* count npic more decoded pictures, then call the SFD_COND_DEC_PIC_FN
 * function, if one is set, with arg and the counts */
void SFPLY_AddDecPic(struct sfd_handle *hn, Sint32 npic, Sint32 arg)
{
    struct sfd_plyinf *plyinf;
    sfd_picfn func;

    plyinf = &hn->plyinf;
    plyinf->ndec += npic;
    func = (sfd_picfn)SFSET_GetCond(hn, SFD_COND_DEC_PIC_FN);
    if (func != NULL) {
        func(hn, arg, plyinf);
    }
}

void SFPLY_AddSkipPic(struct sfd_handle *hn, Sint32 npic, Sint32 arg)
{
    struct sfd_plyinf *plyinf;
    sfd_picfn func;

    plyinf = &hn->plyinf;
    plyinf->nskip += npic;
    func = (sfd_picfn)SFSET_GetCond(hn, SFD_COND_SKIP_PIC_FN);
    if (func != NULL) {
        func(hn, arg, plyinf);
    }
}

static Sint32 sfply_TrCreate(struct sfd_handle *hn)
{
    return SFTRN_CallTrSetup(hn, SFD_SLOT_CREATE);
}

Sint32 SFD_Destroy(struct sfd_handle *hn)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_DESTROY);
    }
    return sfply_TrDestroy(hn);
}

static Sint32 sfply_TrDestroy(struct sfd_handle *hn)
{
    Sint32 err;

    err = SFTRN_CallTrSetup(hn, SFD_SLOT_DESTROY);
    hn->stat = SFD_STAT_FREE;
    hn->req = SFD_STAT_FREE;
    return err;
}

Sint32 SFD_Start(struct sfd_handle *hn)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_START);
    }
    return sfply_Start(hn);
}

static Sint32 sfply_Start(struct sfd_handle *hn)
{
    hn->req = SFD_STAT_PLAY;
    return 0;
}

/* start the audio output stage, which is also the only stage sfply_TrStop
 * stops. returns 0 or the stage's error */
static Sint32 sfply_TrStart(struct sfd_handle *hn)
{
    Sint32 err;

    if ((err = SFTRN_CallTrtTrif(hn, SFD_TR_AUDIO_OUT, SFD_SLOT_START, 0, 0)) !=
        0) {
        return err;
    }
    return 0;
}

Sint32 SFD_Stop(struct sfd_handle *hn)
{
    Sint32 err;

    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_STOP);
    }
    if (hn->stat != SFD_STAT_STOP) {
        if ((err = sfply_TrStop(hn)) != 0) {
            return err;
        }
    }
    return 0;
}

static Sint32 sfply_TrStop(struct sfd_handle *hn)
{
    Sint32 err;

    if (hn->stat == SFD_STAT_PLAY) {
        if ((err = SFTRN_CallTrtTrif(hn, SFD_TR_AUDIO_OUT, SFD_SLOT_STOP, 0,
                                     0)) != 0) {
            return err;
        }
    }
    hn->stat = SFD_STAT_STOP;
    hn->req = SFD_STAT_STOP;
    return 0;
}

/* reset the handle to stopped, keeping its conditions. when hn->work is NULL
 * and SFD_COND_REFEED is set, SFD_AddWritePtr then adds the old input's
 * written count to the new input's. SFD_TermInput then ends the new input.
 * returns 0 or an error code */
Sint32 sfply_ResetHn(struct sfd_handle *hn)
{
    /* @hack spelling: prm, inf and cond declared in the reverse of their frame
     * order, which puts cond at sp+4, inf at sp+180 and prm at sp+208
     * (0x8ced4984, 0x8ced495c, 0x8ced493c); declared in frame order, 30 of 95
     * words differ */
    Sint32 len;
    struct sfd_create_param prm;
    struct sfbuf_ring_inf inf;
    Sint32 cond[SFD_COND_MAX];
    void *work;
    Sint32 refeed;
    Sint32 err;

    prm = hn->prm;
    work = hn->work;
    if (work == NULL) {
        refeed = hn->cond[SFD_COND_REFEED];
        if (refeed != 0) {
            SFD_GetWritePtr(hn, &inf);
            /* @hack spelling: written read through inf's address, which SHC
             * forms in R3 (0x8ced4966); as inf.written, 73 of 95 words
             * differ */
            len = (&inf)->written;
        }
    }
    if ((err = sfply_TrDestroy(hn)) != 0) {
        return err;
    }
    UTY_MemcpyDword(cond, hn->cond, SFD_COND_MAX);
    hn = sfply_InitHn(hn, &prm, work);
    if (hn == NULL) {
        return SFLIB_SetErr(NULL, SFPLY_ERR_RESET);
    }
    UTY_MemcpyDword(hn->cond, cond, SFD_COND_MAX);
    if (work == NULL && refeed != 0) {
        err = SFD_GetWritePtr(hn, &inf);
        if (err != 0) {
            return err;
        }
        /* @hack spelling: written read through inf's address, which SHC forms
         * in R6 (0x8ced49ca); as inf.written, 35 of 95 words differ */
        err = SFD_AddWritePtr(hn, len, (&inf)->written);
        if (err != 0) {
            return err;
        }
        SFD_TermInput(hn);
    }
    return 0;
}

Sint32 SFD_GetWritePtr(struct sfd_handle *hn, struct sfbuf_ring_inf *inf)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_GETWRITE);
    }
    return SFTRN_CallTrtTrif(hn, SFD_TR_INPUT, SFD_SLOT_GET_WRITE, (Sint32)inf,
                             0);
}

Sint32 SFD_AddWritePtr(struct sfd_handle *hn, Sint32 len, Sint32 written)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_INPUT);
    }
    return SFTRN_CallTrtTrif(hn, SFD_TR_INPUT, SFD_SLOT_ADD_WRITE, len,
                             written);
}

/* mark the input complete: the caller has written all of it */
Sint32 SFD_TermInput(struct sfd_handle *hn)
{
    Sint32 obuf;
    struct sfd_trn *trn;

    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_INPUT);
    }
    /* @hack spelling: the stage through trn, which has SHC add its offset to
     * hn in R4 (ADD R14,R4 at 0x8ced4aaa) before it reads obuf; read through
     * hn, the offsets fold into one and 14 of 25 words differ */
    trn = &hn->trntbl.trn[SFD_TR_INPUT];
    obuf = trn->obuf[0];
    SFBUF_SetTermFlg(hn, obuf, TRUE);
    return 0;
}

Sint32 SFD_GetFrm(struct sfd_handle *hn, struct sfd_frm **frm)
{
    *frm = NULL;
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_GETFRM);
    }
    return SFTRN_CallTrtTrif(hn, SFD_TR_VIDEO_OUT, SFD_SLOT_GET_READ,
                             (Sint32)frm, 0);
}

Sint32 SFD_RelFrm(struct sfd_handle *hn, struct sfd_frm *frm)
{
    if (SFLIB_CheckHn(hn) != 0) {
        return SFLIB_SetErr(NULL, SFD_ERR_RELFRM);
    }
    return SFTRN_CallTrtTrif(hn, SFD_TR_VIDEO_OUT, SFD_SLOT_ADD_READ,
                             (Sint32)frm, 0);
}
