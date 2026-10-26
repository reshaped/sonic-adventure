/* middleware ring buffer joints. a joint's buffer has extra bytes after its
 * end, so that a chunk can run past the end in one piece */
#include <machine.h>
#include <sg_ov.h>
#include <sg_xpt.h>
#include <sj.h>
#include <string.h>

#include "movie/sdk/sjrbf.h"

#define SJRBF_MAX 32
#define SJRBF_VERSION_NO 1
#define SJRBF_REVISION_NO 4

/* a joint over the bsize bytes at buf. the writer takes a chunk of free bytes
 * at wr_get, and puts it back, written, at wr_put. the reader takes a chunk of
 * data at rd_get, and puts it back, read, at rd_put. a chunk taken and not yet
 * put back runs from wr_put to wr_get, or from rd_put to rd_get. free_len
 * counts the free bytes not taken, and data_len the data bytes */
struct sj_rbf {
    SJ_OBJ sj;
    UUID *uuid;
    Bool used;
    Sint32 data_len;
    Sint32 free_len;
    char *wr_put;
    char *rd_put;
    char *wr_get;
    char *rd_get;
    char *buf;
    Sint32 bsize;
    /* the extra bytes after the buffer's end, which mirror its start */
    Sint32 xsize;
    void (*err_func)(void *obj, Sint32 ecode);
    void *err_obj;
    Sint32 unk_38;
};

void mwSjRbfInitG(void);
void mwSjRbfFinishG(void);
void mwSjRbfExecServerG(void);
SJ mwSjRbfCreate(char *buf, Sint32 bsize, Sint32 xsize);
void mwSjRbfDestroy(SJ sj);
UUID *mwSjRbfGetUuid(SJ sj);
void mwSjRbfEntryErrFunc(SJ sj, void (*func)(void *obj, Sint32 ecode),
                         void *obj);
void mwSjRbfReset(SJ sj);
Sint32 mwSjRbfGetNumData(SJ sj, Sint32 id);
void mwSjRbfGetChunk(SJ sj, Sint32 id, Sint32 nbyte, SJCK *ck);
void getWriteChunk(struct sj_rbf *rbf, Sint32 nbyte, SJCK *ck);
void getWriteSize(struct sj_rbf *rbf, Sint32 *nbyte);
void getReadChunk(struct sj_rbf *rbf, Sint32 nbyte, SJCK *ck);
void getReadSize(struct sj_rbf *rbf, Sint32 *nbyte);
void mwSjRbfPutChunk(SJ sj, Sint32 id, SJCK *ck);
void putWriteChunk(struct sj_rbf *rbf, SJCK *ck);
Sint32 chkPutWriteChunk(struct sj_rbf *rbf, SJCK *ck);
void cpyBgnBuf(struct sj_rbf *rbf, SJCK *ck);
void cpyExtBuf(struct sj_rbf *rbf, SJCK *ck);
void putReadChunk(struct sj_rbf *rbf, SJCK *ck);
Sint32 chkPutReadChunk(struct sj_rbf *rbf, SJCK *ck);
void mwSjRbfUngetChunk(SJ sj, Sint32 id, SJCK *ck);
void ungetWriteChunk(struct sj_rbf *rbf, SJCK *ck);
Sint32 chkUngetWriteChunk(struct sj_rbf *rbf, SJCK *ck);
void ungetReadChunk(struct sj_rbf *rbf, SJCK *ck);
Sint32 chkUngetReadChunk(struct sj_rbf *rbf, SJCK *ck);
Sint32 mwSjRbfIsGetChunk(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte);
Bool isGetWriteChunk(struct sj_rbf *rbf, Sint32 nbyte, Sint32 *rbyte);
Bool isGetReadChunk(struct sj_rbf *rbf, Sint32 nbyte, Sint32 *rbyte);
char *mwSjRbfGetBufPtr(SJ sj);
Sint32 mwSjRbfGetBufSize(SJ sj);
Sint32 mwSjRbfGetXtrSize(SJ sj);
Sint32 mwSjRbfSetErrCode(struct sj_rbf *rbf, Sint32 ecode);
Sint32 mwSjRbfLock(struct sj_rbf *rbf);
void mwSjRbfUnlock(struct sj_rbf *rbf, Sint32 mask);
char *satuLoopMaxPtr(char *bgn, char *end, char *p);
Sint32 satuLoopMaxVal(Sint32 bgn, Sint32 end, Sint32 val);
char *satuLoopNumPtr(char *bgn, Sint32 num, char *p);
Sint32 satuLoopNumVal(Sint32 bgn, Sint32 num, Sint32 val);

