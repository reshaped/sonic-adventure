/* MPEG system stream parser initialization and shutdown, errors and handles */
#include <sg_xpt.h>
#include <sofdec.h>
#include <string.h>

#include "movie/sdk/mps.h"

static void init_pack_hd(struct mps_pack_hd *pack);
static void init_sys_hd(struct mps_sys_hd *sys);
static void init_pket_hd(struct mps_pket_hd *pket);
Sint32 mpslib_InitLibWork(Sint32 nhn, void *work);
void mpslib_InitErr(struct mps_err *err);
void mpslib_SetErrSub(struct mps_err *err, Sint32 code);
void mpslib_SetErrFnSub(struct mps_err *err, mps_err_func func, void *obj);
struct mps_handle *mpslib_SearchFreeHn(void);
struct mps_handle *mpslib_InitHn(struct mps_handle *mps);

struct mps_lib_work *MPSLIB_libwork;
/* the handle MPSLIB_CheckHn checked last */
struct mps_handle *mpslib_hn_last;

Sint32 MPS_Init(Sint32 nhn, void *work)
{
    Sint32 err;

    /* @hack spelling: err assigned in the test, which SHC tests in R0 (TST
     * R0,R0 at 0x8cedab02) and copies to R6 in the branch's delay slot;
     * assigned first, it is copied to R4 to be tested, and 4 of 19 words differ
     */
    if ((err = mpslib_InitLibWork(nhn, work)) != 0) {
        return err;
    }
    MPSDEC_Init();
    MPSGET_Init();
    return 0;
}

/* lay the library out in work: its own fields, then nhn handles, all free.
 * return 0 */
Sint32 mpslib_InitLibWork(Sint32 nhn, void *work)
{
    struct mps_handle *hn;
    Sint32 i;

    MPSLIB_libwork = work;
    UTY_MemsetDword(MPSLIB_libwork, 0,
                    (sizeof(struct mps_lib_work) +
                     (sizeof(struct mps_handle) * (nhn - 1))) /
                        sizeof(Uint32));
    strncpy(MPSLIB_libwork->version, "MPS Version 1.01 1998-10-14",
            sizeof MPSLIB_libwork->version);
    strncpy(MPSLIB_libwork->copyright, "Copyright (c) 1997,1998 CRI",
            sizeof MPSLIB_libwork->copyright);
    mpslib_InitErr(&MPSLIB_libwork->err);
    MPSLIB_libwork->nhn = nhn;
    hn = MPSLIB_libwork->hn;
    for (i = 0; i < nhn; i++) {
        hn[i].stat = MPS_STAT_FREE;
    }
    return 0;
}

void MPS_Finish(void)
{
    struct mps_handle *hn;
    Sint32 nhn;
    Sint32 i;

    nhn = MPSLIB_libwork->nhn;
    hn = MPSLIB_libwork->hn;
    for (i = 0; i < nhn; i++) {
        if (hn[i].stat == MPS_STAT_FREE) {
            continue;
        }
        MPS_Destroy(&hn[i]);
    }
    MPSDEC_Finish();
    MPSGET_Finish();
}

void mpslib_InitErr(struct mps_err *err)
{
    MPSLIB_InitErrInf(err);
}

void MPSLIB_InitErrInf(struct mps_err *err)
{
    err->func = NULL;
    err->obj = NULL;
    err->code = 0;
}

Sint32 MPSLIB_SetErr(struct mps_handle *mps, Sint32 code)
{
    if (mps == NULL) {
        mpslib_SetErrSub(&MPSLIB_libwork->err, code);
    } else {
        mpslib_SetErrSub(&mps->err, code);
    }
    return code;
}

void mpslib_SetErrSub(struct mps_err *err, Sint32 code)
{
    mps_err_func func;

    err->code = code;
    if (code != 0) {
        func = err->func;
        if (func != NULL) {
            func(err->obj, code);
        }
    }
}

Sint32 MPS_SetErrFn(struct mps_handle *mps, mps_err_func func, void *obj)
{
    if (mps == NULL) {
        mpslib_SetErrFnSub(&MPSLIB_libwork->err, func, obj);
    } else {
        if (MPSLIB_CheckHn(mps) != 0) {
            return MPSLIB_SetErr(NULL, MPS_ERR_SETERRFN);
        }
        mpslib_SetErrFnSub(&mps->err, func, obj);
    }
    return 0;
}

