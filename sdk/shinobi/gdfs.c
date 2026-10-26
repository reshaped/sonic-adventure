/* GD file system initialization, server, directories and file handles, with
 * CD-DA playback and the tray state */
#include <machine.h>
#include <sg_chain.h>
#include <sg_ov.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "shinobi/gdfs.h"

/* a sector's frame address is its ISO 9660 sector number plus 150. the
 * GD-ROM's high-density area starts at frame address 45150, where track 3
 * starts */
#define FAD_OFS 150
#define GD_START_FAD 45150
#define GD_START_TRACK 3

/* the primary volume descriptor's sector, counted from the volume's first,
 * and byte offsets in it. PVD_ID starts the standard identifier, PVD_ID_SIZE
 * bytes long. each other offset holds a little-endian 32-bit value */
#define PVD_SECTOR 16
#define PVD_ID 1
#define PVD_ID_SIZE 5
#define PVD_PATHTBL_SIZE 0x84
#define PVD_PATHTBL_SECTOR 0x8c
#define PVD_ROOT_SECTOR 0x9e
#define PVD_ROOT_SIZE 0xa6
/* the bytes of sctbuf ahead of the path table search's sector, where the
 * search joins a record split across two sectors */
#define PATHTBL_ROOM 64

/* the extended error codes that qualify GDD_ERR_NOTREADY: no disc, and a
 * drive still busy with its check */
#define SENSE_NODISC 58
#define SENSE_CHECKBUSY 0x104

/* the offset of the version's digits in get_ver's text, as in 1.10 */
#define VER_TEXT 12
/* drive version 1.01, packed as gdc_ver is. the CD-DA calls keep a play, and
 * request the head position, only from this version on */
#define GDC_VER_POS 0x10001

/* get_scd's request types, and the bytes each returns: a frame's whole
 * subcode, or its Q channel alone */
#define SCD_ALL 0
#define SCD_ALL_SIZE 100
#define SCD_Q 1
#define SCD_Q_SIZE 14
/* the absolute time in frames, below disc_frame's control byte */
#define DISC_TIME_MASK 0xffffff

/* what a kept play's da_first and da_last are, in da_mode: tracks, or frame
 * addresses */
#define DA_BY_TRACK 1
#define DA_BY_SECTOR 2

#define VBLANK_PRI 80
#define CDDMAE_PRI 16

/* the library's version and revision, as its handle gives them */
#define VERSION_NO 1
#define REVISION_NO 5

const char *const gdgFsBuild = "\ngdFs Ver 1.05 Build:Dec 02 1998 19:06:55\n";

const struct gdfs_lif gdg_fs_lif_tbl = {
    NULL,
    NULL,
    NULL,
    gdFsInitG,
    gdFsReinitG,
    gdFsFinishG,
    gdFsSetGlobalErrG,
    gdFsReadWG,
    gdFsLoadDirG,
    gdFsLoadDirFastG,
    gdFsSetDirG,
    gdFsGetDirInfoG,
    gdFsGetWorkHnG,
    gdFsGetSysHnG,
    gdFsCreateHnG,
    gdFsOpenG,
    gdFsOpenFastG,
    gdFsOpenRangeG,
    gdFsExecServerG,
    gdFsGetGlobalErrG,
    gdFsGetTocG,
    gdFsGetDrvStatG,
    gdFsReqDrvStatG,
    gdFsDaPlayG,
    gdFsDaPlaySctG,
    gdFsDaStopG,
    gdFsDaPauseG,
    gdFsDaReleaseG,
    gdFsDaGetInfoG,
    gdFsEntryErrFuncAllG,
    gdFsIsTrayOpenG,
};

GDFS_WORK *gd_wk = NULL;

const Sint32 gdg_fs_pvd = GD_START_FAD + PVD_SECTOR;
const Sint32 gdg_fs_def_tmode = GDD_FS_TMODE_DMA;

static const Uint8 iso_vd_id[] = "CD001";

/* the handlers' numbers from syChainAddHandler, 0 for none */
static SYCHAIN gdfs_vblank_chain = 0;
static SYCHAIN gdfs_cddmae_chain = 0;

/* the library's handle. #pragma section LH places it in section DSGLH, whose
 * handles make up the program's module, an OVS_MDL_OBJ. at startup, strt2.c's
 * _INITLIBH enters that module's libraries in the overlay library table */
#pragma section LH
OVS_LIB_OBJ gdg_fs_libhdl = {(struct _ov_lif *)&gdg_fs_lif_tbl,
                             VERSION_NO,
                             REVISION_NO,
                             0,
                             0,
                             0,
                             GDFS_LID};
#pragma section

volatile Uint32 gdg_fs_vcnt;
/* the drive's version text, as get_ver writes it */
static char gdc_ver_str[32];
/* the drive state, track and index that the last head position request
 * wrote. nothing reads them */