/* NOLINTBEGIN(clang-diagnostic-unterminated-string-initialization) */
const char mwg_sj_rbf_cpy[32] = "Copyright (c) 1998 SEGA";
const char mwg_sj_rbf_ver[32] = "MWSJ_RBF Version 1.04 1998-10-27";
/* NOLINTEND(clang-diagnostic-unterminated-string-initialization) */

static UUID sj_rbf_uuid = SJRBF_LID;
/* every joint's function table, which mwSjRbfInit makes mwg_sj_rbf_i's */
SJ_IF mwg_sj_rbf_vtbl = {NULL,
                         NULL,
                         NULL,
                         mwSjRbfDestroy,
                         mwSjRbfGetUuid,
                         mwSjRbfReset,
                         mwSjRbfGetChunk,
                         mwSjRbfUngetChunk,
                         mwSjRbfPutChunk,
                         mwSjRbfGetNumData,
                         mwSjRbfIsGetChunk,
                         mwSjRbfEntryErrFunc};
/* the interface mwg_sj_rbf_libhdl gives the overlay library. only Init and
 * Finish are set */
struct sj_rbf_lif mwg_sj_rbf_lif = {NULL,         NULL,           NULL,
                                    mwSjRbfInitG, mwSjRbfFinishG, NULL};

/* the library's handle, in DSGLH, MOVIE.BIN's library table */
#pragma section LH
OVS_LIB_OBJ mwg_sj_rbf_libhdl = {(struct _ov_lif *)&mwg_sj_rbf_lif,
                                 SJRBF_VERSION_NO,
                                 SJRBF_REVISION_NO,
                                 0,
                                 0,
                                 0,
                                 SJRBF_LID};
#pragma section

struct sj_rbf mwg_sj_rbf_obj[SJRBF_MAX];

void mwSjRbfInit(void)
{
    mwg_sj_rbf_i = &mwg_sj_rbf_vtbl;
    mwSjRbfInitG();
}

void mwSjRbfInitG(void)
{
    memset(mwg_sj_rbf_obj, 0, sizeof(mwg_sj_rbf_obj));
}

void mwSjRbfFinishG(void)
{
}

void mwSjRbfExecServerG(void)
{
}

SJ mwSjRbfCreate(char *buf, Sint32 bsize, Sint32 xsize)
{
    Sint32 i;
    struct sj_rbf *rbf;

    for (i = 0; i < SJRBF_MAX; ++i) {
        if (mwg_sj_rbf_obj[i].used == FALSE) {
            break;
        }
    }
    if (i == SJRBF_MAX) {
        return NULL;
    }
    rbf = &mwg_sj_rbf_obj[i];
    rbf->sj.vtbl = &mwg_sj_rbf_vtbl;
    rbf->uuid = &sj_rbf_uuid;
    rbf->used = TRUE;
    rbf->buf = buf;
    rbf->bsize = bsize;
    rbf->xsize = xsize;
    mwSjRbfReset((SJ)rbf);
    return (SJ)rbf;
}

void mwSjRbfDestroy(SJ sj)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;

    if (rbf != NULL) {
        memset(rbf, 0, sizeof(*rbf));
        rbf->used = FALSE;
    }
}

UUID *mwSjRbfGetUuid(SJ sj)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;

    return rbf->uuid;
}

void mwSjRbfEntryErrFunc(SJ sj, void (*func)(void *obj, Sint32 ecode),
                         void *obj)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;

    rbf->err_func = func;
    rbf->err_obj = obj;
}

void mwSjRbfReset(SJ sj)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;

    rbf->data_len = 0;
    rbf->free_len = rbf->bsize;
    rbf->wr_put = rbf->buf;
    rbf->rd_put = rbf->buf;
    rbf->wr_get = rbf->buf;
    rbf->rd_get = rbf->buf;
}

