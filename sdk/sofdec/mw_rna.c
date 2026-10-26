/* the audio renderer (mw_rna.h). while the transfer switch is on, a server
 * pass with no copy running takes a chunk from each channel's joint. it starts
 * copying each chunk to its channel's ring in sound memory. a chunk stays
 * taken until a later pass finds its copy finished. that pass puts the bytes
 * copied on the joint's free line, and returns the rest to its data line.
 * while play is on and the transfer switch off, the passes copy silence after
 * the last data instead, up to a ring's length */
#include <machine.h>
#include <mw_rna.h>
#include <mw_snd.h>
#include <sg_mwply.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>
#include <string.h>

#define MWRNA_MAX_OBJ 8

#define MWRNA_MAX_NCH 2
/* the 32-bit words of each silence buffer */
#define MWRNA_ZERO_NWORD 0x200
/* the sound chip's own sample rate, and a new renderer's */
#define MWRNA_SND_SFREQ 44100
/* startsmpl until mwRnaSetStartSmpl sets it */
#define MWRNA_DEF_STARTSMPL 0x2000
/* the playndata that keeps MWRNA_SW_AUTO from turning play on: ndata never
 * reaches it */
#define MWRNA_NO_AUTOPLAY 0x7fffffff
/* the data, in bytes, in channel 0's joint below which a flush ends */
#define MWRNA_FLUSH_MIN 32
/* the most status reads mwRnaSetTransSw makes while it waits for a copy to
 * finish */
#define MWRNA_WAIT_MAX 100000

#define SJ_UNGETCHUNK(sj, id, ck) (*(sj)->vtbl->UngetChunk)(sj, id, ck)
#define SJ_PUTCHUNK(sj, id, ck) (*(sj)->vtbl->PutChunk)(sj, id, ck)

/* lengths, counts and ring positions are in samples of one channel */
typedef struct mwrna_obj {
    Uint8 used;
    Uint8 stat; /* a set of MWRNA_SW_ bits */
    Uint8 maxnch;
    Uint8 nch;
    Uint8 ovrflw;
    Uint8 copying; /* 1 while a copy of samples or silence runs */
    /* 1 from mwRnaFlush until channel 0's joint holds under MWRNA_FLUSH_MIN
     * bytes of data, when the server turns the transfer switch off */
    Uint8 flush;
    Uint8 pause;
    Sint32 bps; /* bits per sample */
    Sint32 sfreq;
    SJ sj[MWRNA_MAX_NCH];
    SJCK ck[MWRNA_MAX_NCH]; /* the chunks the copy of samples reads */
    Sint32 bufptr[MWRNA_MAX_NCH]; /* each ring's sound memory offset */
    Sint32 bufsize; /* each ring's length */
    Sint32 wrpos;
    Sint32 rdpos;
    Sint32 ndata; /* written samples not yet played */
    MwSnd *snd;
    Sint32 trnsnsmpl; /* the samples of the last copy of samples started */
    /* the ndata at which MWRNA_SW_AUTO turns play on: startsmpl from
     * mwRnaStart, or MWRNA_NO_AUTOPLAY once play is on */
    Sint32 playndata;
    Sint32 startsmpl;
    Sint32 nplay;
    /* the silence copied while the transfer switch is off */
    Sint32 flshpos; /* the ring position of the next copy of silence */
    Sint32 nflsh; /* the silence copied since the switch turned off */
    Sint32 flshnsmpl; /* the samples of the last copy of silence started */
    Sint32 outvol;
    Sint32 outpan[MWRNA_MAX_NCH];
    Sint32 fxch;
    Sint32 fxlev;
    Sint32 semi;
    Sint32 fine;
    Sint32 coff;
    Sint32 q;
    Sint32 ntotal; /* the samples copied since the transfer switch turned on */
    Sint32 swsave; /* mwRnaGetTransSw | mwRnaGetPlaySw at mwRnaPause */
} MWRNA_OBJ;

static MWRNA_OBJ mwg_rna_obj[MWRNA_MAX_OBJ];

static Sint32 mwg_rna_zero[MWRNA_ZERO_NWORD];
static Sint32 mwg_rna_zero_adpcm[MWRNA_ZERO_NWORD];

static Uint32 mwg_rna_tcnt[2];

