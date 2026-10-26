/* ADX stream decoders, which decode a stream from an input joint into a joint
 * for each channel */
#include <sg_xpt.h>
#include <sj.h>
#include <string.h>

#define ADXSJD_MAX_OBJ 8
#define ADXSJD_MAX_NCH 2
/* the most input adxsjd_decode_prep reads a header from, in bytes */
#define ADXSJD_MAX_HDLEN 0x1000
/* the bytes adxsjd_decode_prep waits for before it reads a header: the
 * shortest ADX header that holds the sample format */
#define ADXSJD_MIN_HDLEN 16
/* the bytes of an SPSD stream's header kept for the sound output */
#define ADXSJD_SPSD_LEN 64
/* the most bytes a step takes from each output joint's free line */
#define ADXSJD_MAX_OUTLEN 0x4000
/* the mark that ends an ADX stream's data, where a block's scale would be,
 * read big-endian */
#define ADXSJD_END_MARK 0x8001
/* the largest Sint32, a limit that never binds: maxdecsmpl until one is set,
 * and the input a step asks for */
#define ADXSJD_NO_LIMIT 0x7fffffff
/* the samples adxsjd_get_wr gives as left before the trap when none is set */
#define ADXSJD_NO_TRAP_LP 0x1fffffff

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ADX_SWAP16(x) ((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00))

typedef struct {
    Sint8 used;
    Sint8 stat;
    Sint8 maxnch;
    struct adxb_obj *adxb;
    SJ sji;
    SJ sjo[ADXSJD_MAX_NCH];
    SJCK ick; /* the input chunk the step in progress decodes */
    SJCK ock[ADXSJD_MAX_NCH]; /* each channel's free chunk the step fills */
    Sint32 decnsmpl; /* samples of each channel decoded since ADXSJD_Start */
    Sint32 decdtlen; /* input bytes decoded since ADXSJD_Start */
    Sint32 decpos; /* the stream's sample the next step decodes first */
    Sint32 maxdecsmpl; /* the most samples of each channel a step decodes */
    /* the trap. each step adds the samples it decoded to trapcnt and its input
     * bytes to trapdtlen. trapfunc, if set, runs with trapobj after a step
     * that leaves trapcnt at trapnsmpl or past it. a negative trapnsmpl sets
     * no trap */
    Sint32 trapnsmpl;
    Sint32 trapcnt;
    Sint32 trapdtlen;
    void (*trapfunc)(void *obj);
    void *trapobj;
    char spsd[ADXSJD_SPSD_LEN]; /* the start of an SPSD stream's header */
} ADXSJD_OBJ;
/* ADXSJD_DEFINED keeps sofdec.h from declaring ADXSJD as void *, so here a
 * handle is its record's address */
#define ADXSJD_DEFINED
typedef ADXSJD_OBJ *ADXSJD;
#include <sofdec.h>

ADXSJD_OBJ adxsjd_obj[ADXSJD_MAX_OBJ];

Sint16 *adxsjd_get_wr(void *obj, Sint32 *wpos, Sint32 *nroom, Sint32 *nleft);

void ADXSJD_Init(void)
{
    ADXB_Init();
    memset(adxsjd_obj, 0, sizeof(adxsjd_obj));
}

void ADXSJD_Finish(void)
{
    memset(adxsjd_obj, 0, sizeof(adxsjd_obj));
}

void adxsjd_clear(ADXSJD sjd)
{
    sjd->decnsmpl = 0;
    sjd->decdtlen = 0;
    sjd->decpos = 0;
    sjd->maxdecsmpl = ADXSJD_NO_LIMIT;
    sjd->trapnsmpl = -1;
    sjd->trapcnt = 0;
    sjd->trapdtlen = 0;
}

