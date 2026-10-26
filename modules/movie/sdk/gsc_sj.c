/* GSC's calls on the joints its streams read into */
#include <sg_mwgsc.h>
#include <sg_xpt.h>
#include <sj.h>

#define GSC_ERR_SJ_SIZE (-200) /* a line that reports a negative size */
#define GSC_ERR_SJ_NULL (-201) /* a null joint */
#define GSC_ERR_SJ_SHORT (-202) /* a free chunk shorter than asked for */

static Sint32 sj_line_bytes(SJ sj, Sint32 id, Sint32 *size);
static Sint32 sj_can_get_sectors(SJ sj, Sint32 id, Sint32 sectors,
                                 Sint32 *available);

Sint32 mwGscSjGetFreeBsize(SJ sj, Sint32 *size)
{
    /* @hack spelling: sj_line_bytes's result through result, which makes the
     * call a BSR (0x8ced1d78); returned straight, SHC branches to it and 9 of
     * 10 words differ */
    Sint32 result = sj_line_bytes(sj, SJ_LIN_FREE, size);

    return result;
}

/* store the byte count of the joint's line id in *size. return 0, or
 * GSC_ERR_SJ_NULL or GSC_ERR_SJ_SIZE */
static Sint32 sj_line_bytes(SJ sj, Sint32 id, Sint32 *size)
{
    Sint32 bytes;

    if (mwGscIsHnNull(sj)) {
        return mwGscSetErrCode(GSC_ERR_SJ_NULL);
    }
    bytes = sj->vtbl->GetNumData(sj, id);
    if (bytes < 0) {
        return mwGscSetErrCode(GSC_ERR_SJ_SIZE);
    }
    *size = bytes;
    return 0;
}

Sint32 mwGscSjGetDataBsize(SJ sj, Sint32 *size)
{
    /* @hack spelling: sj_line_bytes's result through result, which makes the
     * call a BSR (0x8ced1dd8); returned straight, SHC branches to it and 9 of
     * 10 words differ */
    Sint32 result = sj_line_bytes(sj, SJ_LIN_DATA, size);

    return result;
}

Sint32 mwGscSjIsGetFreeChunk(SJ sj, Sint32 limit, Sint32 bufsize,
                             Sint32 *sectors)
{
    Sint32 bytes;
    Sint32 count;
    Sint32 err;

    *sectors = 0;
    err = mwGscSjGetDataBsize(sj, &bytes);
    if (err != 0) {
        return 0;
    }
    count = (bufsize - bytes) / MWGSC_SCT_LEN;
    if (count <= 0) {
        return 0;
    }
    count = count < limit ? count : limit;
    return sj_can_get_sectors(sj, SJ_LIN_FREE, count, sectors);
}

/* return 1 when line id can give a chunk of the sectors asked for, and else 0.
 * *available gets the sectors it can give, which are all of them on 1 */
static Sint32 sj_can_get_sectors(SJ sj, Sint32 id, Sint32 sectors,
                                 Sint32 *available)
{
    Sint32 bytes;
    Sint32 result;
    Sint32 nbyte = sectors * MWGSC_SCT_LEN;

    *available = sectors;
    result = sj->vtbl->IsGetChunk(sj, id, nbyte, &bytes);
    if (result == 1) {
        return 1;
    }
    *available = bytes / MWGSC_SCT_LEN;
    return 0;
}

Sint32 mwGscSjGetFreeChunk(SJ sj, Sint32 sectors, SJCK *ck)
{
    SJCK got;
    Sint32 nbyte = sectors * MWGSC_SCT_LEN;

    sj->vtbl->GetChunk(sj, SJ_LIN_FREE, nbyte, &got);
    if (nbyte == got.len) {
        *ck = got;
        return 1;
    }
    mwGscSetErrCode(GSC_ERR_SJ_SHORT);
    return 0;
}

void mwGscSjPutDataChunk(SJ sj, Sint32 nbyte, SJCK *ck)
{
    SJCK first;
    SJCK rest;

    mwSjSplitChunk(ck, nbyte, &first, &rest);
    sj->vtbl->PutChunk(sj, SJ_LIN_DATA, &first);
    *ck = rest;
}

void mwGscSjUngetFreeChunk(SJ sj, SJCK *ck)
{
    if (ck->len > 0) {
        sj->vtbl->UngetChunk(sj, SJ_LIN_FREE, ck);
        ck->data = NULL;
        ck->len = 0;
    }
}

void mwGscSjReset(SJ sj)
{
    sj->vtbl->Reset(sj);
}