#define TEXT_1_LENGTH 43
#define TEXT_2_LENGTH 44
#define TEXT_3_LENGTH 40
#define TEXT_4_LENGTH 45
#define TEXT_5_LENGTH 35
#define TEXT_6_LENGTH 36
#define TEXT_7_LENGTH 37
#define TEXT_8_LENGTH 28
#define TEXT_9_LENGTH 35
#define TEXT_10_LENGTH 28
#define TEXT_11_LENGTH 32
#define TEXT_12_LENGTH 28
#define TEXT_13_LENGTH 34
#define TEXT_14_LENGTH 32
#define TEXT_15_LENGTH 30
#define TEXT_16_LENGTH 30
#define TEXT_17_LENGTH 29
#define TEXT_18_LENGTH 32
#include "sofdec/mw_rna/text.inc"

static const char mwg_rna_copyright[32] = {
#include "sofdec/mw_rna/mwg_rna_copyright.inc"
};
static const char mwg_rna_version[32] = {
#include "sofdec/mw_rna/mwg_rna_version.inc"
};
static char *const mwg_rna_build = TEXT_1;

void mwlRnaClearBuf(MWRNA_OBJ *rna);
Sint32 mwlRnaIsCopyEnd(MWRNA_OBJ *rna);
void mwlRnaAddWrPos(MWRNA_OBJ *rna);
void mwlRnaAddFlshPos(MWRNA_OBJ *rna);
void mwlRnaStartTrans(MWRNA_OBJ *rna);
void mwlRnaInsrtSlntDat(MWRNA_OBJ *rna);
void mwlRnaExecSvrHndl(MWRNA_OBJ *rna);

void mwlRnaLockCs(Sint32 *imask)
{
    *imask = _builtin_get_imask();
    _builtin_set_imask(IMASK_ALL);
}

void mwlRnaUnlockCs(const Sint32 *imask)
{
    _builtin_set_imask(*imask);
}

void mwlRnaSetSvrInt(void)
{
}

void mwRnaInit(void)
{
    Sint32 i;
    Sint32 *p;

    memset(mwg_rna_obj, 0, sizeof(mwg_rna_obj));
    memset(mwg_rna_zero, 0, sizeof(mwg_rna_zero));
    p = mwg_rna_zero_adpcm;
    /* @hack redundant: a store of the fill word through p that never runs.
     * SHC then keeps p in R6 and the fill word in R5 (0x8c66ed20, 0x8c66ed24).
     * without it, the fill loop's registers change and 6 of 30 words differ */
    if (0) {
        *p = 0x80808080;
    }
    /* @hack spelling: i stepped as i = i + 1, which SHC counts up with ADD #1
     * and CMP/GE (0x8c66ed2a, 0x8c66ed2c). as i++, it counts down with DT,
     * the function is 4 bytes shorter and 23 of 30 words differ */
    for (i = 0; i < MWRNA_ZERO_NWORD; i = i + 1) {
        *p++ = 0x80808080;
    }
    mwSndInit();
    mwlRnaSetSvrInt();
    mwRnaErrEntryFunc(0, 0);
}

void mwRnaFinish(void)
{
    Sint32 i;
    Sint32 *p;

    for (i = 0; i < MWRNA_MAX_OBJ; i++) {
        if (mwg_rna_obj[i].used == 1) {
            mwRnaStop(&mwg_rna_obj[i]);
        }
    }
    for (i = 0; i < MWRNA_MAX_OBJ; i++) {
        if (mwg_rna_obj[i].used == 1) {
            mwRnaDestroy(&mwg_rna_obj[i]);
        }
    }
    memset(mwg_rna_obj, 0, sizeof(mwg_rna_obj));
    memset(mwg_rna_zero, 0, sizeof(mwg_rna_zero));
    p = mwg_rna_zero_adpcm;
    /* @hack redundant: a store of the fill word through p that never runs.
     * SHC then keeps p in R6 and the fill word in R5 (0x8c66eda8, 0x8c66edac).
     * without it, the fill loop's registers change and 6 of 63 words differ */
    if (0) {
        *p = 0x80808080;
    }
    /* @hack spelling: i stepped as i = i + 1, which SHC counts up with ADD #1
     * and CMP/GE (0x8c66edb2, 0x8c66edb4). as i++, it counts down with DT,
     * the function is 4 bytes shorter and 24 of 63 words differ */
    for (i = 0; i < MWRNA_ZERO_NWORD; i = i + 1) {
        *p++ = 0x80808080;
    }
}

void mwlRnaSetInitMember(MWRNA_OBJ *rna)
{
    rna->stat = 0;
    rna->ovrflw = 0;
    rna->copying = 0;
    rna->flush = 0;
    rna->ndata = 0;
    rna->wrpos = 0;
    rna->rdpos = 0;
    rna->sfreq = MWRNA_SND_SFREQ;
    rna->bps = 16;
    rna->playndata = MWRNA_DEF_STARTSMPL;
    rna->startsmpl = MWRNA_DEF_STARTSMPL;
    rna->nplay = 0;
    rna->outvol = 0;
    rna->fxch = 0;
    rna->fxlev = 0;
    rna->semi = 0;
    rna->fine = 0;
    rna->coff = 0;
    rna->q = 0;
    rna->pause = 0;
    rna->swsave = 0;
}