ADXSJD ADXSJD_Create(SJ sji, Sint32 maxnch, SJ *sjo)
{
    ADXSJD sjd;
    SJ sj;
    Sint32 i;
    Sint16 *buf;
    Sint32 bsize;
    /* @hack spelling: the second ring's distance through dist, a local, which
     * keeps sjo in R10, sj in R11 and bsize in R12 (MOV R6,R10 at 0x8c66d94e,
     * MOV R0,R12 at 0x8c66d9a2). passed straight to ADXB_Create, the three
     * rotate and 18 of 115 words differ */
    Sint32 dist;

    sj = sjo[0];
    for (i = 0; i < ADXSJD_MAX_OBJ; i++) {
        if (adxsjd_obj[i].used == 0) {
            break;
        }
    }
    if (i == ADXSJD_MAX_OBJ) {
        return NULL;
    }
    sjd = &adxsjd_obj[i];
    buf = (Sint16 *)SJRBF_GetBufPtr(sj);
    bsize = SJRBF_GetBufSize(sj) / sizeof(Sint16);
    dist = bsize + (SJRBF_GetXtrSize(sj) / sizeof(Sint16));
    sjd->adxb = ADXB_Create(maxnch, buf, bsize, dist);
    /* @bug memory: ADXB_EntryGetWrFunc runs before the NULL test (JSR at
     * 0x8c66d9c4, TST at 0x8c66d9ca). when ADXB_Create fails, it stores
     * adxsjd_get_wr and sjd at addresses 0x88 and 0x8c (0x8c6759ae,
     * 0x8c6759b4) */
    ADXB_EntryGetWrFunc(sjd->adxb, adxsjd_get_wr, sjd);
    if (sjd->adxb == NULL) {
        return NULL;
    }
    sjd->sji = sji;
    sjd->maxnch = maxnch;
    for (i = 0; i < maxnch; i++) {
        sjd->sjo[i] = sjo[i];
    }
    sjd->stat = ADXSJD_STAT_STOP;
    adxsjd_clear(sjd);
    sjd->trapfunc = NULL;
    sjd->trapobj = NULL;
    sjd->used = 1;
    return sjd;
}

void ADXSJD_Destroy(ADXSJD sjd)
{
    if (sjd != NULL) {
        if (sjd->adxb != NULL) {
            ADXB_Destroy(sjd->adxb);
        }
        ADXCRS_Lock();
        /* sizeof(ADXSJD) is a pointer's 4 bytes (MOV #4,R6 at 0x8c66da6e), so
         * this clears used, stat and maxnch alone. clearing used frees the
         * record */
        /* NOLINTNEXTLINE(clang-diagnostic-sizeof-pointer-memaccess) */
        memset(sjd, 0, sizeof(ADXSJD));
        ADXCRS_Unlock();
    }
}

Sint32 ADXSJD_GetStat(ADXSJD sjd)
{
    return sjd->stat;
}

void ADXSJD_SetInSj(ADXSJD sjd, SJ sji)
{
    sjd->sji = sji;
}

void ADXSJD_SetOutSj(ADXSJD sjd, Sint32 ch, SJ sjo)
{
    sjd->sjo[ch] = sjo;
}

void ADXSJD_SetMaxDecSmpl(ADXSJD sjd, Sint32 nsmpl)
{
    sjd->maxdecsmpl = nsmpl;
}

void ADXSJD_Start(ADXSJD sjd)
{
    adxsjd_clear(sjd);
    sjd->stat = ADXSJD_STAT_DECINFO;
}

void ADXSJD_Stop(ADXSJD sjd)
{
    ADXB_Stop(sjd->adxb);
    sjd->stat = ADXSJD_STAT_STOP;
}

/* read the stream's header, take it from the input, and start decoding. until
 * the input holds ADXSJD_MIN_HDLEN bytes and a header ADXB_DecodeHeader reads
 * whole, it puts the input back and tries again on the next pass */
