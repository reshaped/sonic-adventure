/* GSC's server and its stream calls: open, close, start, stop and seek
 *
 * a stream's joint passes chunks of its buffer between two lines. its free
 * line holds the room, and its data line the bytes read. the server reads the
 * streams in turn. for each it takes a chunk of up to the stream's rd_sct
 * sectors from the free line. it fills the chunk a transfer of up to the
 * stream's tr_sct sectors at a time, and puts each transfer on the data line */
#include <sg_chain.h>
#include <sg_mwgsc.h>
#include <sg_mwply.h>
#include <sg_ov.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>
#include <string.h>

#include "movie/sdk/gsc.h"

/* the sectors that hold nbyte bytes, and 1 for 0 */
#define BYTE2SCT(nbyte) ((((nbyte) - 1) / MWGSC_SCT_LEN) + 1)
#define NONNEG(n) (((n) < 0) ? 0 : (n))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/* the stall count past which waitEndTrSct gives up on a transfer */
#define STALL_MAX 500000
/* the timer counts after which isRdTout reports a read overdue */
#define RD_TOUT 0x23c378

/* a new stream's rd_sct and tr_sct */
#define RD_SCT_DEFAULT 100
#define TR_SCT_DEFAULT 10

void initHn(Sint32 nhn, struct gsc_stream *hn);
Sint32 setExecServerVin(void);
void execServerVin(void *obj);
void stopForce(Sint32 no);
void closeAll(void);
struct gsc_stream *resetGscHn(struct gsc_stream *hn, GDFS gdfs, Sint32 fsize,
                              SJ sj);
Sint32 getSjBufSize(SJ sj, Sint32 *size);
void requestRead(void);
void prepRequest(void);
void chkSjBufMin(void);
Bool isPauseOffStm(Sint32 no);
struct gsc_stream *getNextReqStm(void);
Bool isReqRdStm(Sint32 no);
void deciReqSize(struct gsc_stream *hn, Sint32 nfree, Sint32 *nsct,
                 Sint32 *nbyte);
void execRequest(void);
void resetRdTmr(void);
Sint32 checkRead(void);
Sint32 chkEndTrSct(struct gsc_stream *hn, Sint32 *nbyte, Bool *done);
Bool isEndTrSct(struct gsc_stream *hn, Sint32 stat, Sint32 nbyte);
Bool isRdTout(void);
Bool isEndRdSct(void);
Bool isEndStm(struct gsc_stream *hn);
void procEndStm(struct gsc_stream *hn);
struct gsc_stream *allocGscHn(void);
void freeGscHn(struct gsc_stream *hn);
Sint32 stopRead(struct gsc_stream *hn);
Sint32 waitEndTrSct(void);
void schRedRdStmno(struct gsc_stream *hn, Sint32 *no);
Sint32 getSeekSct(struct gsc_stream *hn, Sint32 ofst, Sint32 whence,
                  Sint32 *sct);
Sint32 execSeekSct(struct gsc_stream *hn, Sint32 sct);

const char mwg_gsc_build[] = "\nmwGsc Ver.1.04 Build:Nov 11 1998 21:21:49\n";

struct gsc_lif mwg_gsc_lif = {
    NULL,
    NULL,
    NULL,
    mwGscInitG,
    mwGscFinishG,
    mwGscExecServerG,
    mwGscOpenFnameG,
    mwGscOpenRangeG,
    mwGscCloseG,
};
SYCHAIN mwg_chid_vblk = 0;

/* the library's handle, version 1.04. #pragma section LH puts it in section
 * DSGLH, which holds MOVIE.BIN's table of libraries */
#pragma section LH
OVS_LIB_OBJ mwg_gsc_libhdl = {
    (struct _ov_lif *)&mwg_gsc_lif, 1, 4, 0, 0, 0, GSC_LID};
#pragma section

