/* GD file system file handles, reads and seeks */
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "shinobi/gdfs.h"

/* how many vertical blanks gdFsWaitCompleteReq waits for a request whose
 * trnsed doesn't change */
#define WAIT_TOUT 1200
/* how many times gdFsRead makes a request that is refused, and the passes
 * of the empty loop it waits between them */
#define READ_TRIES 20000
#define READ_DELAY 50000
/* the longest gdFsStopRd waits for the drive's wait value to reach 1: 20
 * seconds, in CPU timer counts of 1.28 microseconds */
#define STOP_TOUT 15625000
/* how long gdFsStopRd then waits before the abort: 2 milliseconds, in the
 * same counts */
#define STOP_DELAY 1563

static Sint32 request_cpu_read(GDFS gdfs, Sint32 nsct, void *buf);

Sint32 gdFsWaitCompleteReq(GDFS gdfs)
{
    Uint32 start = gdg_fs_vcnt;
    Sint32 trnsed = gdfs->trnsed;
    Sint32 stat;
    Uint32 elapsed;

    for (;;) {
        gdFsExecServer();
        stat = gdFsGetStat(gdfs);
        switch (stat) {
        case GDD_STAT_IDLE:
            return GDD_ERR_ILLREQ;
        case GDD_STAT_COMPLETE:
            return GDD_ERR_OK;
        case GDD_STAT_READ:
        case GDD_STAT_SEEK:
        case GDD_STAT_BUSY:
            if (trnsed != gdfs->trnsed) {
                start = gdg_fs_vcnt;
            }
            break;
        case GDD_STAT_ERR:
        case GDD_STAT_FATAL:
            return gdfs->err;
        default:
            return GDD_ERR_FATAL;
        }
        elapsed = gdg_fs_vcnt - start;
        if (elapsed > WAIT_TOUT) {
            return GDD_ERR_TOUT;
        }
    }
}

GDFS gdFsReopenHn(GDFS gdfs, Sint32 fid, Sint32 fad, Sint32 fsize)
{
    if (gdfs == NULL) {
        return NULL;
    }
    if (gdfs->wk->now_work == gdfs) {
        return NULL;
    }
    gdfs->fid = fid;
    gdfs->fad = fad;
    gdfs->fsize = fsize;
    gdfs->fsctsize = (gdfs->fsize + GDD_FS_SCTSIZE - 1) / GDD_FS_SCTSIZE;
    gdfs->ofs = 0;
    gdfs->trnsed = 0;
    gdfs->rsize = 0;
    gdfs->trsize = 0;
    gdfs->gdchn = 0;
    gdfs->ex_errcode = 0;
    gdfs->act = GDFS_ACT_NONE;
    gdfs->trflag = GDD_FS_TRANS_READY;
    gdfs->used = TRUE;
    gdfs->stat = GDD_STAT_IDLE;
    gdfs->err = GDD_ERR_OK;
    return gdfs;
}

GDFS_DIRREC gdFsCreateDirhn(void *dirbuf, Sint32 max_dirent)
{
    GDFS_DIRREC dirrec = dirbuf;

    dirrec->dir_num = 0;
    dirrec->max_ent = max_dirent;
    dirrec->dir_fad = 0;
    return dirrec;
}

Sint32 gdFsChangeDir(const char *dirname)
{
    return gdFsLoadDir(dirname, NULL);
}

void gdFsClose(GDFS gdfs)
{
    Sint32 stat;

    if (gdfs == NULL) {
        return;
    }
    if (gdfs->used == FALSE) {
        return;
    }
    stat = gdfs->stat;
    if (stat == GDD_STAT_READ || stat == GDD_STAT_SEEK ||
        stat == GDD_STAT_BUSY) {
        gdFsStopRd(gdfs);
    }
    gdfs->used = FALSE;
}

Bool gdFsGetFileSize(GDFS gdfs, Sint32 *fsize)
{
    if (gdfs == NULL) {
        return FALSE;
    }
    if (gdfs->used == FALSE) {
        return FALSE;
    }
    *fsize = gdfs->fsize;
    return TRUE;
}

Bool gdFsGetFileSctSize(GDFS gdfs, Sint32 *fsctsize)
{
    if (gdfs == NULL) {
        return FALSE;
    }
    if (gdfs->used == FALSE) {
        return FALSE;
    }
    *fsctsize = gdfs->fsctsize;
    return TRUE;
}

Bool gdFsGetFileFad(GDFS gdfs, Sint32 *fad)
{
    if (gdfs == NULL) {
        return FALSE;
    }
    if (gdfs->used == FALSE) {
        return FALSE;
    }
    *fad = gdfs->fad;
    return TRUE;
}

