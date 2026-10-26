/* the Sofdec audio decoder stage, through ADX talk. the stream's audio goes
 * from its ring into a stream joint, for an ADX talk player to play */
#include <sg_xpt.h>
#include <sj.h>
#include <sofdec.h>

#include "movie/sdk/sfd.h"

/* the largest chunk the stage asks of the joint for one copy, and the most it
 * copies at once */
#define SFADXT_CHUNK_MAX 0x24000
#define SFADXT_COPY_MAX 0x19000

/* the player's server frequency, set with ADXT_SetSvrFreq: the times a second
 * its server runs */
#define SFADXT_SVRFREQ 25

/* a call for a ring the stage doesn't have */
#define SFADXT_ERR_NORING ((Sint32)0xff000c03)
/* a player, or a joint, that can't be created */
#define SFADXT_ERR_CREATE ((Sint32)0xff000c04)
#define SFADXT_ERR_SJ ((Sint32)0xff000c05)
/* a handle created before a ring and a work area are set */
#define SFADXT_ERR_PARA ((Sint32)0xff000c06)

static Sint32 sfadxt_SupplyData(struct sfd_handle *hn);
static void sfadxt_CopyData(struct sfd_handle *hn, char *data, Sint32 len,
                            Sint32 *copied);
static void sfadxt_ChkPrepFlg(struct sfd_handle *hn);
static Bool sfadxt_IsPrepEnd(struct sfd_handle *hn);
static void sfadxt_ChkTermFlg(struct sfd_handle *hn);
static Sint32 sfadxt_RingGetRead(struct sfd_handle *hn, char **data,
                                 Sint32 *len);
static Sint32 sfadxt_CopyFlap(struct sfd_handle *hn, struct sfbuf_ring_inf *inf,
                              Sint32 flapsize, Sint32 *flapped, char **end);
static Sint32 sfadxt_RingAddRead(struct sfd_handle *hn, Sint32 len);
static Bool sfadxt_GetTermSrc(struct sfd_handle *hn);
static Bool sfadxt_GetTermDst(struct sfd_handle *hn);
static void sfadxt_SetTermDst(struct sfd_handle *hn, Bool flag);
static Sint32 sfadxt_InitInf(struct sfd_handle *hn, struct sfadxt *adxt);
static ADXT sfadxt_CreateEx(struct sfadxt_para *para);
static void sfadxt_SetTimeFn(struct sfd_handle *hn);
static Sint32 sfadxt_GetTime(struct sfd_handle *hn, Sint32 *time,
                             Sint32 *tunit);
static Sint32 sfadxt_DestroySub(ADXT player);

/* the parameters the next handle's decoder takes */
static struct sfadxt_para sfadxt_para;

void SFD_SetAdxtPara(struct sfadxt_para *para)
{
    /* @hack spelling: every store through (&sfadxt_para)->, which has SHC load
     * the record's address twice and spread it over R0 to R3 (0x8ced6d5c,
     * 0x8ced6d70). without it, SHC loads the address once, the object runs 12
     * bytes short, and 24 of 24 words differ */
    (&sfadxt_para)->rbufsize = para->rbufsize;
    (&sfadxt_para)->flapsize = para->flapsize;
    (&sfadxt_para)->rbuf = (char *)(((Sint32)para->rbuf + 31) & ~31);
    (&sfadxt_para)->nch = para->nch;
    (&sfadxt_para)->sfreq = para->sfreq;
    (&sfadxt_para)->worksize = para->worksize;
    (&sfadxt_para)->work = (char *)(((Sint32)para->work + 31) & ~31);
}

/* clear the parameters SFD_SetAdxtPara set. the stage's functions are
 * sfd_trfns, and none of them reads its three arguments */
Sint32 SFADXT_Init(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    UTY_MemsetDword(&sfadxt_para, 0, sizeof(sfadxt_para) / sizeof(Uint32));
    return 0;
}

Sint32 SFADXT_Finish(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    return 0;
}

/* one server step: copy what the input offers into the joint, then update
 * the output's ready and ended flags. once the output has ended, a step does
 * nothing */
Sint32 SFADXT_ExecServer(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                         Sint32 arg3)
{
    Sint32 err;

    if (sfadxt_GetTermDst(hn) == TRUE) {
        return 0;
    }
    err = sfadxt_SupplyData(hn);
    sfadxt_ChkPrepFlg(hn);
    sfadxt_ChkTermFlg(hn);
    return err;
}