Sint32 mwSjRbfGetNumData(SJ sj, Sint32 id)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;
    Sint32 nbyte = 0;

    switch (id) {
    case SJ_LIN_FREE:
        nbyte = rbf->free_len;
        break;
    case SJ_LIN_DATA:
        nbyte = rbf->data_len;
        break;
    default:
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        break;
    }
    return nbyte;
}

void mwSjRbfGetChunk(SJ sj, Sint32 id, Sint32 nbyte, SJCK *ck)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;
    Sint32 mask;

    ck->len = 0;
    ck->data = NULL;
    mask = mwSjRbfLock(rbf);
    switch (id) {
    case SJ_LIN_FREE:
        getWriteChunk(rbf, nbyte, ck);
        break;
    case SJ_LIN_DATA:
        getReadChunk(rbf, nbyte, ck);
        break;
    default:
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        break;
    }
    mwSjRbfUnlock(rbf, mask);
}

/* take up to nbyte free bytes at wr_get as *ck, after wrapping wr_put and
 * wr_get into the buffer. while a chunk of free bytes is taken, it returns at
 * once and leaves *ck unwritten. an nbyte outside 0 to bsize reports
 * SJ_ERR_PRM, and also leaves *ck unwritten. a negative room reports
 * SJ_ERR_INTERNAL, but the take still runs and gives *ck that room as its
 * length. a free_len outside 0 to bsize after the take also reports
 * SJ_ERR_INTERNAL */
void getWriteChunk(struct sj_rbf *rbf, Sint32 nbyte, SJCK *ck)
{
    Sint32 len;
    Sint32 got;
    Sint32 left;

    if (rbf->wr_put != rbf->wr_get) {
        return;
    }
    if (nbyte < 0 || nbyte > rbf->bsize) {
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        return;
    }
    rbf->wr_put = satuLoopNumPtr(rbf->buf, rbf->bsize, rbf->wr_put);
    rbf->wr_get = satuLoopNumPtr(rbf->buf, rbf->bsize, rbf->wr_get);
    getWriteSize(rbf, &len);
    if (len < 0) {
        mwSjRbfSetErrCode(rbf, SJ_ERR_INTERNAL);
    }
    got = nbyte < len ? nbyte : len;
    ck->len = got;
    ck->data = rbf->wr_get;
    rbf->wr_get += got;
    left = rbf->free_len - got;
    rbf->free_len = left;
    if (left < 0 || left > rbf->bsize) {
        mwSjRbfSetErrCode(rbf, SJ_ERR_INTERNAL);
    }
}

/* *nbyte gets the free bytes one chunk at wr_get can hold, at most free_len.
 * with wr_get below rd_put, the chunk can reach rd_put. otherwise it can
 * reach the end of the extra bytes or rd_put + bsize, whichever is lower */
void getWriteSize(struct sj_rbf *rbf, Sint32 *nbyte)
{
    /* @hack redundant: register on end, which forms the buffer's end in R6
     * and xsize in R7 (MOV.L @(36,R4),R6 at 0x8cecfe56, ADD R3,R6 at
     * 0x8cecfe5a); without it, the two trade and 3 of 39 words differ */
    register char *end;
    char *xend;
    Sint32 n;
    char *wrap;
    /* @hack redundant: register on len, which loads free_len into R7 and
     * *nbyte into R6 (0x8cecfe88, 0x8cecfe8a); without it, the two trade and
     * 5 of 39 words differ */
    register Sint32 len;

    *nbyte = 0;
    end = rbf->buf + rbf->bsize;
    if (rbf->wr_get < rbf->rd_put) {
        *nbyte = rbf->rd_put - rbf->wr_get;
    } else {
        n = rbf->xsize;
        xend = end + n;
        if (xend < (wrap = rbf->rd_put + rbf->bsize)) {
            end = xend;
        } else {
            end = wrap;
        }
        *nbyte = end - rbf->wr_get;
    }
    /* @hack spelling: the two loads as one expression, which reads free_len
     * into R7 and *nbyte into R6 (0x8cecfe88, 0x8cecfe8a); as two statements,
     * they take R6 and R4 and 5 of 39 words differ */
    n = rbf->free_len, len = *nbyte;
    *nbyte = n < len ? n : len;
}