static Sint32 da_stat;
static Sint32 da_track;
static Sint32 da_index;
/* the kept CD-DA play, which gdFsDaReleaseG starts again. da_mode says
 * whether da_first and da_last are tracks or frame addresses */
static Sint32 da_mode;
static Sint32 da_first;
static Sint32 da_last;
static Sint32 da_repeats;
/* the head's frame address once the kept play started, and once it was
 * paused */
static Sint32 da_play_fad;
static Sint32 da_stop_fad;

Sint32 gdFsGetLE4(const Uint8 *p)
{
    /* @hack spelling: the word goes through val. SHC builds it in R3 (OR
     * R0,R3 at 0x8c618eda), and returns it through R4 (0x8c618eec). returned
     * straight, it is built in R0, and 10 of 19 words differ */
    Sint32 val;

    val = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    return val;
}

static Bool is_iso_vd(const Uint8 *sct)
{
    const Uint8 *id = iso_vd_id;
    const Uint8 *p = sct + PVD_ID;
    Sint32 i;

    for (i = 0; i < PVD_ID_SIZE; i++) {
        if (*id++ != *p++) {
            return FALSE;
        }
    }
    return TRUE;
}

Sint32 gdFsSetGlobalErrG(Sint32 err)
{
    gd_wk->errstat = err;
    return 1;
}

Sint32 gdFsGetStatG(void)
{
    GDFS gdfs;
    Sint32 stat;
    Sint32 act;
    Sint32 info[3];
    struct subcode_position pos;
    Sint32 size;
    Sint32 result;
    Sint32 gdchn;

    gd_wk->f_svr = TRUE;
    gdfs = gd_wk->now_work;
    /* @hack spelling: the request number goes through gdchn. SHC selects it
     * (TST R14,R14 at 0x8c618f36) before it loads the call's address. passed
     * in place, it follows the load, and 238 of 306 words differ */
    gdchn = gdfs == NULL ? 0 : gdfs->gdchn;
    stat = gd_wk->dcf->get_cmd_stat(gdchn, info);
    if (gdfs != NULL) {
        switch (stat) {
        case GDD_STAT_IDLE:
        case GDD_STAT_COMPLETE:
            act = gdfs->act;
            gd_wk->now_work = NULL;
            gdfs->stat = stat;
            gdfs->gdchn = 0;
            gdfs->act = GDFS_ACT_NONE;
            if (stat == GDD_STAT_COMPLETE) {
                if (act == GDFS_ACT_READ) {
                    gdfs->trnsed = info[REQ_TRNSED];
                    gdfs->ofs += gdfs->rsize;
                    gdfs->trflag = GDD_FS_TRANS_COMPLETE;
                    if (gdfs->rdendcb != NULL) {
                        gdfs->rdendcb(gdfs->rdcb_1st);
                    }
                } else if (act == GDFS_ACT_STOP) {
                    gdfs->trflag = GDD_FS_TRANS_COMPLETE;
                } else if (act == GDFS_ACT_GDREAD) {
                    gdfs->trnsed = gdfs->trsize;
                    gdfs->ofs += gdfs->rsize;
                    gdfs->trflag = GDD_FS_TRANS_COMPLETE;
                } else if (act == GDFS_ACT_SUBCODE) {
                    /* @hack spelling: gdDecSubcode's result goes through
                     * result. SHC copies it to R4 to test it (MOV R0,R4 at
                     * 0x8c618ffc). tested in place, it is tested in R0, and
                     * 205 of 306 words differ */
                    result = gdDecSubcode((Sint8 *)gd_wk->sctbuf, &pos);
                    if (result != 0) {
                        gdfs->stat = GDD_STAT_ERR;
                        gdfs->err = GDD_ERR_READ;
                        gdfs->ex_errcode = 0;
                        goto error;
                    } else {
                        /* fad holds the address of the GDFS_DAINFO
                         * gdFsDaGetInfoG was given, as an integer */
                        ((GDFS_DAINFO *)gdfs->fad)->track = pos.track;
                        ((GDFS_DAINFO *)gdfs->fad)->min = pos.minute;
                        ((GDFS_DAINFO *)gdfs->fad)->sec = pos.second;
                        ((GDFS_DAINFO *)gdfs->fad)->frame = pos.frame;
                        ((GDFS_DAINFO *)gdfs->fad)->fad =
                            pos.disc_frame & DISC_TIME_MASK;
                    }
                } else if (act == GDFS_ACT_DAPOS) {
                    /* request the head position. its frame address goes
                     * where fid points: to da_play_fad or da_stop_fad */
                    stat = gd_wk->dcf->get_pos(&da_stat, &da_track,
                                               (Sint32 *)gdfs->fid, &da_index);
                    if (stat != 0) {
                        gdfs->gdchn = stat;
                        gdfs->stat = GDD_STAT_BUSY;
                        gdfs->act = GDFS_ACT_CMD;
                        gd_wk->now_work = gdfs;
                    } else {
                        gdfs->stat = GDD_STAT_ERR;
                        gdfs->err = GDD_ERR_RECOVER;
                        gdfs->ex_errcode = 0;
                        *(Sint32 *)gdfs->fid = 0;
                        goto error;
                    }
                }
            }
            break;
        case GDD_STAT_CONTINUE:
            result = gd_wk->dcf->check_trans(gdfs->gdchn, &size);
            if (result == 0) {
                if (size > 0) {
                    gdfs->trflag = GDD_FS_TRANS_READY;
                } else {
                    gdfs->trflag = GDD_FS_TRANS_COMPLETE;
                }
                gdfs->trnsed = gdfs->trsize;
            } else if (result == 1) {
                gdfs->trnsed = size;
            }
            break;
        case GDD_STAT_BUSY:
            gdfs->gdchn_wait = info[REQ_WAIT];
            if (gdfs->act == GDFS_ACT_READ) {
                gdfs->trnsed = info[REQ_TRNSED];
            }
            break;
        case GDD_STAT_ERR:
        case GDD_STAT_FATAL:
            gdfs->stat = stat;
            gdfs->err = info[REQ_ERR];
            gdfs->ex_errcode = info[REQ_EX_ERRCODE];
            gdfs->gdchn = 0;
            gd_wk->now_work = NULL;
            if (gdfs->err == GDD_ERR_NOTREADY) {
                if (gdfs->ex_errcode == SENSE_NODISC) {
                    result = gdFsGetDrvStat();
                    if (result == GDD_DRVSTAT_OPEN ||
                        result == GDD_DRVSTAT_BUSY) {
                        gdfs->err = GDD_ERR_TRAYOPEND;
                    }
                } else if (gdfs->ex_errcode == SENSE_CHECKBUSY) {
                    gdfs->err = GDD_ERR_CHECKBUSY;
                }
            }
        error:
            if (gdfs->err == GDD_ERR_TRAYOPEND ||
                gdfs->err == GDD_ERR_UNITATTENT) {
                gd_wk->istray = TRUE;
            }
            if (gd_wk->g_errcb != NULL) {
                gd_wk->g_errcb(gd_wk->g_errcb_1st, gdfs->err);
            }
            if (gdfs->errcb != NULL) {
                gdfs->errcb(gdfs->errcb_1st, gdfs->err);
            }
            break;
        default:
            /* any other state ends the request, and stat keeps only its
             * low byte */
            gdfs->stat = (Uint8)stat;
            gdfs->gdchn = 0;
            gd_wk->now_work = NULL;
            break;
        }
    }
    gd_wk->f_svr = FALSE;
    return stat;
}