static Sint32 sfadxt_SupplyData(struct sfd_handle *hn)
{
    char *data;
    Sint32 len;
    Sint32 copied;
    Sint32 err;

    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced6de4) and copies to R4 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 3 of 27 words
     * differ */
    if ((err = sfadxt_RingGetRead(hn, &data, &len)) != 0) {
        return err;
    }
    sfadxt_CopyData(hn, data, len, &copied);
    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced6dfc). assigned first, the test reads its copy in R4,
     * and 1 of 27 words differ */
    if ((err = sfadxt_RingAddRead(hn, copied)) != 0) {
        return err;
    }
    return err;
}

/* copy the len bytes at data into a free chunk of the joint, as far as the
 * chunk and SFADXT_COPY_MAX allow. the copied part goes on the joint's data
 * line, and *copied gets its length */
static void sfadxt_CopyData(struct sfd_handle *hn, char *data, Sint32 len,
                            Sint32 *copied)
{
    struct sfd_trn *trn;
    struct sfadxt *adxt;
    SJ sj;
    SJCK ck;
    SJCK rest;
    Sint32 cklen;
    char *dst;
    Sint32 n;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    adxt = &trn->u.adxt;
    sj = adxt->sj;
    sj->vtbl->GetChunk(sj, SJ_LIN_FREE, SFADXT_CHUNK_MAX, &ck);
    cklen = ck.len;
    dst = ck.data;
    if (len < cklen) {
        cklen = len;
    }
    n = (cklen < SFADXT_COPY_MAX) ? cklen : SFADXT_COPY_MAX;
    UTY_MemcpyByte((Uint8 *)dst, (Uint8 *)data, n);
    if (n == 0) {
        sj->vtbl->UngetChunk(sj, SJ_LIN_FREE, &ck);
    } else {
        SJ_SplitChunk(&ck, n, &ck, &rest);
        sj->vtbl->PutChunk(sj, SJ_LIN_DATA, &ck);
        sj->vtbl->UngetChunk(sj, SJ_LIN_FREE, &rest);
    }
    *copied = n;
}

/* the output is ready once the input is */
static void sfadxt_ChkPrepFlg(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Sint32 ibuf;
    Sint32 obuf;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    ibuf = trn->ibuf;
    obuf = trn->obuf[0];
    if (SFBUF_GetPrepFlg(hn, ibuf) == TRUE) {
        if (sfadxt_IsPrepEnd(hn)) {
            SFBUF_SetPrepFlg(hn, obuf, TRUE);
        }
    }
}

static Bool sfadxt_IsPrepEnd(struct sfd_handle *hn)
{
    return TRUE;
}

/* the output has ended once the player has played to the end */
static void sfadxt_ChkTermFlg(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    struct sfadxt *adxt;
    ADXT player;
    Sint32 stat;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    adxt = &trn->u.adxt;
    player = adxt->adxt;
    stat = ADXT_GetStat(player);
    if (stat == ADXT_STAT_PLAYEND) {
        sfadxt_SetTermDst(hn, TRUE);
    }
}

/* the run of input to read, in *data and *len. it runs to the ring's end, or,
 * once sfadxt_CopyFlap has set an end, to the end of the copy past it. the
 * stage keeps the copy's length in the ring's dlm_len, and that end in its
 * dlm_end */
static Sint32 sfadxt_RingGetRead(struct sfd_handle *hn, char **data,
                                 Sint32 *len)
{
    struct sfd_trn *trn;
    Sint32 ibuf;
    struct sfbuf_ring_inf inf;
    char *dlm;
    char *area;
    Sint32 arealen;
    Sint32 flapped;
    char *end;
    Sint32 flapsize;
    Sint32 err;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    ibuf = trn->ibuf;
    err = SFBUF_RingGetRead(hn, ibuf, &inf);
    if (err != 0) {
        return err;
    }
    SFBUF_RingGetDlm(hn, ibuf, &dlm, &flapped, &end, &flapsize);
    err = sfadxt_CopyFlap(hn, &inf, flapsize, &flapped, &end);
    if (err != 0) {
        return SFLIB_SetErr(hn, err);
    }
    /* @hack spelling: the reads through (&inf)->, which form inf's address
     * afresh for each (ADD #24,R4 at 0x8ced6fc4, ADD #24,R5 at 0x8ced6fcc).
     * without it, SHC reads inf on the stack directly, the object runs 12 bytes
     * short, and 44 of 91 words differ */
    area = (&inf)->area;
    arealen = (&inf)->len;
    if (end != NULL) {
        *len = end - area;
    } else {
        *len = arealen;
    }
    *data = area;
    SFBUF_RingSetDlm(hn, ibuf, dlm, flapped, end);
    return 0;
}