void adxsjd_decode_prep(ADXSJD sjd)
{
    ADXB adxb;
    SJ sji;
    SJCK ck1;
    SJCK ck2;
    Sint32 hdlen;

    adxb = sjd->adxb;
    sji = sjd->sji;
    sji->vtbl->GetChunk(sji, SJ_LIN_DATA, ADXSJD_MAX_HDLEN, &ck1);
    if (ck1.len < ADXSJD_MIN_HDLEN) {
        sji->vtbl->UngetChunk(sji, SJ_LIN_DATA, &ck1);
        return;
    }
    hdlen = ADXB_DecodeHeader(adxb, (Sint8 *)ck1.data, ck1.len);
    if (hdlen == 0 || hdlen > ck1.len) {
        sji->vtbl->UngetChunk(sji, SJ_LIN_DATA, &ck1);
        return;
    }
    if (ADXB_GetFormat(adxb) == ADXB_FORMAT_SPSD) {
        memcpy(sjd->spsd, ck1.data, MIN(ck1.len, ADXSJD_SPSD_LEN));
    }
    SJ_SplitChunk(&ck1, hdlen, &ck1, &ck2);
    sji->vtbl->PutChunk(sji, SJ_LIN_FREE, &ck1);
    sji->vtbl->UngetChunk(sji, SJ_LIN_DATA, &ck2);
    sjd->stat = ADXSJD_STAT_DECODE;
}

/* the basic decoder's get_wr: take a free chunk of up to ADXSJD_MAX_OUTLEN
 * bytes from each output joint. *wpos gets the first chunk's offset in its
 * ring, in samples. *nroom gets its samples, at most maxdecsmpl. *nleft, the
 * samples left before decoding stops, gets those left before the trap, or
 * ADXSJD_NO_TRAP_LP with no trap. returns the basic decoder's ring */
Sint16 *adxsjd_get_wr(void *obj, Sint32 *wpos, Sint32 *nroom, Sint32 *nleft)
{
    ADXSJD sjd = obj;
    SJ sjo;
    char *buf;
    Sint32 i;
    Sint32 n;

    sjo = sjd->sjo[0];
    for (i = 0; i < ADXB_GetNumChan(sjd->adxb); i++) {
        sjd->sjo[i]->vtbl->GetChunk(sjd->sjo[i], SJ_LIN_FREE, ADXSJD_MAX_OUTLEN,
                                    &sjd->ock[i]);
    }
    buf = SJRBF_GetBufPtr(sjo);
    *wpos = (Uint32)(sjd->ock[0].data - buf) / sizeof(Sint16);
    n = sjd->ock[0].len / sizeof(Sint16);
    *nroom = MIN(n, sjd->maxdecsmpl);
    if (sjd->trapnsmpl >= 0) {
        *nleft = sjd->trapnsmpl - sjd->trapcnt;
    } else {
        *nleft = ADXSJD_NO_TRAP_LP;
    }
    return ADXB_GetPcmBuf(sjd->adxb);
}

/* start a step on all the input holds, once the first output joint has room
 * for a block. the decoder enters ADXSJD_STAT_DECEND instead when decpos has
 * reached the stream's total, or for ADX when the input starts with
 * ADXSJD_END_MARK */
void adxsjd_decexec_start(ADXSJD sjd)
{
    ADXB adxb;
    SJ sji;
    Sint32 nsmpl;
    Sint32 n;
    Uint16 mark;

    adxb = sjd->adxb;
    sji = sjd->sji;
    sji->vtbl->GetChunk(sji, SJ_LIN_DATA, ADXSJD_NO_LIMIT, &sjd->ick);
    if (ADXB_GetFormat(adxb) == ADXB_FORMAT_ADX) {
        if (sjd->ick.len >= 2) {
            mark = ADX_SWAP16(*(Sint16 *)sjd->ick.data);
            if (mark == ADXSJD_END_MARK) {
                sjd->stat = ADXSJD_STAT_DECEND;
                sji->vtbl->UngetChunk(sji, SJ_LIN_DATA, &sjd->ick);
                return;
            }
        }
    }
    if (sjd->decpos >= ADXSJD_GetTotalNumSmpl(sjd)) {
        sjd->stat = ADXSJD_STAT_DECEND;
        sji->vtbl->UngetChunk(sji, SJ_LIN_DATA, &sjd->ick);
        return;
    }
    nsmpl = ADXSJD_GetBlkSmpl(sjd);
    n = sjd->sjo[0]->vtbl->GetNumData(sjd->sjo[0], SJ_LIN_FREE) /
        sizeof(Sint16);
    if (n < nsmpl) {
        sji->vtbl->UngetChunk(sji, SJ_LIN_DATA, &sjd->ick);
        return;
    }
    ADXB_EntryData(adxb, (Sint8 *)sjd->ick.data, sjd->ick.len);
    ADXB_Start(adxb);
}