Sint32 gdFsReadWG(Sint32 fad, Sint32 nsct, void *buf)
{
    GDFS gdfs;
    Sint32 result;

    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, fad, nsct * GDD_FS_SCTSIZE);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    /* @hack spelling: the read's result goes through result, which makes the
     * call a JSR (0x8c6191d2). returned straight, SHC jumps to gdFsRead, and 8
     * of 31 words differ */
    result = gdFsRead(gdfs, nsct, buf);
    return result;
}

/* the GD-ROM DMA end interrupt's handler. it ends the DMA with now_work's
 * transfer end callback, or with none */
static void gdfs_cddmae_handler(void *obj)
{
    GDFS gdfs = gd_wk->now_work;
    GDFS_FUNC func = NULL;
    void *arg = NULL;

    if (gdfs != NULL && gdfs->trendcb != NULL) {
        func = gdfs->trendcb;
        arg = gdfs->trcb_1st;
    }
    gdGdcG1DmaEnd(func, arg);
}

/* the vertical blank interrupt's handler. it counts the blank, and runs the
 * server unless gdFsGetStatG is running */
static void gdfs_vblank_handler(void *obj)
{
    gdg_fs_vcnt++;
    if (gd_wk->f_svr != TRUE) {
        gdFsExecServerG();
    }
}

/* start the drive, with interrupts masked. add the handlers to their chains,
 * and wait for the drive to start. then read its version, the primary volume
 * descriptor and the root directory into curdir. it returns GDD_ERR_OK with
 * f_init set, or the GDD_ERR_ code for the step that fails */
