/* the Sofdec player: its handles, its decoding fiber and its servers */
#include <kamui.h>
#include <machine.h>
#include <mw_rna.h>
#include <ninja.h>
#include <sg_mwmng.h>
#include <sg_mwply.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>
#include <sofdec.h>
#include <string.h>

#include "movie/sdk/gsc.h"
#include "movie/sdk/mpv.h"
#include "movie/sdk/mwsfd.h"
#include "movie/sdk/rnv.h"
#include "movie/sdk/sfd.h"
#include "movie/sdk/sjrbf.h"

#define MWSFD_MAX_PLAYERS 32
/* the records in the fiber table that syFbrInit takes */
#define MWSFD_FIBERS 5

/* the times in microseconds that mark a step of the decoding pass slow.
 * Sofdec's server runs slow from SLOW_SFD_SERVER on, and the players' passes
 * from SLOW_PLAYERS on. the GD drive's, file system's and stream servers each
 * run slow past SLOW_SERVER */
#define SLOW_SFD_SERVER 1000
#define SLOW_PLAYERS 50
#define SLOW_SERVER 10

#define MWSFD(mwply) ((struct mwsfd_player *)(mwply))

/* the last code mwSfdSetErrCode recorded, and the function it passes each
 * nonzero code to, with obj */
struct mwsfd_err {
    void (*func)(void *obj, Sint32 code);
    void *obj;
    Sint32 code;
};

struct mwsfd_work {
    Uint8 unk_00[4];
    struct mwsfd_player ply[MWSFD_MAX_PLAYERS];
    struct mwsfd_err err;
};

/* the cache control register, and its bits */
#define CCR (*(volatile Uint32 *)0xff00001cUL)
#define CCR_OCE 0x1 /* operand cache on */
#define CCR_CB 0x4 /* P1 copy-back, not write-through */
/* operand cache index mode: address bit 25 takes bit 13's place in the index */
#define CCR_OIX 0x80
#define CCR_ICE 0x100 /* instruction cache on */
/* the operand cache's address array: a tag for each of its lines. the tags lie
 * a line's 32 bytes apart, which is OPERAND_TAG_STEP Uint32s */
#define OPERAND_TAGS ((volatile Uint32 *)0xf4000000UL)
#define OPERAND_LINES 512
#define OPERAND_TAG_STEP 8
/* address's P2 alias, through which code runs uncached */
#define P2(address) ((Uint32)(address) | 0xa0000000UL)

void mwSfdVsync(void *obj);
void mwSfdExecServer(void *obj);
void mwSfdDestroy(MWPLY mwply);
void mwSfdStartFname(MWPLY mwply, Sint8 *fname);
void mwSfdStop(MWPLY mwply);
MWE_PLY_STAT mwSfdGetStat(MWPLY mwply);
void mwSfdGetTime(MWPLY mwply, Sint32 *ncount, Sint32 *tscale);
void mwSfdPause(MWPLY mwply, Sint32 sw);
void mwSfdSetOutVol(MWPLY mwply, Sint32 vol);
Sint32 mwSfdGetOutVol(MWPLY mwply);
void mwSfdSetOutPan(MWPLY mwply, Sint32 chno, Sint32 pan);
Sint32 mwSfdGetOutPan(MWPLY mwply, Sint32 chno);
void mwlSfdSleepDecSvr(void);

const char mwg_sfd_build[] = "\nmwSfd Ver.1.04 Build:Nov 11 1998 21:22:12\n";

MWD_IF mwsfd_if = {
    0,
    0,
    0,
    /* the vertical blank and server functions mwPlyInitSofdec adds to the
     * manager, which take a void *. neither reads its argument */
    (void (*)(MWPLY))mwSfdVsync,
    (void (*)(MWPLY))mwSfdExecServer,
    mwSfdDestroy,
    mwSfdStartFname,
    mwSfdStop,
    mwSfdGetStat,
    mwSfdGetTime,
    mwSfdPause,
    mwSfdSetOutVol,
    mwSfdGetOutVol,
    mwSfdSetOutPan,
    mwSfdGetOutPan,
};
/* the display latency in frames, and the frame rate in millihertz: 29.97 Hz */
Sint32 mwg_disp_latency = 2;
Sint32 mwg_vsync_mhz = 29970;
/* a frame's period and the display delay, in tenths of a millisecond.
 * mwPlyInitSofdec sets them from the two above */
Sint32 mwg_frm_term = 330;
Sint32 mwg_frm_delay = 660;
/* the vertical blank, counted from mwPlyStartFrame, from which on each blank
 * switches to the main fiber */
Sint32 mwg_nvsync_pic = 2;
Sint32 mwg_ply_chain = 0;
/* 1 while the decoding fiber runs a pass, and 1 while the main fiber asks it
 * to sleep. both are volatile, since the vertical blank interrupt can switch
 * fibers between any two reads */
