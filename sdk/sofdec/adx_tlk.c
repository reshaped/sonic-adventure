/* ADX talk players: their creation, start and stop, their settings, and the
 * server that runs them */
#include <mw_rna.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>
#include <sofdec.h>
#include <string.h>

/* the bytes past the input ring's end that mirror its start */
#define ADXT_IBUF_XLEN 36
/* each output ring's size, and the distance from one channel's ring to the
 * next, in samples. the ring's extra samples fill the gap between them */
#define ADXT_OBUF_SIZE 0x2000
#define ADXT_OBUF_DIST 0x2020
/* the bytes nch channels' output rings take */
#define ADXT_CALC_OBUFSIZE(nch) (ADXT_OBUF_DIST * (nch) * sizeof(short))
/* the sector a disc stream's reads stop at until its header is read */
#define ADXT_PREP_RDSCT 25
/* pmode for a stream from the disc */
#define ADXT_PMODE_FNAME 0
/* svrfreq until ADXT_SetSvrFreq sets it: the server's calls a second */
#define ADXT_DEF_SVRFREQ 60
#define ADXT_DEF_OUTVOL 0
#define ADXT_SCT_SIZE 2048
/* pmode for a stream from the caller's joint */
#define ADXT_PMODE_SJ 3

#define TEXT_1_LENGTH 42
#define TEXT_2_LENGTH 41
#include "sofdec/adx_tlk/text.inc"

/* the SFA library's version and build */
const char *sfa_build = TEXT_2;

/* the syTmr counts that the last ADXT_ExecServer took. adxt_svrcnt times the
 * whole pass, and adxt_svrcnt_hndl its loop over the handles. each of the
 * others times the server it names */
static long adxt_svrcnt = 0;
static long adxt_svrcnt_sjd = 0;
static long adxt_svrcnt_rna = 0;
static long adxt_svrcnt_adxf = 0;
static long adxt_svrcnt_adxstm = 0;
static long adxt_svrcnt_hndl = 0;

/* returns NULL when all ADXT_MAX_OBJ are in use, or when a joint, the stream
 * decoder or the sound output can't be created */