static Sint32 gdfs_init_drive(void)
{
    Sint32 mask;
    GDFS gdfs;
    Sint32 gdchn;
    Sint32 err;
    Sint32 stat;
    Sint32 fad;
    Sint32 size;
    Sint32 info[2];

    /* @hack spelling: a loop of one pass, whose breaks share the restore of
     * the mask below it. with gotos to that restore in its place, SHC lays the
     * function out 48 bytes shorter, and 272 of 278 words differ */
    do {
        mask = get_imask();
        set_imask(IMASK_ALL);
        gdg_fs_vcnt = 0;
        if (gdfs_vblank_chain == 0) {
            gdfs_vblank_chain =
                syChainAddHandler(SYD_CHAIN_EVENT_IML6_VBLANK,
                                  gdfs_vblank_handler, VBLANK_PRI, NULL);
        }
        if (gdfs_cddmae_chain == 0) {
            gdfs_cddmae_chain =
                syChainAddHandler(SYD_CHAIN_EVENT_IML4_CDDMAE,
                                  gdfs_cddmae_handler, CDDMAE_PRI, NULL);
        }
        gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
        if (gdfs == NULL) {
            err = GDD_ERR_RESET;
            break;
        }
        gdchn = gd_wk->dcf->init_drive();
        if (gdchn == 0) {
            err = GDD_ERR_RESET;
            break;
        }
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_BUSY;
        gdfs->wk->now_work = gdfs;
        stat = gdFsWaitCompleteReq(gdfs);
        if (gd_wk->dcf->get_drv_stat(info) == FALSE) {
            err = GDD_ERR_RESET;
            break;
        }
        if (stat != GDD_ERR_OK) {
            /* @hack spelling: the state is read as *info, which keeps the frame
             * at 24 bytes (ADD #-24,R15 at 0x8c61923c). read as
             * info[DRV_STAT], the frame is 28 bytes, and 262 of 278 words
             * differ */
            if (stat == GDD_ERR_NOTREADY && *info == GDD_DRVSTAT_OPEN) {
                err = GDD_ERR_TRAYOPEND;
            } else {
                err = stat;
            }
            break;
        }
        if (info[DRV_DISC] != DISC_GDROM) {
            err = GDD_ERR_DISC;
            break;
        }
        gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
        if (gdfs == NULL) {
            err = GDD_ERR_RESET;
            break;
        }
        /* the drive version, from get_ver's text. it stays 0 unless the drive
         * gives it */
        gd_wk->gdc_ver = 0;
        gdchn = gd_wk->dcf->get_ver(gdc_ver_str);
        if (gdchn != 0) {
            gdfs->gdchn = gdchn;
            gdfs->stat = GDD_STAT_BUSY;
            gdfs->wk->now_work = gdfs;
            stat = gdFsWaitCompleteReq(gdfs);
            if (stat == GDD_ERR_OK) {
                gd_wk->gdc_ver |= (gdc_ver_str[VER_TEXT] - '0') << 16;
                gd_wk->gdc_ver |= (gdc_ver_str[VER_TEXT + 2] - '0') << 8;
                gd_wk->gdc_ver |= gdc_ver_str[VER_TEXT + 3] - '0';
            }
        }
        err = gdFsReadW(gdg_fs_pvd, 1, gd_wk->sctbuf);
        if (err != GDD_ERR_OK) {
            err = GDD_ERR_MOUNT;
            break;
        }
        if (!is_iso_vd((Uint8 *)gd_wk->sctbuf)) {
            err = GDD_ERR_DISC;
            break;
        }
        /* @hack spelling: the path table's sector number, then its frame
         * address, in two steps. they have SHC load syChainAddHandler's
         * address into R12 and gdfs_vblank_chain's into R14 (0x8c619278,
         * 0x8c619276). in one step, the two trade, and 105 of 278 words
         * differ */
        gd_wk->pathtbl_fad =
            gdFsGetLE4((Uint8 *)gd_wk->sctbuf + PVD_PATHTBL_SECTOR);
        gd_wk->pathtbl_fad += FAD_OFS;
        gd_wk->pathtbl_size =
            gdFsGetLE4((Uint8 *)gd_wk->sctbuf + PVD_PATHTBL_SIZE);
        /* @hack spelling: the root's sector number, then its frame address, in
         * two steps. they have SHC keep info's address in R10 and
         * gdc_ver_str's in R9 (0x8c619240, 0x8c61924a). in one step, the two
         * trade, and 76 of 278 words differ */
        fad = gdFsGetLE4((Uint8 *)gd_wk->sctbuf + PVD_ROOT_SECTOR);
        fad += FAD_OFS;
        size = gdFsGetLE4((Uint8 *)gd_wk->sctbuf + PVD_ROOT_SIZE);
        gd_wk->curdir->dir_fad = 0;
        stat = gdFsGetDirEnt((Uint8 *)gd_wk->sctbuf, fad, gd_wk->curdir, size);
        if (stat == GDD_ERR_DIROVER) {
            err = stat;
            break;
        } else if (stat != GDD_ERR_OK) {
            err = GDD_ERR_MOUNT;
            break;
        }
        if (gd_wk->curdir->dir_num == 0) {
            err = GDD_ERR_DISC;
            break;
        }
        gd_wk->f_init = TRUE;
        err = GDD_ERR_OK;
    } while (FALSE);
    set_imask(mask);
    return err;
}