volatile Sint32 mwg_thread_exec_flag = 0;
volatile Sint32 mwg_thread_sleep_flag = 0;
/* the vertical blanks since mwPlyStartFrame */
Sint32 mwg_field_no = 0;
/* mw_start_time is the timer's time when picture 0 is shown with audio. the
 * three after it are differences of mw_atime_ms2, mw_vtime_ms2 and
 * mw_rtime_ms2 below: audio less picture, real less picture, and real less
 * audio. all are in tenths of a millisecond */
Sint32 mw_start_time = 0;
Sint32 mw_dif_av_time2 = 0;
Sint32 mw_dif_rv_time2 = 0;
Sint32 mw_dif_ra_time2 = 0;
/* the pictures the draw callback has held back, and those it has skipped */
Sint32 mwg_cnt_wait_draw = 0;
Sint32 mwg_cnt_skip_draw = 0;
/* the number of the last picture shown */
static Sint32 mwl_last_dfno = 0;
/* the Sofdec handle of the last player found ended */
struct sfd_handle *mwg_sfd_hn = 0;
/* the decoding pass's slow steps, by number: 1 is Sofdec's server, 2 the
 * players, 3 the file system's server, 4 the stream server and 5 the GD
 * drive's server. a slow step leaves its timer counts at start and end in
 * mw_tNa and mw_tNb, and adds its microseconds to the total after them */
Uint32 mw_t1a = 0;
Uint32 mw_t1b = 0;
Sint32 mw_dectime = 0;
Uint32 mw_t2a = 0;
Uint32 mw_t2b = 0;
Sint32 mw_splttime = 0;
Uint32 mw_t3a = 0;
Uint32 mw_t3b = 0;
Sint32 mw_gdfstime = 0;
Uint32 mw_t4a = 0;
Uint32 mw_t4b = 0;
Sint32 mw_gsctime = 0;
Uint32 mw_t5a = 0;
Uint32 mw_t5b = 0;
Sint32 mw_gdctime = 0;
/* mwl_sfd_wait_vsync's timer counts: at the last wait's entry and this one's,
 * and when the decoding fiber switches back */
Uint32 mw_t6a = 0;
Uint32 mw_t6b = 0;
Uint32 mw_t6c = 0;
/* the timer counts the first wait after a vertical blank keeps. mw_t7a is
 * mwSfdVsync's count at that blank, and mw_t7b the wait's own. mw_t7c holds
 * the blank's count until that wait, and vsync_flag is 1 until then */
Uint32 mw_t7a = 0;
Uint32 mw_t7b = 0;
Uint32 mw_t7c = 0;
Sint32 vsync_flag = 0;
/* the number of mwSfdExecServer calls */
Sint32 mwg_sfd_exec_svr_cnt = 0;

struct mwsfd_work mwg_ply_work;
/* mwGscInit's work for one stream, with GSC_WORK_ALIGN bytes of room to
 * round its start up */
static char mwsfd_gsc_work[sizeof(struct gsc_stream) + GSC_WORK_ALIGN];
/* the fiber records, the decoding fiber and the main fiber, and the decoding
 * fiber's stack */
struct sy_fiber g_fbrWork[MWSFD_FIBERS];
struct sy_fiber *g_fbrSub;
struct sy_fiber *g_fbrMain;
char g_subStack[0x4000];
/* at the last picture shown: the audio's time, the picture's, and the real
 * time since mw_start_time, in tenths of a millisecond */
Sint32 mw_atime_ms2;
Sint32 mw_vtime_ms2;
Sint32 mw_rtime_ms2;

/* mask every interrupt. returns the old mask, for mwPlyUnlock to restore */
Sint32 mwPlyLock(void)
{
    Sint32 imask;

    imask = get_imask();
    set_imask(IMASK_ALL);
    return imask;
}

void mwPlyUnlock(Sint32 imask)
{
    set_imask(imask);
}