struct gsc_work mwg_gsc_wk;
/* the times the vertical blank handler has run the server */
/* @hack type: volatile, which keeps execServerVin's store ahead of its
 * branch to mwGscExecServerG (MOV.L R2,@R3 at 0x8cecf190), and mwGscInitG's
 * store of 0 ahead of mwg_gsc_wk's load (0x8cecf120); without it, the first
 * moves into the branch's slot, and 11 of 38 words differ in the two */
volatile Uint32 mwg_gsc_svrcnt;

Sint32 mwGscInit(Sint32 nhn, void *work)
{
    /* @hack spelling: mwGscInitG's result through error, which makes the call
     * a BSR (0x8cecf0f4); returned straight, SHC branches to it and 9 of 9
     * words differ */
    Sint32 error;

    mwg_gsc_i = &mwg_gsc_lif;
    error = mwGscInitG(nhn, work);
    return error;
}

Sint32 mwGscInitG(Sint32 nhn, void *work)
{
    if (nhn <= 0) {
        return GSC_ERR_NHN;
    }
    initHn(nhn, work);
    mwg_gsc_svrcnt = 0;
    work =
        (void *)(((Uint32)work + GSC_WORK_ALIGN - 1) & ~(GSC_WORK_ALIGN - 1));
    mwg_gsc_wk.nhn = nhn;
    mwg_gsc_wk.hn = work;
    mwg_gsc_wk.cur = nhn - 1;
    return 0;
}

void initHn(Sint32 nhn, struct gsc_stream *hn)
{
    memset(hn, 0, nhn * sizeof(struct gsc_stream));
}

/* add execServerVin to the vertical blank chain, unless it is there already.
 * return 0, or GSC_ERR_CHAIN when the chain takes no handler */
Sint32 setExecServerVin(void)
{
    if (mwg_chid_vblk == 0) {
        mwg_chid_vblk =
            syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK, execServerVin,
                              MWD_PLY_PRI_VBLNK, NULL);
    }
    if (mwg_chid_vblk == 0) {
        return GSC_ERR_CHAIN;
    }
    return 0;
}

void execServerVin(void *obj)
{
    mwg_gsc_svrcnt++;
    mwGscExecServerG();
}

Sint32 mwGscReset(void)
{
    Sint32 nhn;
    struct gsc_stream *hn;
    Sint32 err;

    nhn = mwg_gsc_wk.nhn;
    hn = mwg_gsc_wk.hn;
    err = mwGscStop(&hn[mwg_gsc_wk.cur]);
    if (err != 0) {
        return err;
    }
    closeAll();
    initHn(nhn, hn);
    mwg_gsc_wk.nhn = nhn;
    mwg_gsc_wk.hn = hn;
    mwg_gsc_wk.cur = nhn - 1;
    return 0;
}

void closeAll(void)
{
    Sint32 i;
    struct gsc_stream *tbl;
    struct gsc_stream *hn;

    tbl = mwg_gsc_wk.hn;
    for (i = 0; i < mwg_gsc_wk.nhn; i++) {
        hn = &tbl[i];
        if (hn->used) {
            mwGscSjReset(hn->sj);
            mwGscCloseG(hn);
        }
    }
}

Sint32 mwGscFinishG(void)
{
    Sint32 err;

    err = mwGscReset();
    if (err != 0) {
        return err;
    }
    mwg_chid_vblk = 0;
    return 0;
}

/* start a transfer, or check the one in flight */
void mwGscExecServerG(void)
{
    if (mwg_gsc_wk.reading == FALSE) {
        requestRead();
    } else {
        checkRead();
    }
}

/* request the chunk's next transfer, or once the chunk is whole, take a new
 * chunk */
void requestRead(void)
{
    if (isEndRdSct()) {
        prepRequest();
    } else {
        execRequest();
    }
}

/* resume the drained streams, then take a chunk of the next stream's joint and
 * request its first transfer. a stream whose joint has no whole sector of room
 * below flow_high pauses */