void mpslib_SetErrFnSub(struct mps_err *err, mps_err_func func, void *obj)
{
    err->func = func;
    err->obj = obj;
}

Sint32 MPS_GetErrInf(struct mps_handle *mps, struct mps_err *err)
{
    if (mps == NULL) {
        *err = MPSLIB_libwork->err;
    } else {
        if (MPSLIB_CheckHn(mps) != 0) {
            return MPSLIB_SetErr(NULL, MPS_ERR_GETERRINF);
        }
        *err = mps->err;
    }
    return 0;
}

Sint32 MPSLIB_CheckHn(struct mps_handle *mps)
{
    mpslib_hn_last = mps;
    if (mps == NULL || mps->stat == MPS_STAT_FREE) {
        return -1;
    }
    return 0;
}

struct mps_handle *MPS_Create(void)
{
    struct mps_handle *mps;

    /* @hack spelling: mps assigned in the test, which SHC tests in R0 (TST
     * R0,R0 at 0x8cedad1a) and copies to R4 in the branch's delay slot;
     * assigned first, it is copied to R4 and tested there, and 3 of 14 words
     * differ */
    if ((mps = mpslib_SearchFreeHn()) == NULL) {
        return NULL;
    }
    /* @hack spelling: mpslib_InitHn's result through mps, which makes the
     * call a BSR (0x8cedad26); returned straight, SHC branches to it and 2 of
     * 14 words differ */
    mps = mpslib_InitHn(mps);
    return mps;
}

struct mps_handle *mpslib_SearchFreeHn(void)
{
    struct mps_handle *hn;
    struct mps_handle *mps;
    Sint32 nhn;
    Sint32 i;

    nhn = MPSLIB_libwork->nhn;
    hn = MPSLIB_libwork->hn;
    for (i = 0; i < nhn; i++) {
        mps = &hn[i];
        if (mps->stat == MPS_STAT_FREE) {
            return mps;
        }
    }
    return NULL;
}

struct mps_handle *mpslib_InitHn(struct mps_handle *mps)
{
    UTY_MemsetDword(mps, 0, sizeof(struct mps_handle) / sizeof(Uint32));
    mps->stat = MPS_STAT_USED;
    MPSLIB_InitErrInf(&mps->err);
    init_pack_hd(&mps->pack);
    init_sys_hd(&mps->sys);
    init_sys_hd(&mps->sys_by_kind[MPS_SYS_AUDIO]);
    init_sys_hd(&mps->sys_by_kind[MPS_SYS_VIDEO]);
    init_sys_hd(&mps->sys_by_kind[MPS_SYS_OTHER]);
    init_pket_hd(&mps->pket);
    return mps;
}

static void init_pack_hd(struct mps_pack_hd *pack)
{
    pack->mpeg1 = MPS_UNREAD;
    pack->scr = MPS_UNREAD;
    pack->mux_rate = MPS_UNREAD;
}

static void init_sys_hd(struct mps_sys_hd *sys)
{
    sys->len = MPS_UNREAD;
    sys->rate_bound = MPS_UNREAD;
    sys->audio_bound = MPS_UNREAD;
    sys->video_bound = MPS_UNREAD;
    sys->fixed = MPS_UNREAD;
    sys->csps = MPS_UNREAD;
    sys->audio_lock = MPS_UNREAD;
    sys->video_lock = MPS_UNREAD;
}

static void init_pket_hd(struct mps_pket_hd *pket)
{
    pket->stream_id = MPS_UNREAD;
    pket->kind = MPS_UNREAD;
    pket->number = MPS_UNREAD;
    pket->len = MPS_UNREAD;
    pket->buf_size = MPS_UNREAD;
    pket->pts = MPS_UNREAD;
    pket->dts = MPS_UNREAD;
    pket->data_len = MPS_UNREAD;
}

Sint32 MPS_Destroy(struct mps_handle *mps)
{
    if (MPSLIB_CheckHn(mps) != 0) {
        return MPSLIB_SetErr(NULL, MPS_ERR_HN);
    }
    mps->stat = MPS_STAT_FREE;
    return 0;
}