Sint32 gdFsInit(Sint32 max_open, void *gdfs_work, Sint32 max_dirent,
                void *dirbuf)
{
    return gdFsInitOvl(max_open, gdfs_work, max_dirent, dirbuf);
}

Sint32 gdFsInitG(Sint32 max_open, void *gdfs_work, Sint32 max_dirent,
                 void *dirbuf)
{
    const GD_DCF *dcf;
    Sint32 mask;
    GDFS gdfs;
    Sint32 i;

    if (gd_wk != NULL && gd_wk->f_init == TRUE) {
        return GDD_ERR_OK;
    }
    if (((Uint32)gdfs_work & 31) != 0) {
        return GDD_ERR_32ALIGN;
    }
    dcf = gdFsGetGdDcf();
    dcf->init_system();
    mask = get_imask();
    set_imask(IMASK_ALL);
    gd_wk = gdfs_work;
    gd_wk->f_init = FALSE;
    gd_wk->f_svr = FALSE;
    gd_wk->max_open = max_open;
    gd_wk->curdir = gdFsCreateDirhn(dirbuf, max_dirent);
    gd_wk->dcf = dcf;
    gd_wk->now_work = NULL;
    gd_wk->daplayed = FALSE;
    gd_wk->g_errcb = NULL;
    gd_wk->istray = FALSE;
    for (i = 0, gdfs = gd_wk->hndlist; i < gd_wk->max_open + 1; i++, gdfs++) {
        gdfs->used = FALSE;
        gdfs->wk = gd_wk;
    }
    gd_wk->syshdl = &gd_wk->hndlist[0];
    gd_wk->hndtbl = &gd_wk->hndlist[1];
    gdFsCreateHn(gd_wk->syshdl, 0, 0, 0);
    set_imask(mask);
    return gdfs_init_drive();
}