MWRNA_OBJ *mwlRnaAlloc(void)
{
    MWRNA_OBJ *rna;
    Sint32 i;

    rna = 0;
    for (i = 0; i < MWRNA_MAX_OBJ; i++) {
        if (mwg_rna_obj[i].used == 0) {
            rna = &mwg_rna_obj[i];
            break;
        }
    }
    return rna;
}

MWRNA mwRnaCreate(SJ *sj, Sint32 nch)
{
    MWRNA_OBJ *rna;
    SJ *p; /* NOLINT(clang-diagnostic-unused-but-set-variable) */
    Sint32 ptr;
    Sint32 size;
    Sint32 imask;
    Sint32 i;

    if (nch < 1 || nch > MWRNA_MAX_NCH) {
        mwRnaErrCallFunc(TEXT_2);
        return 0;
    }
    if (sj == 0) {
        mwRnaErrCallFunc(TEXT_3);
        return 0;
    }
    /* @hack redundant: p, a copy of sj that nothing reads. SHC then walks the
     * joints from a copy of sj (MOV R11,R4 at 0x8c66ee9e). without it, SHC
     * adds sj to 0 instead and 114 of 133 words differ */
    p = sj;
    for (i = 0; i < nch; i++) {
        if (sj[i] == 0) {
            mwRnaErrCallFunc(TEXT_3);
            return 0;
        }
    }
    mwlRnaLockCs(&imask);
    rna = mwlRnaAlloc();
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_4);
    } else {
        rna->nch = nch;
        rna->maxnch = nch;
        for (i = 0; i < nch; i++) {
            rna->sj[i] = sj[i];
        }
        rna->snd = mwSndOpenPort(nch);
        if (rna->snd == 0) {
            mwRnaErrCallFunc(TEXT_5);
            rna = 0;
        } else {
            mwlRnaSetInitMember(rna);
            mwRnaSetNumChan(rna, rna->nch);
            for (i = 0; i < nch; i++) {
                mwSndGetBufInfo(rna->snd, i, &ptr, &size);
                rna->bufptr[i] = ptr;
            }
            rna->bufsize = size / rna->bps * 8;
            rna->used = 1;
        }
    }
    mwlRnaUnlockCs(&imask);
    return rna;
}

void mwRnaDestroy(MWRNA rna)
{
    Sint32 imask;
    Sint32 i;
    Sint32 err;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    mwlRnaLockCs(&imask);
    if (rna->stat != 0) {
        mwRnaStop(rna);
    }
    rna->used = 0;
    for (i = 0; i < rna->maxnch; i++) {
        rna->sj[i] = 0;
    }
    err = mwSndClosePort(rna->snd);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_7);
    }
    memset(rna, 0, sizeof(*rna));
    mwlRnaUnlockCs(&imask);
}

void mwRnaStart(MWRNA rna)
{
    Sint32 imask;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    mwlRnaLockCs(&imask);
    if (rna->stat != 0) {
        mwRnaStop(rna);
    }
    rna->stat = MWRNA_SW_AUTO;
    rna->playndata = rna->startsmpl;
    mwRnaSetTransSw(rna, 1);
    mwlRnaUnlockCs(&imask);
}

/* start copying size bytes from src to channel ch's ring at sound memory
 * offset dst. it returns size, or 0 for a size of 0 or less. a copy that fails
 * to start is reported, and its size still returned */
Sint32 mwlRnaStartCopy(MWRNA_OBJ *rna, Sint32 ch, void *src, Sint32 dst,
                       Sint32 size)
{
    Sint32 err;

    if (size <= 0) {
        return 0;
    }
    err = mwSndTrns(rna->snd, ch, dst, src, size);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_8);
    }
    return size;
}

void mwlRnaClearBuf(MWRNA_OBJ *rna)
{
}

void mwRnaStop(MWRNA rna)
{
    Sint32 imask;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    if (rna->stat != 0) {
        mwlRnaLockCs(&imask);
        mwRnaSetPlaySw(rna, 0);
        mwRnaSetTransSw(rna, 0);
        mwlRnaClearBuf(rna);
        rna->pause = 0;
        rna->stat = 0;
        mwlRnaUnlockCs(&imask);
    }
}