void prepRequest(void)
{
    struct gsc_stream *hn;
    Sint32 nsct;
    Sint32 nfree;
    Sint32 nbyte;
    SJCK ck;
    Sint32 got;

    chkSjBufMin();
    hn = getNextReqStm();
    if (hn == NULL) {
        return;
    }
    mwGscSjIsGetFreeChunk(hn->sj, hn->rd_sct, hn->flow_high, &nfree);
    if (nfree <= 0) {
        hn->pause = TRUE;
        return;
    }
    deciReqSize(hn, nfree, &nsct, &nbyte);
    if (nsct <= 0 || nbyte <= 0) {
        return;
    }
    got = mwGscSjGetFreeChunk(hn->sj, nsct, &ck);
    if (got == 0) {
        mwGscSetErrCode(GSC_ERR_STAT);
        return;
    }
    mwg_gsc_wk.ck = ck;
    mwg_gsc_wk.req_sct = nsct;
    mwg_gsc_wk.tr_byte = 0;
    execRequest();
}

/* resume each paused stream whose joint holds flow_low bytes or fewer */
void chkSjBufMin(void)
{
    Sint32 i;
    struct gsc_stream *tbl;

    tbl = mwg_gsc_wk.hn;
    for (i = 0; i < mwg_gsc_wk.nhn; i++) {
        if (isPauseOffStm(i)) {
            tbl[i].pause = FALSE;
        }
    }
}

Bool isPauseOffStm(Sint32 no)
{
    struct gsc_stream *tbl;
    struct gsc_stream *hn;
    Sint32 nbyte;
    Sint32 err;

    tbl = mwg_gsc_wk.hn;
    hn = &tbl[no];
    if (hn->used == FALSE) {
        return FALSE;
    }
    if (hn->pause == FALSE) {
        return FALSE;
    }
    err = mwGscSjGetDataBsize(hn->sj, &nbyte);
    if (err != 0) {
        return FALSE;
    }
    if (nbyte > hn->flow_low) {
        return FALSE;
    }
    return TRUE;
}

/* return the first stream after mwg_gsc_wk.cur, in turn, that is reading and
 * not paused, or NULL. cur becomes that stream's index, or the index after cur
 * when none is */
struct gsc_stream *getNextReqStm(void)
{
    struct gsc_stream *found;
    struct gsc_stream *tbl;
    Sint32 i;
    Sint32 no;

    found = NULL;
    tbl = mwg_gsc_wk.hn;
    no = (mwg_gsc_wk.cur + 1) % mwg_gsc_wk.nhn;
    for (i = 0; i < mwg_gsc_wk.nhn; i++) {
        if (isReqRdStm(no)) {
            found = &tbl[no];
            break;
        }
        no = (no + 1) % mwg_gsc_wk.nhn;
    }
    mwg_gsc_wk.cur = no;
    return found;
}

Bool isReqRdStm(Sint32 no)
{
    struct gsc_stream *tbl;
    struct gsc_stream *hn;
    Bool req;

    tbl = mwg_gsc_wk.hn;
    req = FALSE;
    /* @hack spelling: hn assigned in the test, which builds the stream's
     * address in R5 from a copy of no (MOV R4,R5 at 0x8cecf402); assigned
     * first, SHC builds it in R4 itself, and 30 of 39 words differ */
    if ((hn = &tbl[no])->used == FALSE) {
        return FALSE;
    }
    if (hn->pause != FALSE) {
        return FALSE;
    }
    switch (hn->stat) {
    case GSC_STAT_STOP:
        break;
    case GSC_STAT_EXEC:
        req = TRUE;
        break;
    case GSC_STAT_END:
        break;
    default:
        mwGscSetErrCode(GSC_ERR_STAT);
        break;
    }
    return req;
}

/* store the read's size in *nsct and *nbyte: the rest of the stream, or nfree
 * sectors when the rest needs that many or more. both stay 0 unless nfree and
 * the rest's sectors are above 0. a stream at its end is marked so first */