MWPLY mwPlyCreateSofdec(MWS_PLY_CPRM_SFD *cprm)
{
    Sint32 i;
    struct mwsfd_player *ply;
    Sint32 error;

    for (i = 0; i < MWSFD_MAX_PLAYERS; i++) {
        ply = &mwg_ply_work.ply[i];
        if (!ply->used) {
            break;
        }
    }
    if (i == MWSFD_MAX_PLAYERS) {
        mwSfdSetErrCode(MWSFD_ERR_NO_HANDLE);
        mwMngError("W2000 mwPlyCreateSofdec:can't create handle");
        return NULL;
    }
    error = mwPlySfdInit(ply, cprm->ftype);
    if (error != 0) {
        return NULL;
    }
    ply->sfd =
        mwPlySfdCreate(ply, cprm->ftype, (char *)cprm->work, cprm->wksize);
    if (ply->sfd == NULL) {
        return NULL;
    }
    mwPlySfdSetCond(ply);
    ply->stream = mwPlySfdCreateSj();
    if (ply->stream == NULL) {
        return NULL;
    }
    ply->base.vtbl = &mwsfd_if;
    ply->cprm = *cprm;
    ply->created = TRUE;
    ply->dtype = cprm->dtype;
    ply->renderer = NULL;
    ply->decoding = FALSE;
    ply->frame_loaded = FALSE;
    ply->disp_set = FALSE;
    ply->bright = 192;
    ply->bright_ofst = 0;
    ply->stat = MWE_PLY_STAT_STOP;
    if (adxt_obj[0].used != 0) {
        ply->adxt = adxt_obj;
        ADXT_SetWaitPlayStart(ply->adxt, 1);
        ADXT_SetAutoRcvr(ply->adxt, ADXT_RMODE_NOACT);
    } else {
        ply->adxt = NULL;
    }
    ply->used = TRUE;
    return &ply->base;
}

void mwSfdDestroy(MWPLY mwply)
{
    if (mwply == NULL) {
        return;
    }
    mwSfdStop(mwply);
    /* @hack spelling: the player as MWSFD(mwply) at each use. with a local copy
     * taken at entry, as mwSfdStop takes one, 5 of 51 words differ */
    MWSFD(mwply)->used = FALSE;
    if (MWSFD(mwply)->gsc != NULL) {
        mwGscClose(MWSFD(mwply)->gsc);
    }
    if (MWSFD(mwply)->stream != NULL) {
        SJ_Destroy(MWSFD(mwply)->stream);
    }
    if (MWSFD(mwply)->sfd != NULL) {
        mwPlySfdDestroy(MWSFD(mwply)->sfd);
    }
    if (MWSFD(mwply)->renderer != NULL) {
        mwRnvDestroy(MWSFD(mwply)->renderer);
        MWSFD(mwply)->renderer = NULL;
    }
    mwPlySfdFinish(MWSFD(mwply));
}

void mwSfdStartFname(MWPLY mwply, Sint8 *fname)
{
    SJ stream;
    Sint32 error;

    /* @hack spelling: the player as MWSFD(mwply) at each use. with a local copy
     * taken at entry, as mwSfdStop takes one, 62 of 107 words differ */
    stream = MWSFD(mwply)->stream;
    mwSfdStop(mwply);
    MWSFD(mwply)->gsc = mwGscOpenFname((char *)fname, stream);
    if (MWSFD(mwply)->gsc == NULL) {
        mwSfdSetErrCode(MWSFD_ERR_GSC_OPEN);
        mwMngError("W2001 mwPlyStartFname:can't open '%s'", fname);
        return;
    }
    if (mwGscSetRdSct(MWSFD(mwply)->gsc, 100) != 0) {
        mwSfdSetErrCode(MWSFD_ERR_PARAM);
        mwMngError("W2005 mwPlyStartFname: gsc parameter miss");
    }
    if (mwGscSetTrSct(MWSFD(mwply)->gsc, 100) != 0) {
        mwSfdSetErrCode(MWSFD_ERR_PARAM);
        mwMngError("W2006 mwPlyStartFname: gsc prameter miss");
    }
    error = mwGscStart(MWSFD(mwply)->gsc);
    if (error != 0) {
        mwSfdSetErrCode(MWSFD_ERR_GSC_START);
        return;
    }
    error = SFD_Start(MWSFD(mwply)->sfd);
    if (error != 0) {
        mwGscClose(MWSFD(mwply)->gsc);
        mwSfdSetErrCode(MWSFD_ERR_START);
        mwMngError("E2002 mwPlyStartFname:can't start SFD ('%s')", fname);
        MWSFD(mwply)->gsc = NULL;
        return;
    }
    if (MWSFD(mwply)->renderer != NULL) {
        mwRnvDestroy(MWSFD(mwply)->renderer);
        MWSFD(mwply)->renderer = NULL;
    }
    MWSFD(mwply)->frame_loaded = FALSE;
    MWSFD(mwply)->stat = MWE_PLY_STAT_PREP;
}

void mwSfdStop(MWPLY mwply)
{
    struct mwsfd_player *ply = MWSFD(mwply);
    struct sfd_handle *sfd;
    Sint32 imask;
    Sint32 error;

    sfd = ply->sfd;
    imask = mwPlyLock();
    mwlSfdSleepDecSvr();
    if (ply->adxt != NULL) {
        ADXT_Stop(ply->adxt);
    }
    if (ply->gsc != NULL) {
        mwGscClose(ply->gsc);
        ply->gsc = NULL;
    }
    error = SFD_Stop(sfd);
    if (error != 0) {
        mwSfdSetErrCode(MWSFD_ERR_STOP);
        mwMngError("E2003 mwSfdStop:can't stop SFD");
    } else {
        if (ply->renderer != NULL) {
            mwRnvDestroy(ply->renderer);
            ply->renderer = NULL;
        }
        ply->stat = MWE_PLY_STAT_STOP;
    }
    mwPlyUnlock(imask);
}