void mwRnaFlush(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    if (rna->flush != 1) {
        rna->flush = 1;
    }
}

void mwRnaPause(MWRNA rna)
{
    Sint32 imask;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    mwlRnaLockCs(&imask);
    if (rna->pause == 0) {
        rna->swsave = mwRnaGetTransSw(rna) | mwRnaGetPlaySw(rna);
        mwRnaSetPlaySw(rna, 0);
        mwRnaSetTransSw(rna, 0);
        rna->pause = 1;
    }
    mwlRnaUnlockCs(&imask);
}

void mwRnaContinue(MWRNA rna)
{
    Sint32 ncount;
    Sint32 imask;
    Sint32 tscale;
    Sint32 ndata;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    mwlRnaLockCs(&imask);
    if (rna->pause == 1) {
        /* @bug logic: swsave is never 2. mwRnaPause stores the or of
         * mwRnaGetTransSw and mwRnaGetPlaySw, each 0 or 1 (OR R0,R8 at
         * 0x8c66f16e), so the test against 2 always holds (CMP/EQ #2,R0 at
         * 0x8c66f1ca). a renderer paused with play on and the transfer off
         * resumes with both on */
        if (rna->swsave != 2 && rna->swsave != 0) {
            mwRnaGetTime(rna, &ncount, &tscale);
            ndata = mwRnaGetNumData(rna);
            mwRnaSetTransSw(rna, 1);
            rna->ntotal = ncount + ndata;
            mwRnaExecServer();
            mwRnaSetPlaySw(rna, 1);
            if (rna->stat == (MWRNA_SW_AUTO | MWRNA_SW_TRANS)) {
                rna->playndata = MWRNA_NO_AUTOPLAY;
            }
        }
        rna->pause = 0;
    }
    mwlRnaUnlockCs(&imask);
}

Sint32 mwRnaIsPause(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return 0;
    }
    return rna->pause;
}

/* whether the copy running has finished: 1 once every channel's memory block
 * reports it finished, else 0. while trnsnsmpl, the samples of the last copy
 * of samples, is 0 or less, it returns 1 at once. it does so even when the
 * copy running is of silence. a failed status read is reported and counts as
 * unfinished */
Sint32 mwlRnaIsCopyEnd(MWRNA_OBJ *rna)
{
    SDE_MEMBLK_STAT stat;
    Sint32 done[MWRNA_MAX_NCH];
    Sint32 ret;
    Sint32 err;
    Sint32 i;

    if (rna->trnsnsmpl <= 0) {
        return 1;
    }
    ret = 1;
    done[0] = done[1] = 1;
    for (i = 0; i < rna->nch; i++) {
        err = mwSndGetMblkStat(rna->snd, i, &stat);
        if (err != 0) {
            ret = 0;
            mwRnaErrCallFunc(TEXT_9);
            break;
        }
        if (stat != SDE_MEMBLK_STAT_TRANSFER_FINISHED) {
            done[i] = 0;
        }
    }
    if (done[0] == 0 || done[1] == 0) {
        ret = 0;
    }
    return ret;
}

void mwlRnaAddWrPos(MWRNA_OBJ *rna)
{
    SJCK ck;
    SJCK ck1;
    SJCK ck2;
    Sint32 i;
    char *src;
    Sint32 dst;

    for (i = 0; i < rna->nch; i++) {
        ck.data = rna->ck[i].data;
        ck.len = rna->ck[i].len;
        mwSjSplitChunk(&ck, rna->trnsnsmpl * rna->bps / 8, &ck1, &ck2);
        SJ_PUTCHUNK(rna->sj[i], SJ_LIN_FREE, &ck1);
        SJ_UNGETCHUNK(rna->sj[i], SJ_LIN_DATA, &ck2);
    }
    /* at a rate other than MWRNA_SND_SFREQ, a copy written at a ring's start
     * also has its first 4 bytes copied past the ring's end. the reason is
     * unknown */
    if (rna->wrpos == 0 && rna->sfreq != MWRNA_SND_SFREQ) {
        for (i = 0; i < rna->nch; i++) {
            src = rna->ck[i].data;
            dst = rna->bufptr[i] + (rna->bufsize * rna->bps / 8);
            /* @hack redundant: a call of mwlRnaSetSvrInt that never runs.
             * SHC then keeps the copy's size, 4, and its source and
             * destination in saved registers R9 to R11 (0x8c66f38a to
             * 0x8c66f3a4). without it, they take R11, R6 and R7, the function
             * runs 4 bytes short and 59 of 141 words differ */
            if (0) {
                mwlRnaSetSvrInt();
            }
            mwlRnaStartCopy(rna, i, src, dst, 4);
        }
    }
    rna->wrpos = (rna->wrpos + rna->trnsnsmpl) % rna->bufsize;
    rna->ndata += rna->trnsnsmpl;
    rna->ntotal += rna->trnsnsmpl;
}