void deciReqSize(struct gsc_stream *hn, Sint32 nfree, Sint32 *nsct,
                 Sint32 *nbyte)
{
    Sint32 rest;
    Sint32 rest_sct;

    *nsct = 0;
    *nbyte = 0;
    if (isEndStm(hn)) {
        procEndStm(hn);
    }
    rest = hn->ofst_end - hn->cur_ofst;
    rest_sct = BYTE2SCT(rest);
    if (nfree > 0 && rest_sct > 0) {
        if (rest_sct < nfree) {
            *nsct = rest_sct;
            *nbyte = rest;
        } else {
            *nsct = nfree;
            /* @hack spelling: nsct read back through the pointer, which has SHC
             * shift a copy of nfree (MOV R11,R2 at 0x8cecf4be); with nfree, it
             * shifts nfree's own register, and 28 of 49 words differ */
            *nbyte = *nsct * MWGSC_SCT_LEN;
        }
    }
}

/* request the chunk's next transfer into the rest of the chunk: the sectors
 * still to come, at most tr_sct */
void execRequest(void)
{
    SJCK ck;
    struct gsc_stream *tbl;
    struct gsc_stream *hn;
    Sint32 cur;
    Sint32 done;
    Sint32 nsct;
    Sint32 err;

    ck = mwg_gsc_wk.ck;
    cur = mwg_gsc_wk.cur;
    tbl = mwg_gsc_wk.hn;
    hn = &tbl[cur];
    done = mwg_gsc_wk.tr_byte / MWGSC_SCT_LEN;
    nsct = MIN(hn->tr_sct, mwg_gsc_wk.req_sct - done);
    err = mwGscGdReqRd(hn, nsct, ck.data);
    if (err == 0) {
        mwg_gsc_wk.rd_sct = nsct;
        mwg_gsc_wk.num_rd = 0;
        mwg_gsc_wk.reading = TRUE;
    }
}

void resetRdTmr(void)
{
    mwg_gsc_wk.rd_tmr = syTmrGetCount();
}

/* once the transfer is in, put its bytes on the joint's data line and move the
 * stream's offset past them. a stream read to its end is marked so once the
 * chunk is whole. return 0, or the error chkEndTrSct met */
Sint32 checkRead(void)
{
    struct gsc_stream *tbl;
    struct gsc_stream *hn;
    Sint32 cur;
    Sint32 nbyte;
    Bool done;
    SJCK ck;
    Sint32 err;

    cur = mwg_gsc_wk.cur;
    tbl = mwg_gsc_wk.hn;
    hn = &tbl[cur];
    ck = mwg_gsc_wk.ck;
    err = chkEndTrSct(hn, &nbyte, &done);
    if (err != 0) {
        return err;
    }
    if (done) {
        mwGscSjPutDataChunk(hn->sj, nbyte, &ck);
        mwg_gsc_wk.ck = ck;
        mwg_gsc_wk.tr_byte += nbyte;
        mwg_gsc_wk.reading = FALSE;
        hn->cur_ofst += nbyte;
        if (isEndRdSct() && isEndStm(hn)) {
            procEndStm(hn);
        }
    }
    return 0;
}

/* store in *done whether the transfer is over, and in *nbyte the bytes it has
 * moved. a transfer that ends short counts as whole, after GSC_ERR_SHORT.
 * return 0, or a GSC_ERR_GD_* error */
Sint32 chkEndTrSct(struct gsc_stream *hn, Sint32 *nbyte, Bool *done)
{
    Sint32 stat;
    Sint32 delta;
    Sint32 err;

    err = mwGscGdGetGdfsStat(hn, &stat);
    if (err != 0) {
        return err;
    }
    /* last takes stat's address, not a byte count, so delta holds no count.
     * only *nbyte, the total, is read */
    err = mwGscGdGetNumRd(hn, (Sint32)&stat, &delta, nbyte);
    if (err != 0) {
        return err;
    }
    mwg_gsc_wk.num_rd = *nbyte;
    if (isEndTrSct(hn, stat, *nbyte) == FALSE) {
        *done = FALSE;
        return 0;
    }
    if (*nbyte < mwg_gsc_wk.rd_sct * MWGSC_SCT_LEN) {
        mwGscSetErrCode(GSC_ERR_SHORT);
        *nbyte = mwg_gsc_wk.rd_sct * MWGSC_SCT_LEN;
    }
    *done = TRUE;
    return 0;
}