MWE_PLY_STAT mwSfdGetStat(MWPLY mwply)
{
    if (mwply == NULL) {
        mwSfdSetErrCode(MWSFD_ERR_NULL_HANDLE);
        mwMngError("W2004 mwSfdGetStat: handle is NULL");
        return MWE_PLY_STAT_STOP;
    }
    return MWSFD(mwply)->stat;
}

Sint32 mwSfdSetErrCode(Sint32 code)
{
    struct mwsfd_err *err = &mwg_ply_work.err;
    void (*func)(void *obj, Sint32 code);

    err->code = code;
    if (code == 0) {
        return 0;
    }
    func = err->func;
    if (func != NULL) {
        func(err->obj, code);
    }
    return code;
}

/* the renderer's draw callback. while the audio plays, it first brings the
 * audio's read position up to the sound chip's, so that the audio's time is
 * current. a picture whose time leads the audio's by the display delay, give
 * or take less than a frame's period, draws. one that leads by more waits,
 * and one that leads by less is skipped. without audio, or while ADX talk
 * counts 0 samples, every picture draws */
Sint32 mwl_ply_draw_cbfn(void *obj, Sint32 time, Sint32 tunit, Sint32 dfno)
{
    struct mwsfd_player *ply = obj;
    Sint32 ncount;
    Sint32 tscale;
    Sint32 imask;
    Sint32 atime;
    Sint32 vtime;
    Sint32 diff;

    if (ply->adxt == NULL) {
        return MWD_RNV_DRAWCB_OK;
    }
    imask = mwPlyLock();
    if (mwRnaGetPlaySw(ply->adxt->rna) == 1) {
        mwlRnaAddRdPos(ply->adxt->rna);
    }
    mwPlyUnlock(imask);
    ADXT_GetTime(ply->adxt, &ncount, &tscale);
    atime = (Sint32)((Float)ncount * 10000.0F / (Float)tscale);
    vtime = (Sint32)((Float)time * 10000.0F / (Float)tunit);
    diff = atime - vtime;
    if (ncount == 0) {
        return MWD_RNV_DRAWCB_OK;
    }
    if (diff <= -mwg_frm_term - mwg_frm_delay) {
        mwg_cnt_wait_draw++;
        return MWD_RNV_DRAWCB_WAIT;
    } else if (diff >= mwg_frm_term - mwg_frm_delay) {
        mwg_cnt_skip_draw++;
        return MWD_RNV_DRAWCB_SKIP;
    }
    return MWD_RNV_DRAWCB_OK;
}

/* the renderer's display callback. it keeps the audio, picture and real times
 * of each picture shown, and their differences. with audio, picture 0 starts
 * the audio and sets mw_start_time */
void mwl_ply_disp_cbfn(void *obj, Sint32 time, Sint32 tunit, Sint32 dfno)
{
    struct mwsfd_player *ply = obj;
    Sint32 ncount;
    Sint32 tscale;
    Sint32 imask;

    if (ply->adxt != NULL) {
        ADXT_GetTime(ply->adxt, &ncount, &tscale);
    } else {
        ncount = 0;
        tscale = 1;
    }
    mw_atime_ms2 = (Sint32)((Float)ncount * 10000.0F / (Float)tscale);
    mw_vtime_ms2 = (Sint32)((Float)time * 10000.0F / (Float)tunit);
    mw_rtime_ms2 =
        ((Sint32)syTmrCountToMicro(syTmrGetCount()) / 100) - mw_start_time;
    mw_dif_av_time2 = mw_atime_ms2 - mw_vtime_ms2;
    mw_dif_rv_time2 = mw_rtime_ms2 - mw_vtime_ms2;
    mw_dif_ra_time2 = mw_rtime_ms2 - mw_atime_ms2;
    if (dfno != 0) {
        /* past picture 0, a picture whose number doesn't follow the last
         * one's hangs the player in this loop */
        if (dfno - mwl_last_dfno != 1) {
            for (;;) {
            }
        }
    }
    mwl_last_dfno = dfno;
    if (dfno == 0 && ply->adxt != NULL) {
        imask = mwPlyLock();
        ADXT_SetWaitPlayStart(ply->adxt, 0);
        ADXT_ExecServer();
        mw_start_time = (Sint32)syTmrCountToMicro(syTmrGetCount()) / 100;
        sdSysServer();
        mwPlyUnlock(imask);
    }
}