void mwlRnaAddFlshPos(MWRNA_OBJ *rna)
{
    Sint32 *zero;
    Sint32 i;
    Sint32 dst;

    /* the first 4 bytes of silence written at a ring's start are copied past
     * its end too, as in mwlRnaAddWrPos */
    if (rna->flshpos == 0 && rna->sfreq != MWRNA_SND_SFREQ) {
        if (rna->bps == 4) {
            zero = mwg_rna_zero_adpcm;
        } else {
            zero = mwg_rna_zero;
        }
        for (i = 0; i < rna->nch; i++) {
            dst = rna->bufptr[i] + (rna->bufsize * rna->bps / 8);
            /* @hack redundant: a call of mwlRnaSetSvrInt that never runs.
             * SHC then saves R9 (0x8c66f41a), and keeps the divisor 8 in R9
             * and the copy's size, 4, in R10 (0x8c66f432, 0x8c66f43c).
             * without it, R9 goes unsaved, the function runs 6 bytes short and
             * 69 of 79 words differ */
            if (0) {
                mwlRnaSetSvrInt();
            }
            mwlRnaStartCopy(rna, i, zero, dst, 4);
        }
    }
    rna->flshpos = (rna->flshpos + rna->flshnsmpl) % rna->bufsize;
    rna->nflsh += rna->flshnsmpl;
}

void mwRnaSetTransSw(MWRNA rna, Sint32 sw)
{
    Sint32 imask;
    Sint32 i;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    mwlRnaLockCs(&imask);
    if (sw == 0) {
        if (mwRnaGetTransSw(rna) != 0) {
            if (rna->copying == 1) {
                for (i = 0; i < MWRNA_WAIT_MAX; i++) {
                    if (mwlRnaIsCopyEnd(rna) == 1) {
                        mwlRnaAddWrPos(rna);
                        rna->copying = 0;
                        break;
                    }
                }
            }
            rna->flshpos = rna->wrpos;
            rna->nflsh = 0;
            rna->stat &= MWRNA_SW_PLAY | MWRNA_SW_AUTO;
        }
    } else if (sw == 1) {
        if (mwRnaGetTransSw(rna) != 1) {
            if (rna->copying == 1) {
                for (i = 0; i < MWRNA_WAIT_MAX; i++) {
                    if (mwlRnaIsCopyEnd(rna) == 1) {
                        mwlRnaAddFlshPos(rna);
                        rna->copying = 0;
                        break;
                    }
                }
            }
            rna->wrpos = 0;
            rna->rdpos = 0;
            rna->ndata = 0;
            rna->copying = 0;
            rna->flush = 0;
            rna->nplay = 0;
            rna->ovrflw = 0;
            rna->stat |= MWRNA_SW_TRANS;
            rna->ntotal = 0;
        }
    }
    mwlRnaUnlockCs(&imask);
}

void mwRnaSetPlaySw(MWRNA rna, Sint32 sw)
{
    Sint32 imask;
    Sint32 err;
    char *msg;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    err = 0;
    mwlRnaLockCs(&imask);
    if (sw == 0) {
        if (mwRnaGetPlaySw(rna) != 0) {
            err = mwSndStop(rna->snd);
            if (err != 0) {
                msg = TEXT_10;
            } else {
                err = mwSndFlushCmd(rna->snd, 0);
                if (err != 0) {
                    msg = TEXT_11;
                } else {
                    rna->stat &= MWRNA_SW_TRANS | MWRNA_SW_AUTO;
                }
            }
        }
    } else if (sw == 1) {
        if (mwRnaGetPlaySw(rna) != 1) {
            err = mwSndPlay(rna->snd, rna->bps, rna->sfreq);
            if (err != 0) {
                msg = TEXT_12;
            } else {
                err = mwSndFlushCmd(rna->snd, 1);
                if (err != 0) {
                    msg = TEXT_11;
                } else {
                    rna->stat |= MWRNA_SW_PLAY;
                }
            }
        }
    }
    mwlRnaUnlockCs(&imask);
    if (err != 0) {
        mwRnaErrCallFunc(msg);
    }
}