Sint32 gdFsSeek(GDFS gdfs, Sint32 sctno, Sint32 type)
{
    Sint32 fsctsize;
    Sint32 ofs;

    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    fsctsize = gdfs->fsctsize;
    switch (type) {
    case GDD_SEEK_SET:
        ofs = sctno;
        break;
    case GDD_SEEK_CUR:
        ofs = gdfs->ofs + sctno;
        break;
    case GDD_SEEK_END:
        ofs = sctno + fsctsize;
        break;
    default:
        return GDD_ERR_SEEK;
    }
    if (ofs < 0 || ofs > fsctsize) {
        return GDD_ERR_OFS;
    }
    gdfs->ofs = ofs;
    return GDD_ERR_OK;
}

Sint32 gdFsTell(GDFS gdfs)
{
    if (gdfs == NULL) {
        return 0;
    }
    if (gdfs->used == FALSE) {
        return 0;
    }
    return gdfs->ofs;
}

Sint32 gdFsCheckEof(GDFS gdfs, Bool *iseof)
{
    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->ofs >= gdfs->fsctsize) {
        *iseof = TRUE;
    } else {
        *iseof = FALSE;
    }
    return GDD_ERR_OK;
}

Sint32 gdFsSetTmode(GDFS gdfs, Sint32 tmode)
{
    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    gdfs->tmode = tmode;
    return GDD_ERR_OK;
}

Sint32 gdFsRead(GDFS gdfs, Sint32 nsct, void *buf)
{
    Bool align32;
    Sint32 tries;
    Sint32 result;
    Sint32 delay;

    /* @hack spelling: the alignment test goes through align32, which SHC
     * keeps in R9 and compares with 1 on each pass (CMP/EQ #1,R0 at
     * 0x8c61a354). tested in place, the address is tested by TST, the
     * function is 12 bytes shorter, and 61 of 71 words differ */
    if (((Uint32)buf & 31) == 0) {
        align32 = TRUE;
    } else {
        align32 = FALSE;
    }
    tries = 0;
    while (tries < READ_TRIES) {
        if (align32 == TRUE) {
            result = gdFsReqRd32(gdfs, nsct, buf);
        } else {
            result = request_cpu_read(gdfs, nsct, buf);
        }
        if (result != 0) {
            break;
        }
        if (nsct == 0) {
            break;
        }
        if (gdfs->ofs >= gdfs->fsctsize) {
            break;
        }
        for (delay = READ_DELAY; delay != 0; delay--) {
        }
        tries++;
    }
    if (tries == READ_TRIES) {
        return GDD_ERR_TOUT;
    }
    if (result >= 0) {
        result = gdFsWaitCompleteReq(gdfs);
    }
    return result;
}

/* start a read by the CPU of nsct sectors from the handle's next sector into
 * buf, up to the file's end. it returns the sectors requested, or
 * GDD_ERR_ILLHNDL for a closed handle. it returns 0 while another request
 * runs. it also returns 0 when nothing is left to read or the drive refuses
 * the read, and marks the handle's request complete */
static Sint32 request_cpu_read(GDFS gdfs, Sint32 nsct, void *buf)
{
    Sint32 fad;
    Sint32 end;
    Sint32 gdchn;
    Sint32 mask;

    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->wk->now_work != NULL) {
        return 0;
    }
    fad = gdfs->fad + gdfs->ofs;
    end = gdfs->fad + gdfs->fsctsize;
    if (fad >= end) {
        gdfs->trnsed = 0;
        gdfs->stat = GDD_STAT_COMPLETE;
        return 0;
    }
    if (fad + nsct > end) {
        nsct = end - fad;
    }
    if (nsct == 0) {
        gdfs->trnsed = 0;
        gdfs->stat = GDD_STAT_COMPLETE;
        return 0;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gdfs->wk->dcf->read(fad, nsct, buf, GDD_FS_TMODE_CPU);
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->rsize = nsct;
        gdfs->stat = GDD_STAT_READ;
        gdfs->trflag = GDD_FS_TRANS_BUSY;
        gdfs->act = GDFS_ACT_READ;
        gdfs->trnsed = 0;
        gdfs->wk->now_work = gdfs;
        gdfs->wk->dcf->exec_server();
    } else {
        gdfs->stat = GDD_STAT_COMPLETE;
    }
    set_imask(mask);
    if (gdchn != 0) {
        return nsct;
    }
    return 0;
}