/* the decoding pass of a player preparing. once the movie's size is known,
 * it creates the player's renderer with the player's settings and sets the
 * player playing. either way, it feeds the decoder's input */
void mwlSfdExecDecSvrPrep(struct mwsfd_player *ply)
{
    struct sfd_handle *sfd;
    struct rnv_cprm param;
    struct sfd_mvinf info;

    sfd = ply->sfd;
    /* SFD_GetMvInf's result is ignored. for a null or free handle it writes
     * no info, and the test at 0x8ced0a00 would read info.width as the stack
     * held it. no player gets here with one: a player is in use only while
     * its handle is */
    SFD_GetMvInf(sfd, &info);
    if (info.width > 0 && ply->renderer == NULL) {
        param.nmbx = info.mb_width;
        param.nmby = info.mb_height;
        param.dtype = ply->dtype;
        ply->renderer = mwRnvCreate(0, &param);
        mwRnvEntryDrawCallbackFn(ply->renderer, mwl_ply_draw_cbfn, ply);
        mwRnvEntryDispCallbackFn(ply->renderer, mwl_ply_disp_cbfn, ply);
        if (ply->disp_set) {
            mwRnvSetDispPos(ply->renderer, ply->disp_lx, ply->disp_ly);
            mwRnvSetDispSize(ply->renderer, ply->disp_sx, ply->disp_sy);
        }
        mwRnvSetBright(ply->renderer, ply->bright);
        mwRnvSetBrightOfst(ply->renderer, ply->bright_ofst);
        ply->stat = MWE_PLY_STAT_PLAYING;
    }
    mwPlySfdSjCopyToSfd(ply);
}

void mwl_sfd_dmy(struct sfd_handle *sfd)
{
    mwg_sfd_hn = sfd;
}

void mwlSfdExecDecSvrPlaying(struct mwsfd_player *ply)
{
    struct sfd_handle *sfd;
    Sint32 stat;

    sfd = ply->sfd;
    mwPlySfdSjCopyToSfd(ply);
    stat = SFD_GetSvrStat();
    if (stat != SFD_SVR_BUSY) {
        mwl_sfd_dmy(sfd);
        ply->stat = MWE_PLY_STAT_PLAYEND;
    }
}

void mwSfdExecDecSvrHndl(struct mwsfd_player *ply)
{
    Sint32 stat;

    switch (ply->stat) {
    case MWE_PLY_STAT_STOP:
        break;
    case MWE_PLY_STAT_PREP:
        mwlSfdExecDecSvrPrep(ply);
        break;
    case MWE_PLY_STAT_PLAYING:
        mwlSfdExecDecSvrPlaying(ply);
        break;
    case MWE_PLY_STAT_PLAYEND:
        break;
    case MWE_PLY_STAT_ERROR:
        break;
    }
    if (ply->gsc != NULL) {
        if (mwGscGdGetGdfsStat(ply->gsc, &stat) == 0) {
            if (stat == GDD_STAT_ERR || stat == GDD_STAT_FATAL) {
                ply->stat = MWE_PLY_STAT_ERROR;
            }
        }
    }
}

/* the decoding fiber's pass. it runs Sofdec's server and each player's
 * decoding pass. then it runs the GD drive's, the file system's and the
 * stream servers, each with interrupts masked. a slow step records its times
 * as mw_t1a describes */
