/* GSC's queries and settings of a stream */
#include <sg_mwgsc.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>

#include "movie/sdk/gsc.h"

Sint32 mwGscGetStat(struct gsc_stream *hn, Sint32 *stat)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    *stat = hn->stat;
    return 0;
}

Sint32 mwGscGetCurOfst(struct gsc_stream *hn, Sint32 *ofst)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    *ofst = hn->cur_ofst;
    return 0;
}

Sint32 mwGscGetStmSize(struct gsc_stream *hn, Sint32 *size)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    *size = hn->stm_size;
    return 0;
}

SJ mwGscGetSjHn(struct gsc_stream *hn)
{
    if (mwGscIsHnNull(hn)) {
        return NULL;
    }
    return hn->sj;
}

Sint32 mwGscGetFlowLimit(struct gsc_stream *hn, Sint32 *low, Sint32 *high)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    *low = hn->flow_low;
    *high = hn->flow_high;
    return 0;
}

Sint32 mwGscGetRdSct(struct gsc_stream *hn, Sint32 *nsct)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    *nsct = hn->rd_sct;
    return 0;
}

Sint32 mwGscGetTrSct(struct gsc_stream *hn, Sint32 *nsct)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    *nsct = hn->tr_sct;
    return 0;
}

Sint32 mwGscSetFlowLimit(struct gsc_stream *hn, Sint32 low, Sint32 high)
{
    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    if (high < 0 || high > hn->bufsize) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    if (low < 0 || low > hn->bufsize) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    if (low > high) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    hn->flow_low = low;
    hn->flow_high = high;
    return 0;
}

Sint32 mwGscSetRdSct(struct gsc_stream *hn, Sint32 nsct)
{
    Sint32 most;

    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    most = hn->bufsize / MWGSC_SCT_LEN;
    if (nsct < 0 || nsct > most) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    hn->rd_sct = nsct;
    return 0;
}

Sint32 mwGscSetTrSct(struct gsc_stream *hn, Sint32 nsct)
{
    Sint32 most;

    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    most = hn->rd_sct;
    if (nsct < 0 || nsct > most) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    hn->tr_sct = nsct;
    return 0;
}

Sint32 mwGscSetOfstRange(struct gsc_stream *hn, Sint32 ofst, Sint32 end)
{
    Sint32 size;

    if (mwGscIsHnNull(hn)) {
        return mwGscSetErrCode(GSC_ERR_HN);
    }
    size = hn->stm_size;
    if (end == GSC_OFST_END) {
        end = size;
    }
    if (ofst < 0 || ofst > size) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    if (end < 0 || end > size) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    if (ofst > end) {
        return mwGscSetErrCode(GSC_ERR_RANGE);
    }
    hn->ofst_start = ofst;
    hn->ofst_end = end;
    return 0;
}

Bool mwGscIsHnNull(void *hn)
{
    if (hn == NULL) {
        return TRUE;
    }
    return FALSE;
}

GDFS mwGscGetGdfsHn(struct gsc_stream *hn)
{
    if (mwGscIsHnNull(hn)) {
        return NULL;
    }
    return hn->gdfs;
}
