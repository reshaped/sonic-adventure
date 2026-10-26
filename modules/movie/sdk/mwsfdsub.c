/* the Sofdec player's handle: its setup, its conditions, and the copy of the
 * stream into its input */
#include <sg_mwply.h>
#include <sg_xpt.h>
#include <sj.h>

#include "movie/sdk/mwsfd.h"
#include "movie/sdk/rnv.h"
#include "movie/sdk/sfd.h"
#include "movie/sdk/sjrbf.h"

/* mwMngError under a second name with fixed arguments, so that a call passes
 * them in R4 and R5, as the image's call does. mwMngError is variadic, and
 * reads its arguments from the stack. a cast of mwMngError to this type
 * schedules the call otherwise */
extern void mwmng_error_register_args(const char *format, Sint32 value);

#define TEXT_1_LENGTH 32
#include "movie/sdk/mwsfdsub/text.inc"

/* the room each region of work takes past its size, to round its start up to
 * 32 */
#define ALIGN_ROOM 32
/* the regions mwPlySfdCreate lays out in work, in order */
#define HANDLE_BYTES 0x100000 /* the handle's three buffers */
#define STREAM_BYTES 0x60000 /* the stream joint's ring */
#define REFERENCE_BYTES 0x7f800 /* two reference frames */
#define PICTURE_BYTES 0xf7800 /* four decoded pictures */
#define AUDIO_RING_BYTES 0x12000 /* the audio decoder's ring */
#define AUDIO_PLAYER_BYTES 0x80a0 /* the audio decoder's player */

/* SFD_Init's tunit for the vertical blank clock, the blanks in 1000 seconds.
 * the vertical blank time source counts 1000 for each blank, in units of
 * 1/tunit second */
#define VSYNC_TUNIT 59939
/* the input unit in bytes, which the system stage reads whole */
#define INPUT_UNIT 2048

/* the error codes mwPlySfdErrFunc keeps */
#define ERR_SLOTS 16

static struct sfd_handle mwg_sfd_libwk;
/* the start of the stream joint's ring in the last layout, rounded up to 32 */
static char *mwg_sfd_sjadr;
static Sint32 mwg_sfd_errcode[ERR_SLOTS];
static Sint32 mwg_sfd_errcnt;

/* the stages SFD_Init initializes and SFD_Finish finishes, in order, up to the
 * first NULL */
static const struct sfd_trlist mwg_sfd_mps_trentry = {
    {&SFD_tr_in_mem, &SFD_tr_sd_mps, &SFD_tr_vd_mpv, &SFD_tr_vo_manu,
     &SFD_tr_ad_adxt, &SFD_tr_ao_auto_p}};
static const struct sfd_trlist mwg_sfd_mpv_trentry = {
    {&SFD_tr_in_mem, &SFD_tr_vd_mpv, &SFD_tr_vo_manu}};

static const struct sfd_config mwg_sfd_mps_initpara = {
    (struct sfd_trlist *)&mwg_sfd_mps_trentry, &mwg_sfd_libwk, 1, VSYNC_TUNIT};
static const struct sfd_config mwg_sfd_mpv_initpara = {
    (struct sfd_trlist *)&mwg_sfd_mpv_trentry, &mwg_sfd_libwk, 1, VSYNC_TUNIT};

/* each new handle's stages, indexed by SFD_TR_*. SFD_Create reads only the
 * first SFD_TR_MAX slots of its struct sfd_trlist, so a cast lets these
 * shorter arrays stand in for one */
static const struct sfd_trif *mwg_ply_mps_trsetup[SFD_TR_MAX] = {
    &SFD_tr_in_mem,
    &SFD_tr_sd_mps,
    &SFD_tr_vd_mpv,
    &SFD_tr_ad_adxt,
    0,
    0,
    &SFD_tr_vo_manu,
    &SFD_tr_ao_auto_p,
    0};
static const struct sfd_trif *const mwg_sfd_mpv_trsetup[SFD_TR_MAX] = {
    &SFD_tr_in_mem, 0, &SFD_tr_vd_mpv, 0, 0, 0, &SFD_tr_vo_manu, 0, 0};