void mwSfdExecDecServer(void)
{
    Uint32 t0;
    Uint32 t1;
    Sint32 dt;
    Sint32 i;
    struct mwsfd_player *ply;
    Sint32 imask;

    t0 = syTmrGetCount();
    SFD_ExecServer();
    t1 = syTmrGetCount();
    /* @hack redundant: a never-run test of t0, which gives R10 to R13 the
     * image's variables, from t0 and t1 (0x8ced0b78, 0x8ced0b7e) on; without
     * it, 35 of 180 words differ */
    if (0) {
        if (t0) {
        }
    }
    dt = syTmrCountToMicro(syTmrDiffCount(t0, t1));
    /* @hack redundant: never-run tests of dt, with an empty else. the reads
     * put syTmrCountToMicro's pool word ahead of syTmrDiffCount's, and give
     * R8 to R13 the image's variables. the empty else puts 1000 in R2 and
     * mw_t1b's address in R3 (0x8ced0b90, 0x8ced0b98), which otherwise trade.
     * without it, 33 of 180 words differ */
    if (0) {
        if (dt) {
        }
        if (dt) {
        }
        if (dt) {
        }
        if (dt) {
        }
    } else {
    }
    if (dt >= SLOW_SFD_SERVER) {
        mw_t1a = t0;
        mw_t1b = t1;
        mw_dectime += dt;
    }
    t0 = syTmrGetCount();
    for (i = 0; i < MWSFD_MAX_PLAYERS; i++) {
        ply = &mwg_ply_work.ply[i];
        if (ply->used) {
            if (ply->stat == MWE_PLY_STAT_STOP) {
                ply->decoding = FALSE;
            } else {
                ply->decoding = TRUE;
                mwSfdExecDecSvrHndl(ply);
            }
        }
    }
    t1 = syTmrGetCount();
    dt = syTmrCountToMicro(syTmrDiffCount(t0, t1));
    /* @hack redundant: a never-run test of t1, with an empty else. the read
     * gives R12 syTmrCountToMicro's address and R11 the first t0, i and 10.
     * it also gives R8 and R10 the loop's 0 and 1, which otherwise trade. the
     * empty else puts 50 in R3 and mw_t2a's address in R2 (0x8ced0c12,
     * 0x8ced0c1a), which otherwise trade. without it, 23 of 180 words differ */
    if (0) {
        if (t1) {
        }
    } else {
    }
    if (dt >= SLOW_PLAYERS) {
        mw_t2a = t0;
        mw_t2b = t1;
        mw_splttime += dt;
    }
    imask = mwPlyLock();
    t0 = syTmrGetCount();
    gdGdcExecServer();
    t1 = syTmrGetCount();
    dt = syTmrCountToMicro(syTmrDiffCount(t0, t1));
    /* @hack redundant: a never-run test of t1, which keeps the GD drive
     * server's counts in R9 and R10 (0x8ced0c36, 0x8ced0c3c), which otherwise
     * trade; without it, 5 of 180 words differ */
    if (0) {
        if (t1) {
        }
    }
    if (dt > SLOW_SERVER) {
        mw_t5a = t0;
        mw_t5b = t1;
        mw_gdctime += dt;
    }
    mwPlyUnlock(imask);
    imask = mwPlyLock();
    t0 = syTmrGetCount();
    gdFsExecServer();
    t1 = syTmrGetCount();
    dt = syTmrCountToMicro(syTmrDiffCount(t0, t1));
    /* @hack redundant: a never-run test of t0, which keeps the file system
     * server's counts in R10 and R9 (0x8ced0c70, 0x8ced0c76), which otherwise
     * trade; without it, 5 of 180 words differ */
    if (0) {
        if (t0) {
        }
    }
    if (dt > SLOW_SERVER) {
        mw_t3a = t0;
        mw_t3b = t1;
        mw_gdfstime += dt;
    }
    mwPlyUnlock(imask);
    imask = mwPlyLock();
    t0 = syTmrGetCount();
    mwGscExecServer();
    t1 = syTmrGetCount();
    dt = syTmrCountToMicro(syTmrDiffCount(t0, t1));
    /* @hack redundant: never-run tests of t1 and dt, which keep
     * syTmrCountToMicro's address in R12, not R11, and give R8 to R12 the
     * image's variables; without it, 32 of 180 words differ */
    if (0) {
        if (t1) {
        }
        if (t1) {
        }
        if (t1) {
        }
        if (dt) {
        }
        if (dt) {
        }
    }
    if (dt > SLOW_SERVER) {
        mw_t4a = t0;
        mw_t4b = t1;
        mw_gsctime += dt;
    }
    mwPlyUnlock(imask);
}

/* the decoding fiber's body. it runs passes back to back, with
 * mwg_thread_exec_flag set during each, and switches to the main fiber
 * whenever mwg_thread_sleep_flag asks it to sleep */
void mwl_sfd_dec_thread(void *data)
{
    for (;;) {
        if (mwg_thread_sleep_flag == 1) {
            syFbrSwitchToFiber(g_fbrMain);
        } else {
            mwg_thread_exec_flag = 1;
            mwSfdExecDecServer();
            mwg_thread_exec_flag = 0;
        }
    }
}

/* the wait-vsync callback Kamui calls while it waits for a vertical blank. it
 * runs the decoding fiber until mwSfdVsync switches back, and records the timer
 * counts mw_t6a and mw_t7a describe */
void mwl_sfd_wait_vsync(void *arg)
{
    mw_t6a = mw_t6b;
    mw_t6b = syTmrGetCount();
    if (vsync_flag == 1) {
        mw_t7a = mw_t7c;
        mw_t7b = syTmrGetCount();
        vsync_flag = 0;
    }
    syFbrSwitchToFiber(g_fbrSub);
    mw_t6c = syTmrGetCount();
}

/* return once the decoding fiber is between passes. while it is in one, this
 * switches to it to let it finish, with mwg_thread_sleep_flag set so that it
 * switches straight back */
void mwlSfdSleepDecSvr(void)
{
    mwg_thread_sleep_flag = 1;
    while (mwg_thread_exec_flag == 1) {
        syFbrSwitchToFiber(g_fbrSub);
    }
    mwg_thread_sleep_flag = 0;
}