void mwlRnaAddRdPos(MWRNA_OBJ *rna)
{
    int pos;
    Sint32 old;
    Sint32 n;
    Sint32 rest;
    Sint32 err;

    old = rna->rdpos;
    err = mwSndGetPlayPos(rna->snd, 0, &pos);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_13);
    }
    rna->rdpos = (pos >> 4) << 4;
    n = rna->rdpos - old;
    if (n < 0) {
        n += rna->bufsize;
    }
    rest = rna->ndata - n;
    if (rest < 0) {
        rest = 0;
        n = rna->wrpos - old;
        rna->ovrflw = 1;
    }
    if (mwRnaIsOvrflw(rna) == 1 && mwRnaGetTransSw(rna) == 1) {
        rna->wrpos = rna->rdpos;
        rna->ovrflw = 0;
    }
    rna->ndata = rest;
    rna->nplay += n;
}

void mwlRnaStartTrans(MWRNA_OBJ *rna)
{
    Sint32 room;
    Sint32 n;
    Sint32 rest;
    Sint32 nbyte;
    Sint32 size;
    Sint32 bps;
    Sint32 i;
    SJ sj;
    char *src;
    Sint32 dst;

    room = ((rna->bufsize - rna->ndata) >> 4) << 4;
    for (i = 0; i < rna->nch; i++) {
        sj = rna->sj[i];
        sj->vtbl->GetChunk(sj, SJ_LIN_DATA, room * rna->bps / 8, &rna->ck[i]);
    }
    bps = rna->bps;
    if ((n = rna->ck[0].len / bps * 8) >= room) {
        n = room;
    }
    rest = rna->bufsize - rna->wrpos;
    n = (n >= rest) ? rest : n;
    n = (n >> 4) << 4;
    /* @hack redundant: the divisor written as 0.0F ? 0 : 8, which SHC loads on
     * its own by MOV #8,R0 (0x8c66f78c). a plain 8 takes R12, which holds 8
     * for the other divisions, the loop below takes other registers, and 63
     * of 149 words differ */
    nbyte = n * bps / (0.0F ? 0 : 8);
    for (i = 0; i < rna->nch; i++) {
        src = rna->ck[i].data;
        dst = rna->bufptr[i] + (rna->wrpos * rna->bps / 8);
        mwg_rna_tcnt[0] = syTmrGetCount();
        size = mwlRnaStartCopy(rna, i, src, dst, nbyte);
        mwg_rna_tcnt[1] = syTmrGetCount();
    }
    rna->trnsnsmpl = size / rna->bps * 8;
}

void mwlRnaInsrtSlntDat(MWRNA_OBJ *rna)
{
    Sint32 bps;
    Sint32 room;
    Sint32 unit;
    Sint32 rest;
    Sint32 n;
    Sint32 lim;
    Sint32 nbyte;
    Sint32 size;
    Sint32 *zero;
    Sint32 i;
    Sint32 dst;

    room = rna->bufsize - rna->ndata - rna->nflsh;
    bps = rna->bps;
    unit = 32 / bps * 8;
    room = room / unit * unit;
    if (room < (rest = rna->bufsize - rna->flshpos)) {
        n = room;
    } else {
        n = rest;
        /* @hack redundant: a goto to the next statement, which loads _divls's
         * address into R2 for the division at 0x8c66f8a4, and into R3 for the
         * one at 0x8c66f8bc. without it, the two trade and 4 of 126 words
         * differ */
        goto clamp;
    }
clamp:
    lim = (Sint32)sizeof(mwg_rna_zero) / bps * 8;
    if (n >= lim) {
        n = lim;
    } else {
        /* @hack redundant: an empty else, which loads _divls's address into R3
         * for the division at 0x8c66f8bc. without it, R2 takes it and 2 of 126
         * words differ */
    }
    n = n / unit * unit;
    /* @hack redundant: the divisor written as 0.0F ? 0 : 8, as in
     * mwlRnaStartTrans, which SHC loads on its own by MOV #8,R0 (0x8c66f8ce).
     * a plain 8 takes R12 and 64 of 126 words differ */
    nbyte = n * bps / (0.0F ? 0 : 8);
    if (bps == 4) {
        zero = mwg_rna_zero_adpcm;
    } else {
        zero = mwg_rna_zero;
    }
    for (i = 0; i < rna->nch; i++) {
        dst = rna->bufptr[i] + (rna->flshpos * rna->bps / 8);
        /* @hack redundant: a call of mwRnaSetPlaySw that never runs. its two
         * reads of rna make SHC save R9 (0x8c66f82e) and keep the saved
         * registers in the image's order. without it, the function runs 8
         * bytes short and 116 of 126 words differ */
        if (0) {
            mwRnaSetPlaySw(rna, rna->pause);
        }
        size = mwlRnaStartCopy(rna, i, zero, dst, nbyte);
    }
    rna->flshnsmpl = size / rna->bps * 8;
}