static struct sfd_create_param mwg_sfd_mps_crepara = {
    (struct sfd_trlist *)mwg_ply_mps_trsetup,
    0,
    {0x70000, 0x60000, 0x30000},
    INPUT_UNIT,
    {4, 352, 480, 1}};
static struct sfd_create_param mwg_sfd_mpv_crepara = {
    (struct sfd_trlist *)mwg_sfd_mpv_trsetup,
    0,
    {0x70000, 0x60000, 0x30000},
    INPUT_UNIT,
    {4, 352, 480, 1}};
static struct sfmpv_para mwg_sfd_mpvpara = {192, 240, 352, 480, 0,
                                            352, 480, 4,   0};
static struct sfadxt_para mwg_sfd_adxtpara = {
    AUDIO_RING_BYTES, 0, 0, 2, 44100, AUDIO_PLAYER_BYTES, 0};
/* the bytes of work the last mwPlySfdCreate laid out */
static Sint32 mwply_work_size = 0;

/* a word nothing reads, at the end of the object's data */
static Sint32 dat_8cedd674 = 0;

Sint32 mwPlySfdInit(struct mwsfd_player *ply, Sint32 file_type)
{
    const struct sfd_config *configuration;
    Sint32 error;

    switch (file_type) {
    case MWSFD_FILE_SYSTEM:
        configuration = &mwg_sfd_mps_initpara;
        break;
    case MWSFD_FILE_VIDEO:
        configuration = &mwg_sfd_mpv_initpara;
        break;
    default:
        return mwSfdSetErrCode(MWSFD_ERR_FILE_TYPE);
    }
    error = SFD_Init((struct sfd_config *)configuration);
    if (error != 0) {
        return mwSfdSetErrCode(MWSFD_ERR_INIT);
    }
    error = SFD_SetErrFn(0, mwPlySfdErrFunc, 0);
    if (error != 0) {
        return mwSfdSetErrCode(MWSFD_ERR_ERROR_FN);
    }
    return 0;
}

Sint32 mwPlySfdFinish(struct mwsfd_player *ply)
{
    SFD_Finish();
    return 0;
}

struct sfd_handle *mwPlySfdCreate(struct mwsfd_player *ply, Sint32 file_type,
                                  char *work, Sint32 work_size)
{
    char *base;
    char *stream_area;
    char *reference_area;
    char *picture_area;
    char *audio_ring_area;
    char *audio_player_area;
    Sint32 need;
    struct sfd_create_param param;
    struct sfd_handle *sfd;
    Sint32 error;

    base = work;
    work += HANDLE_BYTES + ALIGN_ROOM;
    stream_area = work;
    work += STREAM_BYTES + ALIGN_ROOM;
    reference_area = work;
    work += REFERENCE_BYTES + ALIGN_ROOM;
    picture_area = work;
    work += PICTURE_BYTES + ALIGN_ROOM;
    audio_ring_area = work;
    work += AUDIO_RING_BYTES + ALIGN_ROOM;
    audio_player_area = work;
    work += AUDIO_PLAYER_BYTES + ALIGN_ROOM;
    mwply_work_size = work - base;
    need = mwply_work_size;
    if (need > work_size) {
        /* @bug mismatched-call: the call passes TEXT_1 and the shortfall in R4
         * and R5 (0x8ced4d90, 0x8ced4d98), but mwMngError takes its format from
         * the stack (0x8ced13d8). the word there is work_size (0x8ced4d64),
         * which the report reads as its format's address */
        mwmng_error_register_args(TEXT_1, need - work_size);
        return 0;
    }
    /* @bug logic: only the system stream's parameters take base (0x8ced4da2).
     * a video stream's handle keeps the work address 0 its parameters start
     * with, and lays its buffers out from there */
    mwg_sfd_mps_crepara.work = base;
    mwg_sfd_mpvpara.rfb = (unsigned char *)reference_area;
    mwg_sfd_mpvpara.frm = (unsigned char *)picture_area;
    mwg_sfd_adxtpara.rbuf = audio_ring_area;
    mwg_sfd_adxtpara.work = audio_player_area;
    mwg_sfd_sjadr = (char *)(((unsigned long)stream_area + 31) & ~31UL);
    switch (file_type) {
    case MWSFD_FILE_SYSTEM:
        param = mwg_sfd_mps_crepara;
        /* @hack spelling: the store through param's address, which SHC forms in
         * R2 (MOV R15,R2 and MOV.L R3,@(40,R2) at 0x8ced4e36 and 0x8ced4e3a);
         * as param.unit, the store folds into @(60,R15), 61 of 125 words differ
         * and the function is 6 bytes shorter */
        (&param)->unit = INPUT_UNIT;
        break;
    case MWSFD_FILE_VIDEO:
        param = mwg_sfd_mpv_crepara;
        break;
    }
    SFD_SetMpvPara(&mwg_sfd_mpvpara);
    SFD_SetAdxtPara(&mwg_sfd_adxtpara);
    sfd = SFD_Create(&param, 0);
    if (sfd == 0) {
        mwSfdSetErrCode(MWSFD_ERR_CREATE);
        return 0;
    }
    error = SFD_SetErrFn(sfd, mwPlySfdErrFunc, 0);
    if (error != 0) {
        mwSfdSetErrCode(MWSFD_ERR_ERROR_FN);
        return 0;
    }
    return sfd;
}