/* the display pass of one player: hand its renderer as many decoded pictures
 * as it has room for. an ADX talk error sets the player's stat to
 * MWE_PLY_STAT_ERROR */
void mwSfdExecDispSvrHndl(struct mwsfd_player *ply)
{
    struct sfd_handle *sfd;
    Sint32 rooms;
    Sint32 i;
    struct sfd_frm *frm;

    sfd = ply->sfd;
    if (ply->renderer == NULL) {
        return;
    }
    ply->frame_loaded = FALSE;
    rooms = mwRnvGetNumRoomTex(ply->renderer);
    for (i = 0; i < rooms; i++) {
        SFD_GetFrm(sfd, &frm);
        if (frm == NULL) {
            break;
        }
        mwRnvLoad(ply->renderer, (Uint32 *)frm->data, frm->time, frm->tunit);
        SFD_RelFrm(sfd, frm);
        ply->frame_loaded = TRUE;
    }
    if (ply->adxt != NULL) {
        if (ADXT_GetErrCode(ply->adxt) != ADXT_ERR_OK) {
            ply->stat = MWE_PLY_STAT_ERROR;
        }
    }
}

void mwSfdExecDispServer(void)
{
    Sint32 i;
    struct mwsfd_player *ply;

    for (i = 0; i < MWSFD_MAX_PLAYERS; i++) {
        ply = &mwg_ply_work.ply[i];
        if (ply->used) {
            mwSfdExecDispSvrHndl(ply);
        }
    }
}

/* the manager's vertical blank function. it counts the blank, and from the
 * mwg_nvsync_pic-th since mwPlyStartFrame on, switches to the main fiber once
 * the interrupt returns */
void mwSfdVsync(void *obj)
{
    Sint32 imask;

    imask = get_imask();
    set_imask(IMASK_ALL);
    mw_t7c = syTmrGetCount();
    vsync_flag = 1;
    mwg_field_no++;
    if (mwg_field_no >= mwg_nvsync_pic) {
        syFbrDelayedSwitchToFiber(g_fbrMain);
    }
    set_imask(imask);
}

/* the manager's server function: each player's display pass, then the
 * renderer's server */
void mwSfdExecServer(void *obj)
{
    /* clear Ninja's end-of-render flag, so that the next njWaitVSync doesn't
     * wait for the render to end */
    _nj_eor_flag_ = 0;
    mwg_sfd_exec_svr_cnt++;
    mwSfdExecDispServer();
    mwRnvExecServer();
}

void mwSfdStartFrame(void *obj)
{
    mwg_field_no = 0;
}

/* invalidate every operand cache line through the address array, without
 * writing any back */
static void mwl_clear_oc_tags(void)
{
    Sint32 n;
    volatile Uint32 *tag;

    tag = OPERAND_TAGS;
    for (n = 0; n < OPERAND_LINES; n++) {
        *tag = 0;
        tag += OPERAND_TAG_STEP;
    }
}

/* invalidate the operand cache, then turn both caches on, the operand cache in
 * index mode. code that writes CCR must run uncached, so mwPlyInitSofdec calls
 * this through its P2 alias */
static void mwl_cache_index_on(void)
{
    Sint32 imask;

    imask = get_imask();
    set_imask(IMASK_ALL);
    mwl_clear_oc_tags();
    CCR = 0;
    CCR = CCR_ICE | CCR_OIX | CCR_CB | CCR_OCE;
    set_imask(imask);
}

/* invalidate the operand cache, then turn both caches on, with the operand
 * cache's index mode off: address bits 13 to 5 index it */
static void mwl_cache_index_off(void)
{
    Sint32 imask;

    imask = get_imask();
    set_imask(IMASK_ALL);
    mwl_clear_oc_tags();
    CCR = 0;
    CCR = CCR_ICE | CCR_CB | CCR_OCE;
    set_imask(imask);
}

void mwPlyInitSofdec(MWS_PLY_INIT_SFD *iprm)
{
    Sint32 error;

    mwMngInit();
    ADXT_Init();
    mwRnvInit();
    mwSjRbfInit();
    error = mwGscInit(1, mwsfd_gsc_work);
    if (error != 0) {
        mwSfdSetErrCode(MWSFD_ERR_GSC_INIT);
        mwMngError("E2005 mwPlyInitSofdec: can't init GSC");
    }
    memset(&mwg_ply_work, 0, sizeof(mwg_ply_work));
    ((void (*)(void))P2(mwl_cache_index_on))();
    syFbrInit(g_fbrWork, MWSFD_FIBERS);
    g_fbrMain = syFbrConvertThreadToFiber(NULL);
    g_fbrSub = syFbrCreateFiber(g_subStack, sizeof(g_subStack),
                                mwl_sfd_dec_thread, NULL);
    syFbrSetIntStack(mwsfdg_intStack + MWSFD_INT_STACK_WORDS);
    kmSetWaitVsyncCallback(mwl_sfd_wait_vsync, NULL);
    if (iprm == NULL) {
        mwg_disp_latency = 2;
        mwg_vsync_mhz = 29970;
    }
    mwg_frm_term = 10000000 / mwg_vsync_mhz;
    mwg_frm_delay = mwg_disp_latency * mwg_frm_term;
    mwMngEntryServerFunc(mwSfdExecServer, NULL);
    mwMngEntryVsyncFunc(mwSfdVsync, NULL);
    mwMngEntryStartFrameFunc(mwSfdStartFrame, NULL);
}