Bool isEndTrSct(struct gsc_stream *hn, Sint32 stat, Sint32 nbyte)
{
    if (stat != GDD_STAT_COMPLETE &&
        nbyte < mwg_gsc_wk.rd_sct * MWGSC_SCT_LEN) {
        return FALSE;
    }
    return TRUE;
}

Bool isRdTout(void)
{
    Uint32 dt;

    dt = syTmrDiffCount(mwg_gsc_wk.rd_tmr, syTmrGetCount());
    if (dt >= RD_TOUT) {
        return TRUE;
    }
    return FALSE;
}

/* TRUE once the chunk's transfers have moved every sector it asked for, and
 * else FALSE */
Bool isEndRdSct(void)
{
    if (mwg_gsc_wk.tr_byte / MWGSC_SCT_LEN >= mwg_gsc_wk.req_sct) {
        return TRUE;
    }
    return FALSE;
}

Bool isEndStm(struct gsc_stream *hn)
{
    if (hn->ofst_end > hn->cur_ofst) {
        return FALSE;
    }
    return TRUE;
}

void procEndStm(struct gsc_stream *hn)
{
    hn->stat = GSC_STAT_END;
}

struct gsc_stream *mwGscOpenFnameG(char *fname, SJ sj)
{
    struct gsc_stream *hn;
    GDFS gdfs;
    Sint32 fsize;

    if (mwGscIsHnNull(sj)) {
        mwGscSetErrCode(GSC_ERR_SJ);
        return NULL;
    }
    hn = allocGscHn();
    if (hn == NULL) {
        mwGscSetErrCode(GSC_ERR_NOHN);
        return NULL;
    }
    gdfs = mwGscGdOpenFname(fname, &fsize);
    if (gdfs == NULL) {
        return NULL;
    }
    return resetGscHn(hn, gdfs, fsize, sj);
}

/* set hn up as a stopped stream over all of the file, and seek it to its
 * start. both flow limits start at the joint's size. return hn, or NULL when
 * the joint's size, the range or the seek fails */
struct gsc_stream *resetGscHn(struct gsc_stream *hn, GDFS gdfs, Sint32 fsize,
                              SJ sj)
{
    Sint32 size;
    Sint32 err;

    err = getSjBufSize(sj, &size);
    if (err != 0) {
        return NULL;
    }
    hn->used = TRUE;
    hn->stat = GSC_STAT_STOP;
    hn->bufsize = size;
    hn->flow_low = size;
    hn->flow_high = size;
    hn->cur_ofst = 0;
    hn->stm_size = fsize;
    hn->rd_sct = RD_SCT_DEFAULT;
    hn->tr_sct = TR_SCT_DEFAULT;
    hn->pause = FALSE;
    hn->sj = sj;
    hn->gdfs = gdfs;
    err = mwGscSetOfstRange(hn, 0, GSC_OFST_END);
    if (err != 0) {
        return NULL;
    }
    err = mwGscSeekSct(hn, 0, GDD_SEEK_SET);
    if (err != 0) {
        return NULL;
    }
    return hn;
}

struct gsc_stream *allocGscHn(void)
{
    struct gsc_stream *found;
    struct gsc_stream *tbl;
    struct gsc_stream *hn;
    Sint32 i;

    found = NULL;
    tbl = mwg_gsc_wk.hn;
    for (i = 0; i < mwg_gsc_wk.nhn; i++) {
        hn = &tbl[i];
        if (hn->used == FALSE) {
            found = hn;
            break;
        }
    }
    return found;
}

/* store the joint's size, its free and data bytes together, in *size. return
 * 0, or the error a query of the joint met. the test for a count below 0
 * never holds: a query that meets one returns GSC_ERR_SJ_SIZE and stores
 * nothing */