void getReadChunk(struct sj_rbf *rbf, Sint32 nbyte, SJCK *ck)
{
    Sint32 len;
    Sint32 got;
    Sint32 left;

    if (rbf->rd_put != rbf->rd_get) {
        return;
    }
    if (nbyte < 0 || nbyte > rbf->bsize) {
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        return;
    }
    rbf->rd_put = satuLoopNumPtr(rbf->buf, rbf->bsize, rbf->rd_put);
    rbf->rd_get = satuLoopNumPtr(rbf->buf, rbf->bsize, rbf->rd_get);
    getReadSize(rbf, &len);
    if (len < 0) {
        mwSjRbfSetErrCode(rbf, SJ_ERR_INTERNAL);
    }
    got = nbyte < len ? nbyte : len;
    ck->len = got;
    ck->data = rbf->rd_get;
    rbf->rd_get += got;
    left = rbf->data_len - got;
    rbf->data_len = left;
    if (left < 0 || left > rbf->bsize) {
        mwSjRbfSetErrCode(rbf, SJ_ERR_INTERNAL);
    }
}

/* *nbyte gets the data bytes one chunk at rd_get can hold, at most data_len.
 * it counts as getWriteSize does, with rd_get and wr_put in place of wr_get
 * and rd_put */
void getReadSize(struct sj_rbf *rbf, Sint32 *nbyte)
{
    /* @hack redundant: register on end, which forms the buffer's end in R6
     * and xsize in R7 (MOV.L @(36,R4),R6 at 0x8cecff32, ADD R3,R6 at
     * 0x8cecff36); without it, the two trade and 3 of 39 words differ */
    register char *end;
    char *xend;
    Sint32 n;
    char *wrap;
    /* @hack redundant: register on len, which loads data_len into R7 and
     * *nbyte into R6 (0x8cecff64, 0x8cecff66); without it, the two trade and
     * 5 of 39 words differ */
    register Sint32 len;

    *nbyte = 0;
    end = rbf->buf + rbf->bsize;
    if (rbf->rd_get < rbf->wr_put) {
        *nbyte = rbf->wr_put - rbf->rd_get;
    } else {
        n = rbf->xsize;
        xend = end + n;
        if (xend < (wrap = rbf->wr_put + rbf->bsize)) {
            end = xend;
        } else {
            end = wrap;
        }
        *nbyte = end - rbf->rd_get;
    }
    /* @hack spelling: the two loads as one expression, which reads data_len
     * into R7 and *nbyte into R6 (0x8cecff64, 0x8cecff66); as two statements,
     * they take R6 and R4 and 5 of 39 words differ */
    n = rbf->data_len, len = *nbyte;
    *nbyte = n < len ? n : len;
}

void mwSjRbfPutChunk(SJ sj, Sint32 id, SJCK *ck)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;
    Sint32 mask = mwSjRbfLock(rbf);

    switch (id) {
    case SJ_LIN_DATA:
        putWriteChunk(rbf, ck);
        break;
    case SJ_LIN_FREE:
        putReadChunk(rbf, ck);
        break;
    default:
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        break;
    }
    mwSjRbfUnlock(rbf, mask);
}

/* add the written chunk ck to the data line, if chkPutWriteChunk accepts it.
 * what it wrote into the first xsize bytes is copied to their mirror after
 * the buffer's end. what it wrote past the end is copied back to the start */
void putWriteChunk(struct sj_rbf *rbf, SJCK *ck)
{
    char *end = rbf->buf + rbf->bsize;
    Sint32 err = chkPutWriteChunk(rbf, ck);

    if (err == 0) {
        if (rbf->wr_put < rbf->buf + rbf->xsize) {
            cpyBgnBuf(rbf, ck);
        }
        if (rbf->wr_put + ck->len > end) {
            cpyExtBuf(rbf, ck);
        }
        rbf->wr_put += ck->len;
        rbf->data_len += ck->len;
    }
}

/* 0 when ck starts at wr_put and ends by wr_get, within the taken free chunk.
 * otherwise it reports and returns SJ_ERR_PRM, or SJ_ERR_INTERNAL when wr_get
 * is below wr_put */
Sint32 chkPutWriteChunk(struct sj_rbf *rbf, SJCK *ck)
{
    Sint32 len = rbf->wr_get - rbf->wr_put;

    if (len < 0) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_INTERNAL);
    }
    if (ck->data != rbf->wr_put) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
    }
    if (ck->len < 0 || ck->len > len) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
    }
    return 0;
}