/* copy the data that wraps to the ring's top into the margin after the ring's
 * end. the copy goes on from the *flapped bytes copied before, up to flapsize
 * in all. nothing is copied once *end is set, or while no data wraps. once
 * flapsize bytes are copied, or the input has ended, *end is set to the end of
 * the copy */
static Sint32 sfadxt_CopyFlap(struct sfd_handle *hn, struct sfbuf_ring_inf *inf,
                              Sint32 flapsize, Sint32 *flapped, char **end)
{
    char *area;
    Sint32 arealen;
    char *wrap;
    Sint32 wraplen;
    Sint32 n;
    char *src;
    char *dst;

    if (*end != NULL) {
        return 0;
    }
    area = inf->area;
    arealen = inf->len;
    wrap = inf->wrap;
    wraplen = inf->wraplen;
    if (wraplen == 0) {
        return 0;
    }
    n = (wraplen - *flapped < flapsize - *flapped) ? wraplen - *flapped
                                                   : flapsize - *flapped;
    src = wrap + *flapped;
    dst = area + (arealen + *flapped);
    UTY_MemcpyByte((Uint8 *)dst, (Uint8 *)src, n);
    *flapped += n;
    if (*flapped == flapsize || sfadxt_GetTermSrc(hn) == TRUE) {
        *end = dst + n;
    }
    return 0;
}

static Sint32 sfadxt_RingAddRead(struct sfd_handle *hn, Sint32 len)
{
    struct sfd_trn *trn;
    Sint32 ibuf;
    Sint32 err;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    ibuf = trn->ibuf;
    /* @hack spelling: the result assigned in the test, which SHC tests in R0
     * (TST R0,R0 at 0x8ced709a) and copies to R5 in the branch's delay slot.
     * assigned first, it is copied to R4 to be tested, and 4 of 26 words
     * differ */
    if ((err = SFBUF_RingAddRead(hn, ibuf, len)) != 0) {
        return err;
    }
    return 0;
}

static Bool sfadxt_GetTermSrc(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Sint32 ibuf;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    ibuf = trn->ibuf;
    return SFBUF_GetTermFlg(hn, ibuf);
}

static Bool sfadxt_GetTermDst(struct sfd_handle *hn)
{
    struct sfd_trn *trn;
    Sint32 obuf;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    obuf = trn->obuf[0];
    return SFBUF_GetTermFlg(hn, obuf);
}

static void sfadxt_SetTermDst(struct sfd_handle *hn, Bool flag)
{
    struct sfd_trn *trn;
    Sint32 obuf;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    obuf = trn->obuf[0];
    SFBUF_SetTermFlg(hn, obuf, flag);
}

/* create the handle's player, and the joint it reads, over the ring and work
 * SFD_SetAdxtPara set. a joint that can't be created leaves the player
 * undestroyed. on success, the handle's time comes from the player */
Sint32 SFADXT_Create(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                     Sint32 arg3)
{
    struct sfd_trn *trn;
    struct sfadxt *adxt;
    struct sfadxt_para *para;
    ADXT player;
    SJ sj;
    Sint32 err;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    adxt = &trn->u.adxt;
    para = &adxt->para;
    err = sfadxt_InitInf(hn, adxt);
    if (err != 0) {
        return err;
    }
    player = sfadxt_CreateEx(para);
    if (player == NULL) {
        return SFLIB_SetErr(hn, SFADXT_ERR_CREATE);
    }
    sj = SJRBF_Create(para->rbuf, para->rbufsize, para->flapsize);
    if (sj == NULL) {
        return SFLIB_SetErr(hn, SFADXT_ERR_SJ);
    }
    adxt->adxt = player;
    adxt->sj = sj;
    sfadxt_SetTimeFn(hn);
    return 0;
}

static Sint32 sfadxt_InitInf(struct sfd_handle *hn, struct sfadxt *adxt)
{
    if ((&sfadxt_para)->rbuf == NULL || (&sfadxt_para)->work == NULL) {
        return SFLIB_SetErr(hn, SFADXT_ERR_PARA);
    }
    adxt->para = sfadxt_para;
    adxt->adxt = NULL;
    adxt->sj = NULL;
    adxt->time = 0;
    adxt->tunit = 1;
    return 0;
}

static ADXT sfadxt_CreateEx(struct sfadxt_para *para)
{
    ADXT player;

    player = ADXT_Create(para->nch, para->work, para->worksize);
    if (player == NULL) {
        return NULL;
    }
    ADXT_SetSvrFreq(player, SFADXT_SVRFREQ);
    return player;
}

