/* GSC calls into the GD-ROM file system */
#include <machine.h>
#include <sg_mwgsc.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "movie/sdk/gsc.h"

#define GSC_ERR_GD_READ (-100) /* a failed read */
#define GSC_ERR_GD_OPEN (-102) /* a file that will not open */
#define GSC_ERR_GD_FSIZE (-103) /* a file whose size cannot be read */
#define GSC_ERR_GD_RANGE (-104) /* a sector range that will not open */
#define GSC_ERR_GD_HN (-105) /* a stream whose file handle is null */
#define GSC_ERR_GD_SEEK (-106) /* a failed seek */
#define GSC_ERR_GD_REQRD (-107) /* a read that will not start */
#define GSC_ERR_GD_STAT (-108) /* an unknown file system state */

Sint32 gscGdLock(void);
void gscGdUnlock(Sint32 mask);

GDFS mwGscGdOpenFname(char *fname, Sint32 *fsize)
{
    GDFS gdfs = gdFsOpen(fname, NULL);
    Bool got;

    if (gdfs == NULL) {
        mwGscSetErrCode(GSC_ERR_GD_OPEN);
        return NULL;
    }
    /* @hack spelling: gdFsGetFileSize's result through got, which SHC copies
     * to R4 in the branch's slot (MOV R0,R4 at 0x8ced1724); tested in place,
     * 19 of 33 words differ */
    got = gdFsGetFileSize(gdfs, fsize);
    if (got != TRUE) {
        mwGscSetErrCode(GSC_ERR_GD_FSIZE);
        return NULL;
    }
    return gdfs;
}

GDFS mwGscGdOpenRange(Sint32 stsct, Sint32 nsct, Sint32 *fsize)
{
    GDFS gdfs = gdFsOpenRange(stsct, nsct);

    if (gdfs == NULL) {
        mwGscSetErrCode(GSC_ERR_GD_RANGE);
        return NULL;
    }
    *fsize = nsct * MWGSC_SCT_LEN;
    return gdfs;
}

Sint32 mwGscGdClose(struct gsc_stream *hn)
{
    GDFS gdfs = hn->gdfs;

    if (mwGscIsHnNull(gdfs)) {
        return mwGscSetErrCode(GSC_ERR_GD_HN);
    }
    mwGscGdStop(hn);
    gdFsClose(gdfs);
    return 0;
}

Sint32 mwGscGdStop(struct gsc_stream *hn)
{
    GDFS gdfs = hn->gdfs;

    if (mwGscIsHnNull(gdfs)) {
        return mwGscSetErrCode(GSC_ERR_GD_HN);
    }
    return 0;
}

Sint32 mwGscGdReqRd(struct gsc_stream *hn, Sint32 nsct, void *buf)
{
    GDFS gdfs = hn->gdfs;
    Sint32 result;

    if (mwGscIsHnNull(gdfs)) {
        return mwGscSetErrCode(GSC_ERR_GD_HN);
    }
    result = gdFsReqRd32(gdfs, nsct, buf);
    if (result < 0) {
        return mwGscSetErrCode(GSC_ERR_GD_REQRD);
    }
    if (result != nsct) {
        mwGscGdStop(hn);
        return mwGscSetErrCode(GSC_ERR_GD_READ);
    }
    return 0;
}

Sint32 mwGscGdGetNumRd(struct gsc_stream *hn, Sint32 last, Sint32 *nbyte,
                       Sint32 *total)
{
    GDFS gdfs = hn->gdfs;

    if (mwGscIsHnNull(gdfs)) {
        return mwGscSetErrCode(GSC_ERR_GD_HN);
    }
    *total = gdFsGetNumRd(gdfs);
    if (*total < 0) {
        return mwGscSetErrCode(GSC_ERR_GD_READ);
    }
    *nbyte = *total - last;
    return 0;
}

Sint32 mwGscGdGetGdfsStat(struct gsc_stream *hn, Sint32 *stat)
{
    Sint32 result = 0;
    GDFS gdfs = hn->gdfs;

    if (mwGscIsHnNull(gdfs)) {
        return mwGscSetErrCode(GSC_ERR_GD_HN);
    }
    *stat = gdFsGetStat(gdfs);
    switch (*stat) {
    case GDD_STAT_IDLE:
    case GDD_STAT_COMPLETE:
    case GDD_STAT_READ:
    case GDD_STAT_SEEK:
    case GDD_STAT_BUSY:
    case GDD_STAT_ERR:
    case GDD_STAT_FATAL:
    case GDD_STAT_CONTINUE:
    case GDD_STAT_UNDEF:
        break;
    default:
        result = mwGscSetErrCode(GSC_ERR_GD_STAT);
        break;
    }
    return result;
}

Bool mwGscGdIsComplete(struct gsc_stream *hn)
{
    Sint32 stat;
    Bool complete;

    /* @bug uninitialized: the query's error goes unchecked. with hn->gdfs
     * null, mwGscGdGetGdfsStat writes no stat, and the switch reads the word
     * the stack held (MOV.L @R15,R0 at 0x8ced18f4) */
#ifdef GCC
    /* repairs the unset stat the tag names: where the query writes none,
     * GCC's is GDD_STAT_UNDEF, so a stream with no gdfs isn't complete */
    stat = GDD_STAT_UNDEF;
#endif
    mwGscGdGetGdfsStat(hn, &stat);
    /* @hack spelling: a switch of one case, which tests stat with CMP/EQ #1 and
     * branches (BF and BRA at 0x8ced18f8, 0x8ced18fa); as an if, 9 of 14
     * words differ */
    switch (stat) {
    case GDD_STAT_COMPLETE:
        complete = TRUE;
        break;
    default:
        complete = FALSE;
        break;
    }
    return complete;
}

Sint32 mwGscGdSeek(struct gsc_stream *hn, Sint32 sctno)
{
    GDFS gdfs = hn->gdfs;
    Sint32 result;

    if (mwGscIsHnNull(gdfs)) {
        return mwGscSetErrCode(GSC_ERR_GD_HN);
    }
    result = gdFsSeek(gdfs, sctno, GDD_SEEK_SET);
    if (result != 0) {
        return mwGscSetErrCode(GSC_ERR_GD_SEEK);
    }
    return 0;
}

void mwGscGdExecServer(void)
{
    Sint32 mask = gscGdLock();

    gdFsExecServer();
    gscGdUnlock(mask);
}

/* mask every interrupt, and return the mask level as it was for gscGdUnlock */
Sint32 gscGdLock(void)
{
    Sint32 mask = get_imask();

    set_imask(IMASK_ALL);
    return mask;
}

void gscGdUnlock(Sint32 mask)
{
    set_imask(mask);
}