void mwPlyFinishSofdec(void)
{
    kmSetWaitVsyncCallback(NULL, NULL);
    syFbrDeleteFiber(g_fbrSub);
    mwMngDeleteStartFrameFunc(mwSfdStartFrame);
    mwMngDeleteVsyncFunc(mwSfdVsync);
    mwMngDeleteServerFunc(mwSfdExecServer);
    ADXT_Finish();
    mwRnvFinish();
    mwGscFinish();
    mwSjRbfFinish();
    /* @bug logic: mwl_cache_index_off writes CCR while it runs cached, from
     * P1, though code that writes CCR must run uncached. the BSR at
     * 0x8ced11ba calls it at its P1 address, where mwPlyInitSofdec calls
     * mwl_cache_index_on through P2 (OR R3,R2 at 0x8ced105e) */
#ifdef GCC
    /* repairs the cached write the tag names: GCC calls mwl_cache_index_off
     * through P2 too */
    ((void (*)(void))P2(mwl_cache_index_off))();
#else
    mwl_cache_index_off();
#endif
    mwMngFinish();
}

void mwSfdGetTime(MWPLY mwply, Sint32 *ncount, Sint32 *tscale)
{
    /* @hack spelling: the player as MWSFD(mwply) at each use. with a local copy
     * taken at entry, as mwSfdStop takes one, the object runs 4 bytes short
     * and 19 of 19 words differ */
    if (MWSFD(mwply)->adxt != NULL) {
        ADXT_GetTime(MWSFD(mwply)->adxt, ncount, tscale);
    } else {
        *ncount = 0;
        *tscale = 2997;
    }
}

void mwPlySetFastHalfpel(MWPLY mwply, Sint32 sw)
{
    struct sfd_handle *sfd = MWSFD(mwply)->sfd;

    SFD_SetMpvCond(sfd, MPV_COND_HALF_PEL_2, sw);
}

void mwPlySetDispPos(MWPLY mwply, Float lx, Float ly)
{
    struct mwsfd_player *ply = MWSFD(mwply);

    ply->disp_lx = lx;
    ply->disp_ly = ly;
    ply->disp_set = TRUE;
    mwRnvSetDispPos(ply->renderer, lx, ly);
}

void mwPlySetDispSize(MWPLY mwply, Float sx, Float sy)
{
    struct mwsfd_player *ply = MWSFD(mwply);

    ply->disp_sx = sx;
    ply->disp_sy = sy;
    ply->disp_set = TRUE;
    if (ply->renderer != NULL) {
        mwRnvSetDispSize(ply->renderer, sx, sy);
    }
}

void mwPlySetBright(MWPLY mwply, Sint32 val)
{
    struct mwsfd_player *ply = MWSFD(mwply);

    ply->bright = val;
    if (ply->renderer != NULL) {
        mwRnvSetBright(ply->renderer, val);
    }
}

void mwPlySetBrightOfst(MWPLY mwply, Sint32 val)
{
    struct mwsfd_player *ply = MWSFD(mwply);

    ply->bright_ofst = val;
    if (ply->renderer != NULL) {
        mwRnvSetBrightOfst(ply->renderer, val);
    }
}

Sint32 mwPlyGetBright(MWPLY mwply)
{
    return MWSFD(mwply)->bright;
}

Sint32 mwPlyGetBrightOfst(MWPLY mwply)
{
    return MWSFD(mwply)->bright_ofst;
}

void mwSfdPause(MWPLY mwply, Sint32 sw)
{
}

void mwSfdSetOutVol(MWPLY mwply, Sint32 vol)
{
    ADXT_SetOutVol(MWSFD(mwply)->adxt, vol);
}

Sint32 mwSfdGetOutVol(MWPLY mwply)
{
    return ADXT_GetOutVol(MWSFD(mwply)->adxt);
}

void mwSfdSetOutPan(MWPLY mwply, Sint32 chno, Sint32 pan)
{
    ADXT_SetOutPan(MWSFD(mwply)->adxt, chno, pan);
}

Sint32 mwSfdGetOutPan(MWPLY mwply, Sint32 chno)
{
    return ADXT_GetOutPan(MWSFD(mwply)->adxt, chno);
}