Sint32 gdFsReinitG(void)
{
    Sint32 mask;
    GDFS gdfs;
    Sint32 i;

    if (gd_wk == NULL) {
        return GDD_ERR_INIT;
    }
    for (i = 0, gdfs = gd_wk->hndlist; i < gd_wk->max_open + 1; gdfs++, i++) {
        gdFsClose(gdfs);
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gd_wk->dcf->init_system();
    gd_wk->f_init = FALSE;
    gd_wk->f_svr = FALSE;
    gd_wk->now_work = NULL;
    gd_wk->daplayed = FALSE;
    for (i = 0, gdfs = gd_wk->hndlist; i < gd_wk->max_open + 1; i++, gdfs++) {
        gdfs->used = FALSE;
        gdfs->wk = gd_wk;
    }
    gd_wk->syshdl = &gd_wk->hndlist[0];
    gd_wk->hndtbl = &gd_wk->hndlist[1];
    gdFsCreateHn(gd_wk->syshdl, 0, 0, 0);
    set_imask(mask);
    return gdfs_init_drive();
}

void gdFsFinishG(void)
{
    GDFS gdfs;
    Sint32 i;

    if (gdfs_vblank_chain != 0) {
        syChainDeleteHandler(gdfs_vblank_chain);
    }
    if (gdfs_cddmae_chain != 0) {
        syChainDeleteHandler(gdfs_cddmae_chain);
    }
    gdfs_vblank_chain = 0;
    gdfs_cddmae_chain = 0;
    for (i = 0, gdfs = gd_wk->hndlist; i < gd_wk->max_open + 1; gdfs++, i++) {
        gdFsClose(gdfs);
    }
    gd_wk = NULL;
}

Sint32 gdFsLoadDirG(const char *dirname, GDFS_DIRREC gf_dirrec)
{
    GDFS_DIRREC_ENT *ent;
    Sint32 fad;
    Sint32 size;
    Sint32 no;
    Sint32 result;

    if (gf_dirrec == NULL) {
        gf_dirrec = gd_wk->curdir;
    }
    if (*dirname == '/' || *dirname == '\\') {
        no = gdFsSearchPath((Uint8 *)gd_wk->sctbuf + PATHTBL_ROOM,
                            gd_wk->pathtbl_fad, gd_wk->pathtbl_size, dirname);
        if (no == 0) {
            return GDD_ERR_NOTFOUND;
        }
        fad = no + FAD_OFS;
        size = GDD_FS_SCTSIZE;
    } else {
        no = gdFsSearchName(dirname, gd_wk->curdir, FALSE);
        if (no == 0) {
            return GDD_ERR_NOTFOUND;
        }
        /* @hack spelling: the entry is stepped to from the table's start, which
         * SHC scales by MUL.L (0x8c619776). taken as &dirrec_tbl[no - 1], it
         * is scaled by MULS and the product's low byte sign-extended, and 35
         * of 73 words differ */
        ent = gd_wk->curdir->dirrec_tbl;
        ent += no - 1;
        if ((ent->flag & GDD_FF_DIRECTORY) == 0) {
            return GDD_ERR_NOTDIR;
        }
        fad = ent->sector + FAD_OFS;
        size = ent->fsize;
    }
    /* @hack spelling: the read's result goes through result, which makes the
     * call a JSR (0x8c61979a). returned straight, SHC jumps to gdFsGetDirEnt,
     * and 19 of 73 words differ */
    result = gdFsGetDirEnt((Uint8 *)gd_wk->sctbuf, fad, gf_dirrec, size);
    return result;
}

Sint32 gdFsLoadDirFastG(const char *dirname, GDFS_DIRREC gf_dirrec)
{
    return gdFsLoadDirG(dirname, gf_dirrec);
}

Sint32 gdFsSetDirG(GDFS_DIRREC gf_dirrec)
{
    GDFS_DIRREC curdir = gd_wk->curdir;
    Sint32 max_ent = curdir->max_ent;
    Sint32 dir_num = gf_dirrec->dir_num;
    Sint32 num;
    Sint32 i;
    GDFS_DIRREC_ENT *src;
    GDFS_DIRREC_ENT *dst;

    num = max_ent > dir_num ? dir_num : max_ent;
    curdir->dir_fad = gf_dirrec->dir_fad;
    for (i = 0, src = gf_dirrec->dirrec_tbl, dst = curdir->dirrec_tbl; i < num;
         i++) {
        gdFsMemcpy(src++, dst++, sizeof(GDFS_DIRREC_ENT));
    }
    curdir->dir_num = i;
    if (max_ent < dir_num) {
        return GDD_ERR_DIROVER;
    }
    return GDD_ERR_OK;
}

Sint32 gdFsGetDirInfoG(const char *name, GDFS_DIRINFO *dirinfo)
{
    GDFS_DIRREC_ENT *ent;
    Sint32 no;

    no = gdFsSearchName(name, gd_wk->curdir, FALSE);
    if (no == 0) {
        return GDD_ERR_NOTFOUND;
    }
    /* @hack spelling: the entry is stepped to from the table's start, which SHC
     * scales by MUL.L (0x8c619858). taken as &dirrec_tbl[no - 1], it is
     * scaled by MULS and the product's low byte sign-extended, and 23 of 37
     * words differ */
    ent = gd_wk->curdir->dirrec_tbl;
    ent += no - 1;
    dirinfo->fad = ent->sector + FAD_OFS;
    dirinfo->fsize = ent->fsize;
    dirinfo->flag = ent->flag;
    return GDD_ERR_OK;
}

GDFS gdFsCreateHnG(GDFS gdfs, Sint32 fid, Sint32 fad, Sint32 fsize)
{
    if (gdfs->used != FALSE) {
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
    gdfs->rdendcb = NULL;
    gdfs->trendcb = NULL;
    gdfs->errcb = NULL;
    gdfs->gdchn = 0;
    gdfs->ex_errcode = 0;
    gdfs->act = GDFS_ACT_NONE;
    gdfs->trflag = GDD_FS_TRANS_READY;
    gdfs->used = TRUE;
    gdfs->tmode = gdg_fs_def_tmode;
    gdfs->stat = GDD_STAT_IDLE;
    gdfs->err = GDD_ERR_OK;
    return gdfs;
}

GDFS gdFsOpenG(const char *fname, GDFS_DIRREC gf_dirrec)
{
    GDFS gdfs;
    GDFS_DIRREC_ENT *ent;
    Sint32 no;
    Sint32 i;

    if (gf_dirrec == NULL) {
        gf_dirrec = gd_wk->curdir;
    }
    no = gdFsSearchName(fname, gf_dirrec, FALSE);
    if (no == 0) {
        gdFsSetGlobalErr(GDD_ERR_NOTFOUND);
        return NULL;
    }
    for (i = 0, gdfs = gd_wk->hndtbl; i < gd_wk->max_open; i++, gdfs++) {
        if (gdfs->used == FALSE) {
            /* @hack spelling: the entry is stepped to from the table's start,
             * which SHC scales by MUL.L (0x8c619912). taken as
             * &dirrec_tbl[no - 1], it is scaled by MULS and the product's low
             * byte sign-extended, and 42 of 65 words differ */
            ent = gf_dirrec->dirrec_tbl;
            ent += no - 1;
            gdFsCreateHn(gdfs, no, ent->sector + FAD_OFS, ent->fsize);
            gdFsSetGlobalErr(GDD_ERR_OK);
            return gdfs;
        }
    }
    gdFsSetGlobalErr(GDD_ERR_NOHNDL);
    return NULL;
}

GDFS gdFsOpenFastG(const char *fname, GDFS_DIRREC gf_dirrec)
{
    return gdFsOpenG(fname, gf_dirrec);
}

GDFS gdFsOpenRangeG(Sint32 stsct, Sint32 nsct)
{
    GDFS gdfs;
    Sint32 i;

    if (stsct < GD_START_FAD) {
        gdFsSetGlobalErr(GDD_ERR_ILLREQ);
        return NULL;
    }
    for (i = 0, gdfs = gd_wk->hndtbl; i < gd_wk->max_open; i++, gdfs++) {
        if (gdfs->used == FALSE) {
            gdFsCreateHn(gdfs, 0, stsct, nsct * GDD_FS_SCTSIZE);
            gdFsSetGlobalErr(GDD_ERR_OK);
            return gdfs;
        }
    }
    gdFsSetGlobalErr(GDD_ERR_NOHNDL);
    return NULL;
}

void gdFsExecServerG(void)
{
    gd_wk->dcf->exec_server();
    if (gd_wk->now_work != NULL) {
        gdFsGetStatG();
    }
}

Sint32 gdFsGetGlobalErrG(void)
{
    return gd_wk->errstat;
}

GDFS gdFsGetWorkHnG(void)
{
    return gd_wk->now_work;
}

GDFS gdFsGetSysHnG(void)
{
    return gd_wk->syshdl;
}

Sint32 gdFsGetTocG(Sint32 type, void *buf)
{
    GDFS gdfs;
    Sint32 mask;
    Sint32 gdchn;

    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gd_wk->dcf->get_toc(type, buf);
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_BUSY;
        gdfs->wk->now_work = gdfs;
    }
    set_imask(mask);
    if (gdchn == 0) {
        gdchn = GDD_ERR_BUSY;
    } else {
        gdchn = gdFsWaitCompleteReq(gdfs);
    }
    return gdchn;
}

Sint32 gdFsGetDrvStatG(void)
{
    Sint32 info[2];
    /* @hack spelling: the call's result goes through got. SHC copies it to R4
     * to test it (MOV R0,R4 at 0x8c619ad6). tested in place, it is tested in
     * R0, and 12 of 20 words differ */
    Bool got;

    got = gd_wk->dcf->get_drv_stat(info);
    if (got == FALSE) {
        return GDD_DRVSTAT_CANTREAD;
    }
    return info[DRV_STAT];
}

Sint32 gdFsReqDrvStatG(void)
{
    GDFS gdfs;
    Sint32 mask;
    Sint32 gdchn;

    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gd_wk->dcf->get_scd(SCD_Q, SCD_Q_SIZE, gd_wk->sctbuf);
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_BUSY;
        gdfs->act = GDFS_ACT_DRVSTAT;
        gdfs->wk->now_work = gdfs;
        gd_wk->dcf->exec_server();
    }
    set_imask(mask);
    if (gdchn == 0) {
        return GDD_ERR_BUSY;
    }
    return GDD_ERR_OK;
}