void cpyBgnBuf(struct sj_rbf *rbf, SJCK *ck)
{
    char *buf = rbf->buf;
    Sint32 bsize = rbf->bsize;
    char *src = rbf->wr_put;
    char *dst = src + bsize;
    Uint32 len;

    len = ck->len < buf + rbf->xsize - src ? ck->len : buf + rbf->xsize - src;
    memcpy(dst, src, len);
}

void cpyExtBuf(struct sj_rbf *rbf, SJCK *ck)
{
    char *end = rbf->buf + rbf->bsize;
    char *src;
    char *dst;
    Sint32 len;

    if (rbf->wr_put < end) {
        dst = rbf->buf;
        src = end;
        len = rbf->wr_put + ck->len - end;
    } else {
        end = rbf->wr_put;
        dst = end - rbf->bsize;
        src = end;
        len = ck->len;
    }
    memcpy(dst, src, len);
}

void putReadChunk(struct sj_rbf *rbf, SJCK *ck)
{
    Sint32 err = chkPutReadChunk(rbf, ck);

    if (err == 0) {
        rbf->rd_put += ck->len;
        rbf->free_len += ck->len;
    }
}

Sint32 chkPutReadChunk(struct sj_rbf *rbf, SJCK *ck)
{
    Sint32 len = rbf->rd_get - rbf->rd_put;

    if (len < 0) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_INTERNAL);
    }
    if (ck->data != rbf->rd_put) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
    }
    if (ck->len < 0 || ck->len > len) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
    }
    return 0;
}

void mwSjRbfUngetChunk(SJ sj, Sint32 id, SJCK *ck)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;
    Sint32 mask = mwSjRbfLock(rbf);

    switch (id) {
    case SJ_LIN_FREE:
        ungetWriteChunk(rbf, ck);
        break;
    case SJ_LIN_DATA:
        ungetReadChunk(rbf, ck);
        break;
    default:
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        break;
    }
    mwSjRbfUnlock(rbf, mask);
}

void ungetWriteChunk(struct sj_rbf *rbf, SJCK *ck)
{
    Sint32 err = chkUngetWriteChunk(rbf, ck);

    if (err == 0) {
        rbf->wr_get -= ck->len;
        rbf->free_len += ck->len;
    }
}

/* 0 when ck ends at wr_get, within the taken free chunk, so that only the
 * bytes taken last go back. otherwise it reports and returns SJ_ERR_PRM, or
 * SJ_ERR_INTERNAL when wr_get is below wr_put */
Sint32 chkUngetWriteChunk(struct sj_rbf *rbf, SJCK *ck)
{
    Sint32 len = rbf->wr_get - rbf->wr_put;

    if (len < 0) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_INTERNAL);
    }
    if (ck->data + ck->len != rbf->wr_get) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
    }
    if (ck->len < 0 || ck->len > len) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
    }
    return 0;
}

void ungetReadChunk(struct sj_rbf *rbf, SJCK *ck)
{
    Sint32 err = chkUngetReadChunk(rbf, ck);

    if (err == 0) {
        rbf->rd_get -= ck->len;
        rbf->data_len += ck->len;
    }
}

Sint32 chkUngetReadChunk(struct sj_rbf *rbf, SJCK *ck)
{
    Sint32 len = rbf->rd_get - rbf->rd_put;

    if (len < 0) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_INTERNAL);
    }
    if (ck->data + ck->len != rbf->rd_get) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
    }
    if (ck->len < 0 || ck->len > len) {
        return mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
    }
    return 0;
}

Sint32 mwSjRbfIsGetChunk(SJ sj, Sint32 id, Sint32 nbyte, Sint32 *rbyte)
{
    Bool can = FALSE;
    struct sj_rbf *rbf = (struct sj_rbf *)sj;
    Sint32 mask = mwSjRbfLock(rbf);

    switch (id) {
    case SJ_LIN_FREE:
        can = isGetWriteChunk(rbf, nbyte, rbyte);
        break;
    case SJ_LIN_DATA:
        can = isGetReadChunk(rbf, nbyte, rbyte);
        break;
    default:
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        break;
    }
    mwSjRbfUnlock(rbf, mask);
    return can;
}