void mwPlySfdDestroy(struct sfd_handle *sfd)
{
    Sint32 error;

    error = SFD_Destroy(sfd);
    if (error != 0) {
        mwSfdSetErrCode(MWSFD_ERR_DESTROY);
    }
}

Sint32 mwSfdIsSkip(struct sfd_handle *handle, Sint32 type, Sint32 time,
                   Sint32 tunit)
{
    /* @hack redundant: the return in a loop of one pass, which leaves RTS's
     * slot to a NOP (0x8ced4ec6). as a plain return, MOV #0,R0 fills the
     * slot, the object runs 4 bytes short and 3 of 3 words differ */
    for (;;) {
        return 0;
    }
}

Sint32 mwPlySfdSetCond(struct mwsfd_player *ply)
{
    struct sfd_handle *sfd;

    sfd = ply->sfd;
    SFD_SetCond(sfd, SFD_COND_REFEED, 0);
    /* conditions 1 and 0, which nothing in MOVIE.BIN reads */
    SFD_SetCond(sfd, 1, 1);
    SFD_SetCond(sfd, 0, 0);
    SFD_SetCond(sfd, SFD_COND_PREP_PICS, 4);
    SFD_SetCond(sfd, SFD_COND_DEC_TIME, 8000);
    SFD_SetCond(sfd, SFD_COND_TIME_SRC, SFTIM_SRC_NONE);
    return SFD_SetUsrIsSkipFn(sfd, mwSfdIsSkip);
}

Sint32 mwPlySfdStart(struct mwsfd_player *ply)
{
    struct sfd_handle *sfd;
    Sint32 error;

    sfd = ply->sfd;
    error = SFD_Start(sfd);
    if (error != 0) {
        return mwSfdSetErrCode(MWSFD_ERR_START);
    }
    return 0;
}

Sint32 mwPlySfdStop(struct mwsfd_player *ply)
{
    struct sfd_handle *sfd;
    Sint32 error;

    sfd = ply->sfd;
    error = SFD_Stop(sfd);
    if (error != 0) {
        return mwSfdSetErrCode(MWSFD_ERR_STOP);
    }
    return 0;
}

void mwPlySfdGetWritePtr(struct mwsfd_player *ply, char **area, Sint32 *size,
                         Sint32 *written)
{
    struct sfd_handle *sfd;
    struct sfbuf_ring_inf info;

    sfd = ply->sfd;
    SFD_GetWritePtr(sfd, &info);

    /* @hack spelling: info's members read through its address, which SHC forms
     * again for each (MOV R15 and ADD #12 at 0x8ced4fae, 0x8ced4fb6,
     * 0x8ced4fbe). as info.area, info.len and info.written, the loads read
     * @(12,R15), @(16,R15) and @(32,R15) straight, the object runs 12 bytes
     * short and 19 of 29 words differ */
    *area = (&info)->area;
    *size = (&info)->len;
    *written = (&info)->written;
}