/* end a step. it frees the input bytes the step decoded, and puts each
 * channel's samples on its output joint's data line, but none past the
 * stream's total. then it counts them, runs the trap if the step reached it,
 * and resets the basic decoder */
void adxsjd_decexec_end(ADXSJD sjd)
{
    ADXB adxb;
    SJ sji;
    SJCK ck1;
    SJCK ck2;
    Sint32 total;
    Sint32 dtlen;
    Sint32 decn;
    Sint32 nsmpl;
    Sint32 i;

    adxb = sjd->adxb;
    sji = sjd->sji;
    total = ADXB_GetTotalNumSmpl(adxb);
    dtlen = ADXB_GetDecDtLen(adxb);
    decn = ADXB_GetDecNumSmpl(adxb);
    nsmpl = MIN(decn, total - sjd->decpos);
    SJ_SplitChunk(&sjd->ick, dtlen, &ck1, &ck2);
    sji->vtbl->PutChunk(sji, SJ_LIN_FREE, &ck1);
    sji->vtbl->UngetChunk(sji, SJ_LIN_DATA, &ck2);
    for (i = 0; i < ADXB_GetNumChan(sjd->adxb); i++) {
        SJ_SplitChunk(&sjd->ock[i], nsmpl * sizeof(Sint16), &ck1, &ck2);
        sjd->sjo[i]->vtbl->PutChunk(sjd->sjo[i], SJ_LIN_DATA, &ck1);
        sjd->sjo[i]->vtbl->UngetChunk(sjd->sjo[i], SJ_LIN_FREE, &ck2);
    }
    sjd->decnsmpl += nsmpl;
    sjd->decdtlen += dtlen;
    sjd->decpos += nsmpl;
    sjd->trapcnt += nsmpl;
    sjd->trapdtlen += dtlen;
    /* @hack spelling: the test of trapcnt nested inside the test of trapnsmpl,
     * which reads trapnsmpl again for the second test (MOV.L @(60,R14),R1 at
     * 0x8c66de1c). as one test with &&, SHC reads it once, the function is 2
     * bytes shorter and 34 of 150 words differ */
    if (sjd->trapnsmpl >= 0) {
        if (sjd->trapcnt >= sjd->trapnsmpl && sjd->trapfunc != NULL) {
            sjd->trapfunc(sjd->trapobj);
        }
    }
    ADXB_Reset(adxb);
}

void adxsjd_decode_exec(ADXSJD sjd)
{
    ADXB adxb;

    adxb = sjd->adxb;
    if (ADXB_GetStat(adxb) == ADXB_STAT_STOP) {
        adxsjd_decexec_start(sjd);
    }
    ADXB_ExecHndl(adxb);
    if (ADXB_GetStat(adxb) == ADXB_STAT_END) {
        adxsjd_decexec_end(sjd);
    }
}

void ADXSJD_ExecHndl(ADXSJD sjd)
{
    if (sjd->stat == ADXSJD_STAT_DECINFO) {
        adxsjd_decode_prep(sjd);
    } else if (sjd->stat == ADXSJD_STAT_DECODE) {
        adxsjd_decode_exec(sjd);
    }
}