void mwlRnaExecSvrHndl(MWRNA_OBJ *rna)
{
    if (mwRnaGetTransSw(rna) == 1 && rna->copying == 1 &&
        mwlRnaIsCopyEnd(rna) == 1) {
        mwlRnaAddWrPos(rna);
        rna->copying = 0;
    }
    if (rna->flush == 1) {
        if (rna->sj[0]->vtbl->GetNumData(rna->sj[0], SJ_LIN_DATA) <
            MWRNA_FLUSH_MIN) {
            mwRnaSetTransSw(rna, 0);
            rna->flush = 0;
        }
    }
    if (mwRnaGetPlaySw(rna) == 1) {
        mwlRnaAddRdPos(rna);
        if (mwRnaGetTransSw(rna) == 0 && rna->nflsh < rna->bufsize) {
            if (rna->copying == 1 && mwlRnaIsCopyEnd(rna) == 1) {
                mwlRnaAddFlshPos(rna);
                rna->copying = 0;
            }
            if (rna->copying == 0) {
                mwlRnaInsrtSlntDat(rna);
                rna->copying = 1;
            }
        }
    }
    if (mwRnaGetTransSw(rna) == 1 && rna->copying == 0) {
        mwlRnaStartTrans(rna);
        rna->copying = 1;
    }
    if (rna->stat == (MWRNA_SW_AUTO | MWRNA_SW_TRANS) &&
        rna->ndata >= rna->playndata && rna->pause == 0) {
        mwRnaSetPlaySw(rna, 1);
        rna->playndata = MWRNA_NO_AUTOPLAY;
    }
    if (rna->stat == (MWRNA_SW_AUTO | MWRNA_SW_PLAY) && rna->ndata == 0) {
        mwRnaSetPlaySw(rna, 0);
        mwlRnaClearBuf(rna);
    }
}

void mwRnaExecServer(void)
{
    Sint32 imask;
    Uint32 i;

    mwlRnaLockCs(&imask);
    for (i = 0; i < MWRNA_MAX_OBJ; i++) {
        if (mwg_rna_obj[i].used == 1) {
            mwlRnaExecSvrHndl(&mwg_rna_obj[i]);
        }
    }
    mwlRnaUnlockCs(&imask);
}

void mwRnaGetTime(MWRNA rna, Sint32 *ncount, Sint32 *tscale)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    *ncount = rna->ntotal - rna->ndata;
    *tscale = mwRnaGetSfreq(rna);
}

Sint32 mwRnaGetStat(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    return rna->stat;
}

Sint32 mwRnaGetTransSw(MWRNA rna)
{
    Sint32 sw;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    sw = rna->stat & MWRNA_SW_TRANS;
    return sw;
}

Sint32 mwRnaGetPlaySw(MWRNA rna)
{
    Sint32 sw;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    sw = rna->stat & MWRNA_SW_PLAY;
    sw >>= 1;
    return sw;
}

Sint32 mwRnaGetAutoSw(MWRNA rna)
{
    Sint32 sw;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    sw = rna->stat & MWRNA_SW_AUTO;
    sw >>= 2;
    return sw;
}

Sint32 mwRnaGetNumData(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    return rna->ndata;
}

Sint32 mwRnaGetNumRoom(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    return rna->bufsize - rna->ndata;
}

void mwRnaSetNumChan(MWRNA rna, Sint32 nch)
{
    Sint32 err;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    rna->nch = nch;
    err = mwSndSetUseCh(rna->snd, nch);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_14);
    }
    if (nch == 1) {
        rna->outpan[0] = 0;
        rna->outpan[1] = 0;
    } else {
        rna->outpan[0] = -15;
        rna->outpan[1] = 15;
    }
    mwRnaSetOutPan(rna, 0, rna->outpan[0]);
    mwRnaSetOutPan(rna, 1, rna->outpan[1]);
}

void mwRnaSetStartSmpl(MWRNA rna, Sint32 smpl)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    rna->startsmpl = smpl;
}

void mwRnaSetBitPerSmpl(MWRNA rna, Sint32 bps)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    rna->bufsize = rna->bufsize * rna->bps / 8;
    rna->bps = bps;
    rna->bufsize = rna->bufsize / bps * 8;
}

void mwRnaSetSfreq(MWRNA rna, Sint32 sfreq)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    rna->sfreq = sfreq;
}