void mwPlySfdAddWritePtr(struct mwsfd_player *ply, Sint32 size, Sint32 written)
{
    struct sfd_handle *sfd = ply->sfd;

    if (size >= 0) {
        SFD_AddWritePtr(sfd, size, written);
    }
}

void mwPlySfdExecServer(struct mwsfd_player *ply)
{
    struct sfd_handle *sfd;
    struct sfd_mvinf info;
    struct rnv_cprm param;

    sfd = ply->sfd;
    SFD_ExecServer();
    SFD_GetMvInf(sfd, &info);
    if (info.width > 0 && ply->renderer == 0) {
        param.nmbx = info.mb_width;
        param.nmby = info.mb_height;

        /* @bug uninitialized: param.dtype stays unset. the image stores only
         * nmbx and nmby (0x8ced5014, 0x8ced501a), and mwRnvCreate reads dtype
         * as the stack held it (0x8ced2058). no code calls this function, so
         * the read never runs */
        ply->renderer = mwRnvCreate(0, &param);
    }
}

Sint32 mwPlySfdIsEndDec(struct mwsfd_player *ply)
{
    Sint32 status;

    status = SFD_GetSvrStat();
    if (status != SFD_SVR_BUSY) {
        return 1;
    }
    return 0;
}

/* Sofdec's error function: keep each nonzero code. the first ERR_SLOTS - 1
 * fill the slots in order, and each later one replaces the last slot's */
void mwPlySfdErrFunc(void *argument, Sint32 code)
{
    if (code != 0) {
        mwg_sfd_errcode[mwg_sfd_errcnt] = code;
        if (mwg_sfd_errcnt < ERR_SLOTS - 1) {
            mwg_sfd_errcnt++;
        }
    }
}

void mwPlySfdSjInit(void)
{
    mwSjRbfInit();
}

void mwPlySfdSjFinish(void)
{
}

SJ mwPlySfdCreateSj(void)
{
    SJ stream;

    stream = SJRBF_Create(mwg_sfd_sjadr, STREAM_BYTES, 0);
    return stream;
}

void mwPlySfdSjDestroy(struct mwsfd_player *ply)
{
    SJ stream;

    stream = ply->stream;
    stream->vtbl->Destroy(stream);
}

void mwPlySfdSjCopyToSfd(struct mwsfd_player *ply)
{
    Sint32 size;
    char *area;
    Sint32 written;
    SJCK chunk;
    SJ stream;
    struct sfd_handle *sfd;

    sfd = ply->sfd;
    stream = ply->stream;
    if (sfd == 0) {
        return;
    }
    mwPlySfdGetWritePtr(ply, &area, &size, &written);
    size = size / 32 * 32;
    if (size <= 0) {
        return;
    }
    stream->vtbl->GetChunk(stream, SJ_LIN_DATA, size, &chunk);
    mwPlySh4CpyMem(area, chunk.data, chunk.len);
    stream->vtbl->PutChunk(stream, SJ_LIN_FREE, &chunk);
    mwPlySfdAddWritePtr(ply, chunk.len, written);
}

void mwPlySetAudioSw(struct mwsfd_player *ply, Sint32 on)
{
    struct sfd_handle *sfd;

    sfd = ply->sfd;
    if (on == 1) {
        mwg_ply_mps_trsetup[SFD_TR_AUDIO] = &SFD_tr_ad_adxt;
        SFD_SetCond(sfd, SFD_COND_AUDIO, 1);
    } else {
        mwg_ply_mps_trsetup[SFD_TR_AUDIO] = 0;
        SFD_SetCond(sfd, SFD_COND_AUDIO, 0);
    }
}