ADXT ADXT_Create(long maxnch, void *work, long worksize)
{
    ADXT adxt;
    char *buf;
    long size;
    long i;

    buf = (char *)(((unsigned long)work + 31) & ~31);
    size = worksize - (buf - (char *)work);
    for (i = 0; i < ADXT_MAX_OBJ; i++) {
        if (adxt_obj[i].used == 0) {
            break;
        }
    }
    if (i == ADXT_MAX_OBJ) {
        return NULL;
    }
    adxt = &adxt_obj[i];
    memset(adxt, 0, sizeof(ADX_TALK));
    adxt->maxnch = maxnch;
    adxt->ibuf = (Sint8 *)(buf + ADXT_CALC_OBUFSIZE(maxnch));
    adxt->ibuflen = (size - ADXT_CALC_OBUFSIZE(maxnch) - ADXT_IBUF_XLEN) /
                    ADXT_SCT_SIZE * ADXT_SCT_SIZE;
    adxt->ibufxlen = ADXT_IBUF_XLEN;
    adxt->obuf = (short *)buf;
    adxt->obufsize = ADXT_OBUF_SIZE;
    adxt->obufdist = ADXT_OBUF_DIST;
    adxt->sji = adxt->sjf =
        SJRBF_Create((char *)adxt->ibuf, adxt->ibuflen, adxt->ibufxlen);
    if (adxt->sji == NULL) {
        /* @bug memory: a joint or the stream decoder that can't be created
         * leaves sjd NULL, here and in the two failures below. ADXT_Destroy's
         * ADXT_Stop then calls ADXSJD_Stop on it, which passes ADXB_Stop the
         * word at address 4 (MOV.L @(4,R4),R4 at 0x8c66dabc) and stores its
         * stat at address 1 (MOV.B R0,@(1,R2) at 0x8c66dac2). ADXT_Stop also
         * resets each NULL output joint through an interface read from
         * address 0 (MOV.L @R11,R2 at 0x8c65b9dc) */
        ADXT_Destroy(adxt);
        return NULL;
    }
    for (i = 0; i < maxnch; i++) {
        adxt->sjo[i] =
            SJRBF_Create((char *)(adxt->obuf + (adxt->obufdist * i)),
                         adxt->obufsize * sizeof(short),
                         (adxt->obufdist - adxt->obufsize) * sizeof(short));
        if (adxt->sjo[i] == NULL) {
            ADXT_Destroy(adxt);
            return NULL;
        }
    }
    /* @hack spelling: sjd set in the test, which tests ADXSJD_Create's R0
     * itself (TST R0,R0 at 0x8c65b730). set before the test, SHC tests a copy
     * in R3, the function is 2 bytes longer and 26 of 208 words differ */
    if ((adxt->sjd = ADXSJD_Create(adxt->sji, maxnch, adxt->sjo)) == NULL) {
        ADXT_Destroy(adxt);
        return NULL;
    }
    /* @hack spelling: rna set in the test, which tests ADXRNA_Create's R0
     * itself (TST R0,R0 at 0x8c65b740). set before the test, SHC tests a copy
     * in R2, the function is 2 bytes longer and 13 of 208 words differ */
    if ((adxt->rna = ADXRNA_Create(adxt->sjo, maxnch)) == NULL) {
        ADXT_Destroy(adxt);
        return NULL;
    }
    adxt->svrfreq = ADXT_DEF_SVRFREQ;
    adxt->maxsct = adxt->ibuflen / ADXT_SCT_SIZE;
    adxt->minsct = adxt->maxsct * 0.85;
    adxt->outvol = ADXT_DEF_OUTVOL;
    for (i = 0; i < maxnch; i++) {
        adxt->outpan[i] = ADXT_PAN_AUTO;
    }
    adxt->lpflg = 1;
    adxt->trp = 0;
    adxt->wpos = 0;
    adxt->mofst = 0;
    adxt->ercode = 0;
    adxt->edecpos = 0;
    adxt->edeccnt = 0;
    adxt->eshrtcnt = 0;
    adxt->autorcvr = ADXT_RMODE_STOP;
    adxt->used = 1;
    return adxt;
}

void ADXT_Destroy(ADXT adxt)
{
    long i;

    if (adxt == NULL) {
        return;
    }
    ADXT_Stop(adxt);
    if (adxt->rna != NULL) {
        ADXRNA_Destroy(adxt->rna);
    }
    if (adxt->sjd != NULL) {
        ADXSJD_Destroy(adxt->sjd);
    }
    if (adxt->stm != NULL) {
        ADXSTM_Close(adxt->stm);
    }
    ADXCRS_Lock();
    if (adxt->sjf != NULL) {
        SJ_Destroy(adxt->sjf);
    }
    for (i = 0; i < adxt->maxnch; i++) {
        if (adxt->sjo[i] != NULL) {
            SJ_Destroy(adxt->sjo[i]);
        }
    }
    memset(adxt, 0, sizeof(ADX_TALK));
    adxt->used = 0;
    ADXCRS_Unlock();
}

void adxt_start_sj(ADXT adxt, SJ sj)
{
    ADXSJD_SetInSj(adxt->sjd, sj);
    adxt->sji = sj;
    ADXSJD_Start(adxt->sjd);
    adxt->stat = ADXT_STAT_DECINFO;
    adxt->lpcnt = 0;
    adxt->pstready_flag = 0;
}

void adxt_start_stm(ADXT adxt)
{
    ADXSTM_SetBufSize(adxt->stm, adxt->minsct * ADXT_SCT_SIZE,
                      adxt->maxsct * ADXT_SCT_SIZE);
    ADXSTM_SetEos(adxt->stm, ADXT_PREP_RDSCT);
    ADXSTM_Start(adxt->stm);
    adxt_start_sj(adxt, adxt->sjf);
    adxt->pmode = ADXT_PMODE_FNAME;
}

void ADXT_StartSj(ADXT adxt, SJ sj)
{
    ADXT_Stop(adxt);
    ADXCRS_Lock();
    ADXSJD_SetInSj(adxt->sjd, sj);
    adxt->sji = sj;
    ADXSJD_Start(adxt->sjd);
    adxt->stat = ADXT_STAT_DECINFO;
    adxt->pmode = ADXT_PMODE_SJ;
    adxt->lpcnt = 0;
    ADXCRS_Unlock();
}