void gdFsEntryErrFuncAllG(GDFS_ERRFUNC erfunc, void *obj)
{
    Sint32 mask;

    mask = get_imask();
    set_imask(IMASK_ALL);
    gd_wk->g_errcb = erfunc;
    gd_wk->g_errcb_1st = obj;
    set_imask(mask);
}

Sint32 gdFsDaPlayG(Sint32 st_track, Sint32 end_track, Sint32 reptime)
{
    GDFS gdfs;
    Sint32 mask;
    Sint32 gdchn;

    if (st_track < GD_START_TRACK) {
        return GDD_ERR_BUSY;
    }
    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gd_wk->dcf->da_play(st_track, end_track, reptime);
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_BUSY;
        if (gd_wk->gdc_ver >= GDC_VER_POS) {
            gd_wk->daplayed = TRUE;
            da_mode = DA_BY_TRACK;
            da_first = st_track;
            da_last = end_track;
            da_repeats = reptime;
            gdfs->fid = (Sint32)&da_play_fad;
            gdfs->act = GDFS_ACT_DAPOS;
        } else {
            gdfs->act = GDFS_ACT_CMD;
        }
        gdfs->wk->now_work = gdfs;
        gd_wk->dcf->exec_server();
    }
    set_imask(mask);
    if (gdchn == 0) {
        return GDD_ERR_BUSY;
    }
    return GDD_ERR_OK;
}

Sint32 gdFsDaPlaySctG(Sint32 st_sct, Sint32 end_sct, Sint32 reptime)
{
    GDFS gdfs;
    Sint32 mask;
    Sint32 gdchn;

    if (st_sct < GD_START_FAD) {
        return GDD_ERR_BUSY;
    }
    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gd_wk->dcf->da_play_sct(st_sct, end_sct, reptime);
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_BUSY;
        if (gd_wk->gdc_ver >= GDC_VER_POS) {
            gd_wk->daplayed = TRUE;
            da_mode = DA_BY_SECTOR;
            da_first = st_sct;
            da_last = end_sct;
            da_repeats = reptime;
            gdfs->fid = (Sint32)&da_play_fad;
            gdfs->act = GDFS_ACT_DAPOS;
        } else {
            gdfs->act = GDFS_ACT_CMD;
        }
        gdfs->wk->now_work = gdfs;
        gd_wk->dcf->exec_server();
    }
    set_imask(mask);
    if (gdchn == 0) {
        return GDD_ERR_BUSY;
    }
    return GDD_ERR_OK;
}