/* make the player the handle's time source */
static void sfadxt_SetTimeFn(struct sfd_handle *hn)
{
    SFSET_SetCond(hn, SFD_COND_TIME_SRC, SFTIM_SRC_AUDIO);
    SFTIM_SetTimeFn(hn, sfadxt_GetTime, SFTIM_SRC_AUDIO);
}

/* the handle's time from the player. it is SFTIM_NOT_YET before the handle's
 * time starts, and the player's time while the handle plays. otherwise it is
 * the time the player gave last */
static Sint32 sfadxt_GetTime(struct sfd_handle *hn, Sint32 *time, Sint32 *tunit)
{
    struct sfd_trn *trn;
    struct sfadxt *adxt;
    ADXT player;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    adxt = &trn->u.adxt;
    player = adxt->adxt;
    /* @hack spelling: an empty arm for the failed check, which has adxt's time
     * pass through R3 and its unit through R2 (0x8ced7280, 0x8ced7286). as one
     * test of the check, the two trade registers, and 4 of 49 words differ */
    if (!SFTIM_ChkRegularTime(hn, time, tunit)) {
    } else {
        *time = adxt->time;
        *tunit = adxt->tunit;
        if (hn->stat == SFD_STAT_PLAY) {
            ADXT_GetTime(player, time, tunit);
            adxt->time = *time;
            adxt->tunit = *tunit;
        }
    }
    return 0;
}

Sint32 SFADXT_Destroy(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    struct sfd_trn *trn;
    struct sfadxt *adxt;
    ADXT player;
    SJ sj;
    Sint32 err;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    adxt = &trn->u.adxt;
    player = adxt->adxt;
    sj = adxt->sj;
    sj->vtbl->Destroy(sj);
    /* @hack spelling: sfadxt_DestroySub's result through err, which keeps the
     * call a BSR (0x8ced72d0). returned directly, it becomes a BRA after the
     * epilogue, and 8 of 21 words differ */
    err = sfadxt_DestroySub(player);
    return err;
}

static Sint32 sfadxt_DestroySub(ADXT player)
{
    ADXT_Destroy(player);
    return 0;
}

Sint32 SFADXT_Standby(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return 0;
}

/* start the player on the joint */
Sint32 SFADXT_Start(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    struct sfd_trn *trn;
    struct sfadxt *adxt;
    ADXT player;
    SJ sj;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    adxt = &trn->u.adxt;
    player = adxt->adxt;
    sj = adxt->sj;
    ADXT_StartSj(player, sj);
    return 0;
}

Sint32 SFADXT_Stop(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2, Sint32 arg3)
{
    struct sfd_trn *trn;
    struct sfadxt *adxt;
    ADXT player;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    adxt = &trn->u.adxt;
    player = adxt->adxt;
    ADXT_Stop(player);
    return 0;
}

/* the stage leaves the player playing through a pause */
Sint32 SFADXT_Pause(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                    Sint32 arg3)
{
    struct sfd_trn *trn;
    struct sfadxt *adxt;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    ADXT player;

    trn = &hn->trntbl.trn[SFD_TR_AUDIO];
    adxt = &trn->u.adxt;
    /* @hack redundant: player, set and never read. SHC drops the read, but
     * keeps the address it reads through, adxt (ADD #16,R4 at 0x8ced732a).
     * without it, that ADD goes too, and 5 of 7 words differ */
    player = adxt->adxt;
    return 0;
}

/* SFADXT_GetWrite, SFADXT_AddWrite, SFADXT_GetRead and SFADXT_AddRead each
 * record SFADXT_ERR_NORING as the handle's error, and return it. the stage has
 * no ring of its own: the player reads the joint directly */
Sint32 SFADXT_GetWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                       Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFADXT_ERR_NORING);
}

Sint32 SFADXT_AddWrite(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                       Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFADXT_ERR_NORING);
}

Sint32 SFADXT_GetRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFADXT_ERR_NORING);
}

Sint32 SFADXT_AddRead(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                      Sint32 arg3)
{
    return SFLIB_SetErr(hn, SFADXT_ERR_NORING);
}

const struct sfd_trif SFD_tr_ad_adxt = {{
    SFADXT_Init,
    SFADXT_Finish,
    SFADXT_ExecServer,
    SFADXT_Create,
    SFADXT_Destroy,
    SFADXT_Standby,
    SFADXT_Start,
    SFADXT_Stop,
    SFADXT_Pause,
    SFADXT_GetWrite,
    SFADXT_AddWrite,
    SFADXT_GetRead,
    SFADXT_AddRead,
}};