void ADXT_Stop(ADXT adxt)
{
    long i;

    ADXCRS_Lock();
    ADXRNA_SetTransSw(adxt->rna, 0);
    ADXRNA_SetPlaySw(adxt->rna, 0);
    ADXSJD_Stop(adxt->sjd);
    if (adxt->stm != NULL) {
        ADXSTM_Stop(adxt->stm);
        ADXSTM_Close(adxt->stm);
        SJ_Reset(adxt->sji);
        adxt->stm = NULL;
    }
    if (adxt->pmode == ADXT_PMODE_MEM && adxt->sji != NULL) {
        SJ_Destroy(adxt->sji);
    }
    adxt->sji = NULL;
    for (i = 0; i < adxt->maxnch; i++) {
        SJ_Reset(adxt->sjo[i]);
    }
    adxt->stat = ADXT_STAT_STOP;
    ADXCRS_Unlock();
}

long ADXT_GetStat(ADXT adxt)
{
    return adxt->stat;
}

void ADXT_GetTime(ADXT adxt, long *ncount, long *tscale)
{
    if (adxt->stat == ADXT_STAT_PLAYING || adxt->stat == ADXT_STAT_DECEND) {
        ADXRNA_GetTime(adxt->rna, ncount, tscale);
    } else if (adxt->stat == ADXT_STAT_PLAYEND) {
        *ncount = ADXSJD_GetTotalNumSmpl(adxt->sjd);
        *tscale = ADXSJD_GetSfreq(adxt->sjd);
        *ncount *= 16 / ADXSJD_GetOutBps(adxt->sjd);
    } else {
        *ncount = 0;
        *tscale = 1;
    }
}

long ADXT_GetNumSmpl(ADXT adxt)
{
    if (adxt->stat >= ADXT_STAT_PREP) {
        return ADXSJD_GetTotalNumSmpl(adxt->sjd);
    }
    return 0;
}

long ADXT_GetSfreq(ADXT adxt)
{
    if (adxt->stat >= ADXT_STAT_PREP) {
        return ADXSJD_GetSfreq(adxt->sjd);
    }
    return 0;
}

void ADXT_SetOutPan(ADXT adxt, long ch, long pan)
{
    adxt->outpan[ch] = pan;
    if (ch < adxt->maxnch) {
        ADXRNA_SetOutPan(adxt->rna, ch, pan);
    } else {
        ADXERR_CallErrFunc(TEXT_1);
    }
}

long ADXT_GetOutPan(ADXT adxt, long ch)
{
    return adxt->outpan[ch];
}

void ADXT_SetOutVol(ADXT adxt, long vol)
{
    adxt->outvol = vol;
    ADXRNA_SetOutVol(adxt->rna, vol);
}

long ADXT_GetOutVol(ADXT adxt)
{
    return adxt->outvol;
}

void ADXT_SetSvrFreq(ADXT adxt, long freq)
{
    adxt->svrfreq = freq;
}

void ADXT_SetReloadSct(ADXT adxt, long minsct)
{
    adxt->minsct = minsct;
}

/* NOLINTBEGIN(clang-diagnostic-return-type) */
/* the whole sectors of data the input joint holds */
long ADXT_GetNumSctIbuf(ADXT adxt)
{
    if (adxt->sji != NULL) {
        return SJ_GetNumData(adxt->sji, SJ_LIN_DATA) / ADXT_SCT_SIZE;
    }
    /* @bug uninitialized: with no input joint, nothing sets the count. BT at
     * 0x8c65bb74 goes to the epilogue, and RTS at 0x8c65bb94 returns R0 as the
     * caller left it */
}

/* NOLINTEND(clang-diagnostic-return-type) */

long ADXT_IsIbufSafety(ADXT adxt)
{
    long n;

    if (adxt->sji != NULL) {
        n = SJ_GetNumData(adxt->sji, SJ_LIN_DATA);
        if (n >= adxt->minsct * ADXT_SCT_SIZE) {
            return 1;
        }
    }
    return 0;
}