Sint32 gdFsReqRd32(GDFS gdfs, Sint32 nsct, void *buf)
{
    Sint32 fad;
    Sint32 end;
    Sint32 gdchn;
    Sint32 mask;

    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->wk->now_work != NULL) {
        return 0;
    }
    if (((Uint32)buf & 31) != 0) {
        return GDD_ERR_32ALIGN;
    }
    fad = gdfs->fad + gdfs->ofs;
    end = gdfs->fad + gdfs->fsctsize;
    if (fad >= end) {
        gdfs->trnsed = 0;
        gdfs->stat = GDD_STAT_COMPLETE;
        return 0;
    }
    if (fad + nsct > end) {
        nsct = end - fad;
    }
    if (nsct == 0) {
        gdfs->trnsed = 0;
        gdfs->stat = GDD_STAT_COMPLETE;
        return nsct;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gdfs->wk->dcf->read(fad, nsct, buf, gdfs->tmode);
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->rsize = nsct;
        gdfs->stat = GDD_STAT_READ;
        gdfs->trflag = GDD_FS_TRANS_BUSY;
        gdfs->act = GDFS_ACT_READ;
        gdfs->trnsed = 0;
        gdfs->wk->now_work = gdfs;
        gdfs->wk->dcf->exec_server();
    } else {
        gdfs->stat = GDD_STAT_COMPLETE;
    }
    set_imask(mask);
    if (gdchn != 0) {
        return nsct;
    }
    return 0;
}

Sint32 gdFsReqGdRd(GDFS gdfs, Sint32 nsct)
{
    Sint32 fad;
    Sint32 end;
    Sint32 gdchn;
    Sint32 mask;

    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->wk->now_work != NULL) {
        return 0;
    }
    fad = gdfs->fad + gdfs->ofs;
    end = gdfs->fad + gdfs->fsctsize;
    if (fad >= end) {
        gdfs->trnsed = 0;
        gdfs->stat = GDD_STAT_COMPLETE;
        return 0;
    }
    if (fad + nsct > end) {
        nsct = end - fad;
    }
    if (nsct == 0) {
        gdfs->trnsed = 0;
        gdfs->stat = GDD_STAT_COMPLETE;
        return nsct;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gdfs->wk->dcf->req_gd_read(fad, nsct);
    if (gdchn != 0) {
        gdfs->wk->now_work = gdfs;
        gdfs->gdchn = gdchn;
        gdfs->rsize = nsct;
        gdfs->stat = GDD_STAT_READ;
        gdfs->trflag = GDD_FS_TRANS_BUSY;
        gdfs->act = GDFS_ACT_GDREAD;
        gdfs->trsize = 0;
        gdfs->trnsed = 0;
        gdfs->wk->dcf->exec_server();
    } else {
        gdfs->stat = GDD_STAT_COMPLETE;
    }
    set_imask(mask);
    if (gdchn != 0) {
        return nsct;
    }
    return 0;
}

Sint32 gdFsTrans32(GDFS gdfs, Sint32 nbytes, void *buf)
{
    Sint32 result;
    Sint32 mask;

    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    if (((Uint32)buf & 31) != 0) {
        return GDD_ERR_32ALIGN;
    }
    if ((nbytes & 31) != 0) {
        return GDD_ERR_SIZE;
    }
    if (gdfs->act != GDFS_ACT_GDREAD) {
        return GDD_ERR_NOTREAD;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    result = gdfs->wk->dcf->trans(gdfs->gdchn, buf, nbytes);
    if (result == 0) {
        gdfs->trflag = GDD_FS_TRANS_BUSY;
        gdfs->trsize = nbytes;
        gdfs->trnsed = 0;
        gdfs->wk->dcf->exec_server();
        /* @hack redundant: result, already GDD_ERR_OK, is set to it again
         * (MOV #0,R4 at 0x8c61a6d4), which keeps result in R4. without it,
         * SHC saves R12 as well and keeps result in R13, the function is 4
         * bytes longer, and 83 of 86 words differ */
        result = GDD_ERR_OK;
    } else if (result == -1) {
        result = GDD_ERR_SIZEOVER;
    } else {
        result = GDD_ERR_BUSY;
    }
    set_imask(mask);
    return result;
}

Sint32 gdFsGetTransStat(GDFS gdfs)
{
    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    return gdfs->trflag;
}

Sint32 gdFsMovePickup(GDFS gdfs)
{
    Sint32 fad;
    Sint32 gdchn;
    Sint32 mask;

    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->wk->now_work != NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    fad = gdfs->fad + gdfs->ofs;
    gdchn = gdfs->wk->dcf->seek(fad);
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_SEEK;
        gdfs->act = GDFS_ACT_SEEK;
        gdfs->wk->now_work = gdfs;
        gdfs->wk->dcf->exec_server();
    }
    set_imask(mask);
    if (gdchn != 0) {
        /* @hack spelling: the wait's result goes through gdchn, which makes
         * the call a BSR (0x8c61a798). returned straight, SHC restores the
         * registers and branches to it, the function is 4 bytes longer, and
         * 15 of 73 words differ */
        gdchn = gdFsWaitCompleteReq(gdfs);
        return gdchn;
    }
    return GDD_ERR_BUSY;
}