Sint32 getSjBufSize(SJ sj, Sint32 *size)
{
    Sint32 nfree;
    Sint32 ndata;
    Sint32 err;

    err = mwGscSjGetFreeBsize(sj, &nfree);
    if (err != 0) {
        return err;
    }
    err = mwGscSjGetDataBsize(sj, &ndata);
    if (err != 0) {
        return err;
    }
    if (nfree < 0 || ndata < 0) {
        return mwGscSetErrCode(GSC_ERR_BUFSIZE);
    }
    *size = nfree + ndata;
    return 0;
}

struct gsc_stream *mwGscOpenRangeG(Sint32 stsct, Sint32 nsct, SJ sj)
{
    struct gsc_stream *hn;
    GDFS gdfs;
    Sint32 fsize;

    if (mwGscIsHnNull(sj)) {
        mwGscSetErrCode(GSC_ERR_SJ);
        return NULL;
    }
    hn = allocGscHn();
    if (hn == NULL) {
        mwGscSetErrCode(GSC_ERR_NOHN);
        return NULL;
    }
    gdfs = mwGscGdOpenRange(stsct, nsct, &fsize);
    if (gdfs == NULL) {
        return NULL;
    }
    return resetGscHn(hn, gdfs, fsize, sj);
}

Sint32 mwGscCloseG(struct gsc_stream *hn)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    mwGscStop(hn);
    mwGscGdClose(hn);
    freeGscHn(hn);
    return 0;
}

void freeGscHn(struct gsc_stream *hn)
{
    memset(hn, 0, sizeof(struct gsc_stream));
}

Sint32 mwGscStart(struct gsc_stream *hn)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    /* @hack spelling: the default second among the cases, which SHC places
     * right after the tests (MOV #-2,R4 at 0x8cecf930); last, it follows the
     * cases, and 14 of 26 words differ */
    switch (hn->stat) {
    case GSC_STAT_EXEC:
        break;
    default:
        return mwGscSetErrCode(GSC_ERR_STAT);
    case GSC_STAT_STOP:
    case GSC_STAT_END:
        hn->stat = GSC_STAT_EXEC;
        break;
    }
    return 0;
}

Sint32 mwGscStop(struct gsc_stream *hn)
{
    Sint32 err;

    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    if (hn->stat == GSC_STAT_EXEC) {
        err = stopRead(hn);
        /* @hack redundant: an else after the return, which gives the stopped
         * state R2 (MOV #1,R2 at 0x8cecf972); without it, the state takes R3,
         * and 2 of 29 words differ */
        if (err != 0) {
            return err;
        } else {
            hn->stat = GSC_STAT_STOP;
        }
    }
    return 0;
}

/* for any stream but the server's current one, return 0 at once. otherwise
 * wait for the transfer in flight, return the rest of the chunk to the free
 * line, and mark no transfer in flight. the disc read isn't stopped:
 * stopForce's mwGscGdStop only checks the file handle. return 0, or the error
 * the wait met */
Sint32 stopRead(struct gsc_stream *hn)
{
    Sint32 no;
    SJ sj;
    Sint32 err;

    sj = hn->sj;
    schRedRdStmno(hn, &no);
    if (mwg_gsc_wk.cur != no) {
        return 0;
    }
    err = waitEndTrSct();
    if (isEndRdSct() == FALSE) {
        mwGscSjUngetFreeChunk(sj, &mwg_gsc_wk.ck);
        mwg_gsc_wk.tr_byte = mwg_gsc_wk.req_sct * MWGSC_SCT_LEN;
    }
    stopForce(no);
    mwg_gsc_wk.reading = FALSE;
    return err;
}

/* run the server until the transfer in flight is in. return 0, the error
 * checkRead met, or GSC_ERR_STALL once stall passes STALL_MAX */
Sint32 waitEndTrSct(void)
{
    Sint32 err;
    Sint32 stall;
    Sint32 num_rd;

    err = 0;
    stall = 0;
    for (;;) {
        if (mwg_gsc_wk.reading == FALSE) {
            break;
        }
        num_rd = mwg_gsc_wk.num_rd;
        err = checkRead();
        if (err != 0) {
            break;
        }
        if (num_rd == mwg_gsc_wk.num_rd) {
            stall++;
        } else {
            stall = 0;
        }
        /* stall++ here adds 1 on every pass. with the 1 above, a pass that
         * moves no bytes adds 2 */
        if (stall++ > STALL_MAX) {
            err = mwGscSetErrCode(GSC_ERR_STALL);
            break;
        }
        mwGscGdExecServer();
    }
    return err;
}