void ADXT_SetAutoRcvr(ADXT adxt, long rmode)
{
    adxt->autorcvr = rmode;
}

long ADXT_IsCompleted(ADXT adxt)
{
    if (adxt->stat == ADXT_STAT_PLAYEND) {
        return 1;
    }
    return 0;
}

void ADXT_ExecServer(void)
{
    /* @hack redundant: register on t0, t1, t3, t4, t5 and adxt, which keeps
     * the counts in R11 to R14 and calls syTmrGetCount through R8 (0x8c65bbf8
     * to 0x8c65bc0a). without it on all six, the addresses take R12 and R13
     * and 83 of 97 words differ. without it on any one of them, 11 to 85
     * differ */
    register long t0;
    register long t1;
    long t2;
    register long t3;
    register long t4;
    register long t5;
    register ADXT adxt;
    long i;

    t0 = syTmrGetCount();
    ADXSJD_ExecServer();
    t1 = syTmrGetCount();
    adxt_svrcnt_sjd = syTmrDiffCount(t0, t1);
    ADXRNA_ExecServer();
    t2 = syTmrGetCount();
    adxt_svrcnt_rna = syTmrDiffCount(t1, t2);
    ADXF_ExecServer();
    t3 = syTmrGetCount();
    adxt_svrcnt_adxf = syTmrDiffCount(t2, t3);
    ADXSTM_ExecServer();
    t4 = syTmrGetCount();
    adxt_svrcnt_adxstm = syTmrDiffCount(t3, t4);
    /* @hack redundant: a read of ADXT_ExecHndl's address that changes nothing,
     * which puts that address in R9 and adxt_obj's in R10 (ADD R10,R4 at
     * 0x8c65bc68, JSR @R9 at 0x8c65bc70). without it, the two trade and 2 of
     * 97 words differ */
    (void)ADXT_ExecHndl;
    for (i = 0; i < ADXT_MAX_OBJ; i++) {
        adxt = &adxt_obj[i];
        if (adxt->used == 1) {
            ADXT_ExecHndl(adxt);
        }
    }
    t5 = syTmrGetCount();
    adxt_svrcnt_hndl = syTmrDiffCount(t4, t5);
    adxt_svrcnt = syTmrDiffCount(t0, t5);
}

long ADXT_GetErrCode(ADXT adxt)
{
    return adxt->ercode;
}

void ADXT_ClearErrCode(ADXT adxt)
{
    adxt->ercode = 0;
}

long ADXT_GetLpCnt(ADXT adxt)
{
    return adxt->lpcnt;
}

void ADXT_SetLpFlg(ADXT adxt, long flg)
{
    adxt->lpflg = flg;
}

SJ ADXT_GetInputSj(ADXT adxt)
{
    return adxt->sji;
}

void ADXT_SetWaitPlayStart(ADXT adxt, long flg)
{
    adxt->pstwait_flag = flg;
}

long ADXT_IsReadyPlayStart(ADXT adxt)
{
    return adxt->pstready_flag;
}

void ADXT_SetFx(ADXT adxt, long fxch, long fxlvl)
{
    mwRnaSetFx(adxt->rna, fxch, fxlvl);
}

void ADXT_GetFx(ADXT adxt, long *fxch, long *fxlvl)
{
    mwRnaGetFx(adxt->rna, fxch, fxlvl);
}

void ADXT_SetFilter(ADXT adxt, long coff, long q)
{
    mwRnaSetFilter(adxt->rna, coff, q);
}

void ADXT_GetFilter(ADXT adxt, long *coff, long *q)
{
    mwRnaGetFilter(adxt->rna, coff, q);
}

void ADXT_SetTranspose(ADXT adxt, long transps, long detune)
{
    mwRnaSetPitch(adxt->rna, transps, detune);
}

void ADXT_GetTranspose(ADXT adxt, long *transps, long *detune)
{
    mwRnaGetPitch(adxt->rna, transps, detune);
}

void ADXT_EntryErrFunc(void (*func)(void *obj, char *msg), void *obj)
{
    ADXERR_EntryErrFunc(func, obj);
}