Sint32 gdFsDaStopG(void)
{
    GDFS gdfs;
    Sint32 mask;
    Sint32 gdchn;

    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gd_wk->dcf->da_pause();
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_BUSY;
        if (gd_wk->daplayed != FALSE && gd_wk->gdc_ver >= GDC_VER_POS) {
            gdfs->fid = (Sint32)&da_stop_fad;
            gdfs->act = GDFS_ACT_DAPOS;
        } else {
            gdfs->act = GDFS_ACT_CMD;
        }
        gdfs->wk->now_work = gdfs;
        gd_wk->dcf->exec_server();
    }
    set_imask(mask);
    if (gdchn == 0) {
        return GDD_ERR_BUSY;
    }
    return GDD_ERR_OK;
}

/* the same as gdFsDaStopG */
Sint32 gdFsDaPauseG(void)
{
    GDFS gdfs;
    Sint32 mask;
    Sint32 gdchn;

    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gd_wk->dcf->da_pause();
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_BUSY;
        if (gd_wk->daplayed != FALSE && gd_wk->gdc_ver >= GDC_VER_POS) {
            gdfs->fid = (Sint32)&da_stop_fad;
            gdfs->act = GDFS_ACT_DAPOS;
        } else {
            gdfs->act = GDFS_ACT_CMD;
        }
        gdfs->wk->now_work = gdfs;
        gd_wk->dcf->exec_server();
    }
    set_imask(mask);
    if (gdchn == 0) {
        return GDD_ERR_BUSY;
    }
    return GDD_ERR_OK;
}

Sint32 gdFsDaReleaseG(Sint32 reptime)
{
    GDFS gdfs;
    Sint32 mask;
    Sint32 gdchn;

    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    if (gd_wk->daplayed != FALSE && da_play_fad != da_stop_fad) {
        gd_wk->daplayed = FALSE;
    }
    if (gd_wk->daplayed != FALSE && gd_wk->gdc_ver >= GDC_VER_POS) {
        if (da_mode == DA_BY_TRACK) {
            gdchn = gd_wk->dcf->da_play(da_first, da_last, da_repeats);
        } else {
            gdchn = gd_wk->dcf->da_play_sct(da_first, da_last, da_repeats);
        }
        if (gdchn != 0) {
            gdfs->gdchn = gdchn;
            gdfs->stat = GDD_STAT_BUSY;
            gdfs->fid = (Sint32)&da_play_fad;
            gdfs->act = GDFS_ACT_DAPOS;
            gdfs->wk->now_work = gdfs;
            gd_wk->dcf->exec_server();
        }
    } else {
        gdchn = gd_wk->dcf->da_release(reptime);
        if (gdchn != 0) {
            gdfs->gdchn = gdchn;
            gdfs->stat = GDD_STAT_BUSY;
            gdfs->act = GDFS_ACT_CMD;
            gdfs->wk->now_work = gdfs;
            gd_wk->dcf->exec_server();
        }
    }
    set_imask(mask);
    if (gdchn == 0) {
        return GDD_ERR_BUSY;
    }
    return GDD_ERR_OK;
}

Sint32 gdFsDaGetInfoG(GDFS_DAINFO *dainfo)
{
    GDFS gdfs;
    Sint32 mask;
    Sint32 gdchn;

    gdfs = gdFsReopenHn(gd_wk->syshdl, 0, 0, 0);
    if (gdfs == NULL) {
        return GDD_ERR_BUSY;
    }
    mask = get_imask();
    set_imask(IMASK_ALL);
    gdchn = gd_wk->dcf->get_scd(SCD_ALL, SCD_ALL_SIZE, gd_wk->sctbuf);
    if (gdchn != 0) {
        gdfs->gdchn = gdchn;
        gdfs->stat = GDD_STAT_BUSY;
        gdfs->act = GDFS_ACT_SUBCODE;
        gdfs->fad = (Sint32)dainfo;
        gdfs->wk->now_work = gdfs;
        gd_wk->dcf->exec_server();
    }
    set_imask(mask);
    if (gdchn == 0) {
        return GDD_ERR_BUSY;
    }
    return GDD_ERR_OK;
}

Bool gdFsIsTrayOpenG(void)
{
    Sint32 stat;

    if (gd_wk->istray == TRUE) {
        return TRUE;
    }
    stat = gdFsGetDrvStat();
    if (stat == GDD_DRVSTAT_OPEN) {
        gd_wk->istray = TRUE;
        return TRUE;
    } else if (stat == GDD_DRVSTAT_BUSY) {
        gdFsReqDrvStat();
    }
    return FALSE;
}