void mwRnaSetOutVol(MWRNA rna, Sint32 vol)
{
    Sint32 att;
    Sint32 err;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    if (vol >= 0) {
        rna->outvol = 0;
        att = 0;
    } else if (vol <= -999) {
        rna->outvol = -999;
        att = 255;
    } else {
        rna->outvol = vol;
        att = ((vol * 255) - 128) / -999;
    }
    err = mwSndSetVol(rna->snd, att);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_15);
    }
}

void mwRnaSetOutPan(MWRNA rna, Sint32 ch, Sint32 pan)
{
    Sint32 err;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    if (pan < -15) {
        pan = -15;
    } else if (pan > 15) {
        pan = 15;
    }
    rna->outpan[ch] = pan;
    err = mwSndSetPan(rna->snd, ch, pan);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_16);
    }
}

void mwRnaSetFx(MWRNA rna, Sint32 ch, Sint32 lev)
{
    Sint32 level;
    Sint32 err;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    if (lev < -42) {
        level = 0;
    } else if (lev >= 0) {
        level = 15;
    } else {
        level = (lev / 3) + 15;
    }
    if (ch >= 0 && ch <= 16) {
        rna->fxch = ch;
        rna->fxlev = lev;
        err = mwSndSetFx(rna->snd, ch, level);
        if (err != 0) {
            mwRnaErrCallFunc(TEXT_17);
        }
    }
}

void mwRnaSetPitch(MWRNA rna, Sint32 semi, Sint32 fine)
{
    Sint32 err;

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    rna->semi = semi;
    rna->fine = fine;
    err = mwSndSetPitch(rna->snd, semi, fine);
    if (err != 0) {
        mwRnaErrCallFunc(TEXT_18);
    }
}

Sint32 mwRnaGetBitPerSmpl(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    return rna->bps;
}

Sint32 mwRnaGetSfreq(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    return rna->sfreq;
}

Sint32 mwRnaGetOutVol(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    return rna->outvol;
}

Sint32 mwRnaGetOutPan(MWRNA rna, Sint32 ch)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    return rna->outpan[ch];
}

void mwRnaGetFx(MWRNA rna, Sint32 *ch, Sint32 *lev)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        *ch = -1;
        *lev = -1;
        return;
    }
    *ch = rna->fxch;
    *lev = rna->fxlev;
}

void mwRnaGetPitch(MWRNA rna, Sint32 *semi, Sint32 *fine)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        *semi = -1;
        *fine = -1;
        return;
    }
    *semi = rna->semi;
    *fine = rna->fine;
}

void mwRnaEntryErrFunc(MW_PLY_ERRFN errfn, void *obj)
{
    mwRnaErrEntryFunc(errfn, obj);
}

Sint32 mwRnaIsOvrflw(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return -1;
    }
    return rna->ovrflw;
}

void mwRnaClearOvrflw(MWRNA rna)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    rna->ovrflw = 0;
}

void mwRnaSetFilter(MWRNA rna, Sint32 coff, Sint32 q)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    rna->coff = coff;
    rna->q = q;
    mwSndSetFilter(rna->snd, coff, q);
}

void mwRnaGetFilter(MWRNA rna, Sint32 *coff, Sint32 *q)
{
    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    *coff = rna->coff;
    *q = rna->q;
}

void mwRnaSetStmHdInfo(MWRNA rna, char *hdinfo, Sint32 *result)
{
    int bps;
    int coff;
    int q;
    int fxch;
    int fxlev;
    int vol;
    int sfreq;
    int pan[MWRNA_MAX_NCH];

    if (rna == 0) {
        mwRnaErrCallFunc(TEXT_6);
        return;
    }
    mwSndSetHdInfo(rna->snd);
    mwSndGetBitPerSmpl(rna->snd, &bps);
    if (bps == 0) {
        return;
    }
    mwSndGetSfreq(rna->snd, &sfreq);
    mwSndGetVol(rna->snd, &vol);
    mwSndGetPan(rna->snd, 0, &pan[0]);
    mwSndGetPan(rna->snd, 1, &pan[1]);
    mwSndGetFx(rna->snd, &fxch, &fxlev);
    mwSndGetFilter(rna->snd, &coff, &q);
    mwRnaSetBitPerSmpl(rna, bps);
    mwRnaSetSfreq(rna, sfreq);
    mwRnaSetOutVol(rna, vol);
    mwRnaSetOutPan(rna, 0, pan[0]);
    mwRnaSetOutPan(rna, 1, pan[1]);
    mwRnaSetFx(rna, fxch, fxlev);
    mwRnaSetFilter(rna, coff, q);
}

void mwRnaGetHdPrm(MWRNA rna, Sint32 arg1, Sint32 arg2)
{
    mwSndGetHdPrm(rna, arg1, arg2);
}