void ADXSJD_ExecServer(void)
{
    Sint32 i;

    for (i = 0; i < ADXSJD_MAX_OBJ; i++) {
        if (adxsjd_obj[i].used == 1) {
            ADXSJD_ExecHndl(&adxsjd_obj[i]);
        }
    }
}

Sint32 ADXSJD_GetDecDtLen(ADXSJD sjd)
{
    return sjd->decdtlen;
}

Sint32 ADXSJD_GetDecNumSmpl(ADXSJD sjd)
{
    return sjd->decnsmpl;
}

void ADXSJD_SetDecPos(ADXSJD sjd, Sint32 pos)
{
    sjd->decpos = pos;
}

Sint32 ADXSJD_GetDecPos(ADXSJD sjd)
{
    return sjd->decpos;
}

void ADXSJD_EntryTrapFunc(ADXSJD sjd, void (*func)(void *obj), void *obj)
{
    sjd->trapfunc = func;
    sjd->trapobj = obj;
}

void ADXSJD_SetTrapNumSmpl(ADXSJD sjd, Sint32 nsmpl)
{
    sjd->trapnsmpl = nsmpl;
}

Sint32 ADXSJD_GetTrapNumSmpl(ADXSJD sjd)
{
    return sjd->trapnsmpl;
}

void ADXSJD_SetTrapCnt(ADXSJD sjd, Sint32 cnt)
{
    sjd->trapcnt = cnt;
}

Sint32 ADXSJD_GetTrapCnt(ADXSJD sjd)
{
    return sjd->trapcnt;
}

void ADXSJD_SetTrapDtLen(ADXSJD sjd, Sint32 len)
{
    sjd->trapdtlen = len;
}

Sint32 ADXSJD_GetTrapDtLen(ADXSJD sjd)
{
    return sjd->trapdtlen;
}

Sint32 ADXSJD_GetFormat(ADXSJD sjd)
{
    return ADXB_GetFormat(sjd->adxb);
}

Sint32 ADXSJD_GetSfreq(ADXSJD sjd)
{
    return ADXB_GetSfreq(sjd->adxb);
}

Sint32 ADXSJD_GetNumChan(ADXSJD sjd)
{
    return ADXB_GetNumChan(sjd->adxb);
}

Sint32 ADXSJD_GetOutBps(ADXSJD sjd)
{
    return ADXB_GetOutBps(sjd->adxb);
}

Sint32 ADXSJD_GetBlkSmpl(ADXSJD sjd)
{
    return ADXB_GetBlkSmpl(sjd->adxb);
}

Sint32 ADXSJD_GetBlkLen(ADXSJD sjd)
{
    return ADXB_GetBlkLen(sjd->adxb);
}

Sint32 ADXSJD_GetTotalNumSmpl(ADXSJD sjd)
{
    return ADXB_GetTotalNumSmpl(sjd->adxb);
}

Sint32 ADXSJD_GetCof(ADXSJD sjd)
{
    return ADXB_GetCof(sjd->adxb);
}

Sint32 ADXSJD_GetNumLoop(ADXSJD sjd)
{
    return ADXB_GetNumLoop(sjd->adxb);
}

Sint32 ADXSJD_GetLpInsNsmpl(ADXSJD sjd)
{
    return ADXB_GetLpInsNsmpl(sjd->adxb);
}

Sint32 ADXSJD_GetLpStartPos(ADXSJD sjd)
{
    return ADXB_GetLpStartPos(sjd->adxb);
}

Sint32 ADXSJD_GetLpStartOfst(ADXSJD sjd)
{
    return ADXB_GetLpStartOfst(sjd->adxb);
}

Sint32 ADXSJD_GetLpEndPos(ADXSJD sjd)
{
    return ADXB_GetLpEndPos(sjd->adxb);
}

Sint32 ADXSJD_GetLpEndOfst(ADXSJD sjd)
{
    return ADXB_GetLpEndOfst(sjd->adxb);
}

char *ADXSJD_GetSpsdInfo(ADXSJD sjd)
{
    return sjd->spsd;
}