Sint32 gdFsGetStat(GDFS gdfs)
{
    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    return gdfs->stat;
}

Sint32 gdFsGetNumRd(GDFS gdfs)
{
    if (gdfs == NULL) {
        return 0;
    }
    if (gdfs->used == FALSE) {
        return 0;
    }
    return gdfs->trnsed;
}

Sint32 gdFsStopRd(GDFS gdfs)
{
    Sint32 result;
    Sint32 mask;
    Uint32 start;
    Uint32 elapsed;

    if (gdfs == NULL) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->used == FALSE) {
        return GDD_ERR_ILLHNDL;
    }
    if (gdfs->stat != GDD_STAT_READ && gdfs->stat != GDD_STAT_BUSY) {
        gdfs->stat = GDD_STAT_IDLE;
        return GDD_ERR_OK;
    }
    if (gdfs->wk->now_work != gdfs) {
        return GDD_ERR_BUSY;
    }
    start = syTmrGetCount();
    while (gdfs->gdchn_wait != 1) {
        elapsed = syTmrDiffCount(start, syTmrGetCount());
        if (elapsed > STOP_TOUT) {
            return GDD_ERR_TOUT;
        }
    }
    start = syTmrGetCount();
    do {
        elapsed = syTmrDiffCount(start, syTmrGetCount());
    } while (elapsed < STOP_DELAY);
    mask = get_imask();
    set_imask(IMASK_ALL);
    /* the vertical blank handler runs the server, which can end the request
     * during the waits. so the tests of the state and now_work run again, with
     * interrupts masked */
    /* @hack spelling: the checks run in a loop of one pass, whose breaks
     * share one set_imask. SHC loads GDD_ERR_BUSY and GDFS_ACT_STOP into R5
     * and R11 before the tests (0x8c61a87c, 0x8c61a87e). as an if-else
     * chain, it loads each where it is used, and 81 of 136 words differ */
    do {
        if (gdfs->stat != GDD_STAT_READ && gdfs->stat != GDD_STAT_BUSY) {
            gdfs->stat = GDD_STAT_IDLE;
            result = GDD_ERR_OK;
            break;
        }
        if (gdfs->wk->now_work != gdfs) {
            result = GDD_ERR_BUSY;
            break;
        }
        gdfs->wk->dcf->read_abort(gdfs->gdchn);
        /* unlike a read's, a stop's completion adds no sectors to ofs and
         * calls no read end callback */
        gdfs->act = GDFS_ACT_STOP;
        set_imask(mask);
        /* @hack spelling: the wait's result goes through result, which makes
         * the call a BSR (0x8c61a8c0). returned straight, SHC restores the
         * registers and branches to it, the function is 12 bytes longer, and
         * 33 of 136 words differ */
        result = gdFsWaitCompleteReq(gdfs);
        return result;
    } while (FALSE);
    set_imask(mask);
    return result;
}

Sint32 gdFsGetErrStat(GDFS gdfs)
{
    if (gdfs == NULL) {
        return 0;
    }
    if (gdfs->used == FALSE) {
        return 0;
    }
    return gdfs->err;
}

void gdFsEntryRdEndFunc(GDFS gdfs, GDFS_FUNC func, void *obj)
{
    Sint32 mask;

    if (gdfs == NULL) {
        return;
    }
    if (gdfs->used == FALSE) {
        return;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdfs->rdendcb = func;
    gdfs->rdcb_1st = obj;
    set_imask(mask);
}

void gdFsEntryTrEndFunc(GDFS gdfs, GDFS_FUNC func, void *obj)
{
    Sint32 mask;

    if (gdfs == NULL) {
        return;
    }
    if (gdfs->used == FALSE) {
        return;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdfs->trendcb = func;
    gdfs->trcb_1st = obj;
    set_imask(mask);
}

void gdFsEntryErrFunc(GDFS gdfs, GDFS_ERRFUNC erfunc, void *obj)
{
    Sint32 mask;

    if (gdfs == NULL) {
        return;
    }
    if (gdfs->used == FALSE) {
        return;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdfs->errcb = erfunc;
    gdfs->errcb_1st = obj;
    set_imask(mask);
}