/* TRUE when a free chunk of nbyte bytes can be taken now, and *rbyte gets the
 * bytes one can take, up to nbyte. it wraps wr_put and wr_get as
 * getWriteChunk does. FALSE with *rbyte 0 while a free chunk is taken and
 * not yet put back, or for an nbyte outside 0 to bsize, which reports
 * SJ_ERR_PRM */
Bool isGetWriteChunk(struct sj_rbf *rbf, Sint32 nbyte, Sint32 *rbyte)
{
    Sint32 len;
    Sint32 got;

    *rbyte = 0;
    if (rbf->wr_put != rbf->wr_get) {
        return FALSE;
    }
    if (nbyte < 0 || nbyte > rbf->bsize) {
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        return FALSE;
    }
    rbf->wr_put = satuLoopNumPtr(rbf->buf, rbf->bsize, rbf->wr_put);
    rbf->wr_get = satuLoopNumPtr(rbf->buf, rbf->bsize, rbf->wr_get);
    getWriteSize(rbf, &len);
    got = nbyte < len ? nbyte : len;
    *rbyte = nbyte < got ? nbyte : got;
    if (*rbyte < nbyte) {
        return FALSE;
    }
    return TRUE;
}

Bool isGetReadChunk(struct sj_rbf *rbf, Sint32 nbyte, Sint32 *rbyte)
{
    Sint32 len;
    Sint32 got;

    *rbyte = 0;
    if (rbf->rd_put != rbf->rd_get) {
        return FALSE;
    }
    if (nbyte < 0 || nbyte > rbf->bsize) {
        mwSjRbfSetErrCode(rbf, SJ_ERR_PRM);
        return FALSE;
    }
    rbf->rd_put = satuLoopNumPtr(rbf->buf, rbf->bsize, rbf->rd_put);
    rbf->rd_get = satuLoopNumPtr(rbf->buf, rbf->bsize, rbf->rd_get);
    getReadSize(rbf, &len);
    got = nbyte < len ? nbyte : len;
    *rbyte = nbyte < got ? nbyte : got;
    if (*rbyte < nbyte) {
        return FALSE;
    }
    return TRUE;
}

char *mwSjRbfGetBufPtr(SJ sj)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;

    return rbf->buf;
}

Sint32 mwSjRbfGetBufSize(SJ sj)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;

    return rbf->bsize;
}

Sint32 mwSjRbfGetXtrSize(SJ sj)
{
    struct sj_rbf *rbf = (struct sj_rbf *)sj;

    return rbf->xsize;
}

/* report ecode to the joint's error function, if it has one, and return
 * ecode. 0 reports nothing */
Sint32 mwSjRbfSetErrCode(struct sj_rbf *rbf, Sint32 ecode)
{
    void (*func)(void *obj, Sint32 ecode);

    if (ecode == 0) {
        return 0;
    }
    func = rbf->err_func;
    if (func != NULL) {
        func(rbf->err_obj, ecode);
    }
    return ecode;
}

/* mask every interrupt, and return the mask it replaced, for mwSjRbfUnlock to
 * restore */
Sint32 mwSjRbfLock(struct sj_rbf *rbf)
{
    Sint32 mask = get_imask();

    set_imask(IMASK_ALL);
    return mask;
}

void mwSjRbfUnlock(struct sj_rbf *rbf, Sint32 mask)
{
    set_imask(mask);
}

/* p moved by whole laps of end - bgn + 1 bytes into bgn through end, both
 * included */
char *satuLoopMaxPtr(char *bgn, char *end, char *p)
{
    return (char *)satuLoopMaxVal((Sint32)bgn, (Sint32)end, (Sint32)p);
}

Sint32 satuLoopMaxVal(Sint32 bgn, Sint32 end, Sint32 val)
{
    Sint32 num = end - bgn + 1;

    if (val > end || val < bgn) {
        val -= bgn;
        val %= num;
        if (val < 0) {
            val += num;
        }
        val += bgn;
    }
    return val;
}

/* satuLoopMaxPtr over the num bytes from bgn */
char *satuLoopNumPtr(char *bgn, Sint32 num, char *p)
{
    return (char *)satuLoopNumVal((Sint32)bgn, num, (Sint32)p);
}

Sint32 satuLoopNumVal(Sint32 bgn, Sint32 num, Sint32 val)
{
    return satuLoopMaxVal(bgn, bgn + num - 1, val);
}