void stopForce(Sint32 no)
{
    struct gsc_stream *tbl;
    struct gsc_stream *hn;

    tbl = mwg_gsc_wk.hn;
    hn = &tbl[no];
    mwGscGdStop(hn);
}

/* store hn's index in the stream table in *no, or -1 when the table doesn't
 * hold it */
void schRedRdStmno(struct gsc_stream *hn, Sint32 *no)
{
    Sint32 i;
    struct gsc_stream *tbl;

    tbl = mwg_gsc_wk.hn;
    for (i = 0; i < mwg_gsc_wk.nhn; i++) {
        if (hn == &tbl[i]) {
            *no = i;
            return;
        }
    }
    *no = -1;
}

Sint32 mwGscSeekSct(struct gsc_stream *hn, Sint32 ofst, Sint32 whence)
{
    Sint32 sct;
    Sint32 nsct;
    Sint32 err;

    /* @bug memory: hn->stm_size is read before hn is tested (MOV.L @(24,R14),R1
     * at 0x8cecfaa8), so a null hn reads address 24 before the call returns
     * GSC_ERR_HN */
    nsct = NONNEG(BYTE2SCT(hn->stm_size));
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    err = stopRead(hn);
    if (err != 0) {
        return err;
    }
    err = getSeekSct(hn, ofst, whence, &sct);
    if (err != 0) {
        return err;
    }
    if (sct < 0 || sct > nsct) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    err = execSeekSct(hn, sct);
    if (err != 0) {
        return err;
    }
    hn->cur_ofst = sct * MWGSC_SCT_LEN;
    return 0;
}

Sint32 getSeekSct(struct gsc_stream *hn, Sint32 ofst, Sint32 whence,
                  Sint32 *sct)
{
    Sint32 nsct;
    Sint32 cur;

    nsct = NONNEG(BYTE2SCT(hn->stm_size));
    cur = NONNEG(hn->cur_ofst / MWGSC_SCT_LEN);
    *sct = 0;
    switch (whence) {
    case GDD_SEEK_SET:
        *sct = ofst;
        break;
    case GDD_SEEK_CUR:
        *sct = ofst + cur;
        break;
    case GDD_SEEK_END:
        *sct = ofst + nsct;
        break;
    default:
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    return 0;
}

/* seek hn's file to sector sct. a reading stream goes through mwGscStart
 * again, which leaves it reading */
Sint32 execSeekSct(struct gsc_stream *hn, Sint32 sct)
{
    Sint32 stat;
    Sint32 err;

    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    err = mwGscGetStat(hn, &stat);
    if (err != 0) {
        return err;
    }
    err = mwGscGdSeek(hn, sct);
    if (err != 0) {
        return err;
    }
    switch (stat) {
    case GSC_STAT_STOP:
        break;
    case GSC_STAT_EXEC:
        err = mwGscStart(hn);
        break;
    case GSC_STAT_END:
        break;
    default:
        mwGscSetErrCode(GSC_ERR_STAT);
        break;
    }
    return err;
}

void mwGscEntryErrFunc(void (*func)(void *obj, Sint32 code), void *obj)
{
    if (func == NULL) {
        mwg_gsc_wk.err_func = NULL;
        mwg_gsc_wk.err_obj = NULL;
        return;
    }
    mwg_gsc_wk.err_func = func;
    mwg_gsc_wk.err_obj = obj;
}

Sint32 mwGscSetErrCode(Sint32 code)
{
    void (*func)(void *obj, Sint32 code);

    mwg_gsc_wk.err_code = code;
    if (code == 0) {
        return 0;
    }
    func = mwg_gsc_wk.err_func;
    if (func != NULL) {
        func(mwg_gsc_wk.err_obj, code);
    }
    return code;
}
