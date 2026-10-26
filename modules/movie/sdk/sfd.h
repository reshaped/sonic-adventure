/* the Sofdec player's records, and the calls between its members. MOVIE.BIN
 * links every member declared here, and 1ST_READ.BIN none */
#ifndef MOVIE_SDK_SFD_H
#define MOVIE_SDK_SFD_H
#include <sg_xpt.h>
#include <sj.h>
#include <sofdec.h>

#include "movie/sdk/mps.h"
#include "movie/sdk/mpv.h"

/* a handle's transfer stages, through which the stream passes from buffer to
 * buffer */
enum sfd_tr {
    SFD_TR_INPUT,
    SFD_TR_SYSTEM, /* the system stream demultiplexer */
    SFD_TR_VIDEO,
    SFD_TR_AUDIO,
    SFD_TR_VIDEO_POST,
    SFD_TR_AUDIO_POST,
    SFD_TR_VIDEO_OUT,
    SFD_TR_AUDIO_OUT,
    SFD_TR_USER, /* from the system stream's private packets */
    SFD_TR_MAX
};

/* the buffers between the stages */
enum sfd_buf {
    SFD_BUF_INPUT,
    SFD_BUF_VIDEO,
    SFD_BUF_AUDIO,
    SFD_BUF_FRAME,
    SFD_BUF_PCM,
    SFD_BUF_FRAME_POST,
    SFD_BUF_PCM_POST,
    SFD_BUF_USER,
    SFD_BUF_MAX
};

/* the indexes of a stage's functions in its sfd_trif */
enum sfd_slot {
    SFD_SLOT_INIT,
    SFD_SLOT_FINISH,
    SFD_SLOT_EXEC_SERVER,
    SFD_SLOT_CREATE,
    SFD_SLOT_DESTROY,
    SFD_SLOT_STANDBY,
    SFD_SLOT_START,
    SFD_SLOT_STOP,
    SFD_SLOT_PAUSE,
    SFD_SLOT_GET_WRITE,
    SFD_SLOT_ADD_WRITE,
    SFD_SLOT_GET_READ,
    SFD_SLOT_ADD_READ,
    SFD_SLOT_MAX
};

/* a handle's state, in stat, and the state requested of it, in req. a failed
 * handle's stat is minus the state it failed in, as SFLIB_SetErr leaves it */
enum sfd_stat {
    SFD_STAT_FREE,
    SFD_STAT_STOP,
    SFD_STAT_PREP,
    SFD_STAT_STBY,
    SFD_STAT_PLAY,
    SFD_STAT_PAUSE,
    SFD_STAT_FIN
};

/* the indexes of a handle's conditions, in cond, that the player's members
 * name. sfd_ply.c's sfply_cond_dfl holds each new handle's conditions */
/* whether the handle decodes I, P and B pictures */
#define SFD_COND_DEC_I 2
#define SFD_COND_DEC_P 3
#define SFD_COND_DEC_B 4
#define SFD_COND_VIDEO 5 /* whether the handle plays video */
#define SFD_COND_AUDIO 6 /* whether the handle plays audio */
/* whether sfply_ResetHn keeps the input written to a handle with no work area
 * of its own */
#define SFD_COND_REFEED 8
/* whether every decoded picture is due at once, whatever its time */
#define SFD_COND_DISP_ALWAYS 14
#define SFD_COND_TIME_SRC 15 /* the handle's time source, an SFTIM_SRC_* */
/* the time the handle ends at, in units of 1/SFD_COND_END_TUNIT second, or
 * sfd_ply.c's SFPLY_NO_END_TIME for none */
#define SFD_COND_END_TIME 20
#define SFD_COND_END_TUNIT 21
/* the input bytes the system stage waits for before its outputs are ready, or
 * the input buffer's size if less */
#define SFD_COND_PREP_SIZE 22
/* the pictures the video decoder stage waits for before its output is ready,
 * or hn->prm.unk_2c[0] if fewer */
#define SFD_COND_PREP_PICS 23
#define SFD_COND_END_OUT 25 /* an SFD_END_OUT_* */
/* the sfd_picfn for each picture decoded, and for each one skipped, or NULL */
#define SFD_COND_DEC_PIC_FN 36
#define SFD_COND_SKIP_PIC_FN 37
#define SFD_COND_SKIP_LATE 38 /* whether a late picture is skipped */
#define SFD_COND_DEC_ALL 39 /* whether no picture is skipped */
/* the time a picture takes to decode, in units of 1/SFD_COND_DEC_TUNIT
 * second */
#define SFD_COND_DEC_TIME 42
#define SFD_COND_DEC_TUNIT 43
#define SFD_COND_MAX 44

/* SFD_COND_END_OUT values: the outputs whose end ends a handle */
#define SFD_END_OUT_BOTH 0
#define SFD_END_OUT_AUDIO 1
#define SFD_END_OUT_VIDEO 2
#define SFD_END_OUT_EITHER 3

/* SFD_COND_TIME_SRC values: the handle's time source */
enum sftim_src {
    SFTIM_SRC_NONE,
    SFTIM_SRC_VSYNC,
    SFTIM_SRC_AUDIO,
    SFTIM_SRC_USER_FRAME,
    SFTIM_SRC_USER,
    SFTIM_SRC_MAX
};

/* times a time source stores in place of a time */
#define SFTIM_NOT_YET (-1) /* the handle's time has not started */
#define SFTIM_NONE (-2) /* no time; every picture is due */

struct sfd_handle;

/* a stage function. it returns 0, or an error code. the init and finish slots
 * are called with hn NULL and the three arguments 0 */
typedef Sint32 (*sfd_trfn)(struct sfd_handle *hn, Sint32 arg1, Sint32 arg2,
                           Sint32 arg3);

/* a stage's function table, indexed by SFD_SLOT_* */
struct sfd_trif {
    sfd_trfn fn[SFD_SLOT_MAX];
};

/* the stage function tables a handle is built from, indexed by SFD_TR_*, or
 * NULL for a stage the handle lacks. SFTRN_Init and SFTRN_Finish call each
 * table's init or finish, up to the first NULL or the first error */
#define SFD_TRLIST_MAX 15
struct sfd_trlist {
    const struct sfd_trif *trif[SFD_TRLIST_MAX];
};

/* SFD_Create's parameters */
struct sfd_create_param {
    struct sfd_trlist *trlist;
    char *work;
    Sint32 bufsize[SFD_BUF_MAX]; /* each buffer's size within work */
    Sint32 unit; /* the input unit, which the demultiplexer reads whole */
    Sint32 unk_2c[4];
};

/* an error function. it is called with the object it was set with, and the
 * error code */
typedef void (*sfd_errfn)(void *obj, Sint32 ecode);

struct sfd_err {
    sfd_errfn func;
    void *obj;
    Sint32 ecode; /* the last error code recorded */
    Sint32 unk_0c;
    Sint32 unk_10;
};

/* a handle's movie information, which the video decoder fills from a picture
 * while bitrate is 0 */
struct sfd_mvinf {
    Sint32 width;
    Sint32 height;
    Sint32 mb_width; /* the width in macroblocks */
    Sint32 mb_height; /* the height in macroblocks */
    Sint32 bitrate;
};

/* a handle's playback counts: the pictures decoded and skipped, and
 * MPV_GetDctCnt's two counts after the last picture decoded */
struct sfd_plyinf {
    Sint32 ndec;
    Sint32 nskip;
    Sint32 dctcnt0;
    Sint32 dctcnt1;
};

/* the function a handle calls for each picture decoded or skipped, set through
 * SFD_COND_DEC_PIC_FN and SFD_COND_SKIP_PIC_FN. the video decoder stage passes
 * the picture's MPV_PTYPE_* as arg */
typedef void (*sfd_picfn)(struct sfd_handle *hn, Sint32 arg,
                          struct sfd_plyinf *plyinf);

/* a time source. it stores the handle's time, in units of 1/tunit second, or
 * an SFTIM_* in place of a time */
typedef Sint32 (*sfd_timefn)(struct sfd_handle *hn, Sint32 *time,
                             Sint32 *tunit);
/* the caller's test of a picture, set with SFD_SetUsrIsSkipFn, in place of the
 * video decoder stage's own lateness test. it takes the picture's MPV_PTYPE_*
 * and its time in units of 1/tunit second, and returns whether to skip it */
typedef Sint32 (*sfd_skipfn)(struct sfd_handle *hn, Sint32 type, Sint32 time,
                             Sint32 tunit);

struct sftim_tcode {
    Sint32 rate; /* an index into sftim_prate */
    Sint32 drop; /* whether it drops frames */
    Sint32 hour;
    Sint32 minute;
    Sint32 second;
    Sint32 frame;
    Sint32 nadd; /* frames added to the timecode */
};

struct sftim {
    sfd_timefn timefn; /* the source in use */
    sfd_timefn srcfn[SFTIM_SRC_MAX]; /* each source, by SFTIM_SRC_* */
    sfd_skipfn skipfn;
    struct sftim_tcode tcode;
    Sint32 bound_time;
    Sint32 bound_tunit;
    Sint32 itime;
    Sint32 interval; /* the time between pictures, as SFTIM_UpdateItime finds */
    Sint32 max_delta;
    Sint32 min_delta;
    Sint32 nufrm; /* the frames the caller has reported */
    Sint32 nvsync; /* the vsyncs counted while playing */
    Sint32 tunit;
};

struct sftim_clock {
    Sint32 nvsync;
    Sint32 unk_04;
    Sint32 tunit;
};

/* buffer kinds: a ring of bytes, a queue of pictures, or a ring of samples */
enum sfbuf_type { SFBUF_TYPE_RING, SFBUF_TYPE_VFRM, SFBUF_TYPE_ARING };

/* a ring of bytes, read from rp up to wp. margin bytes follow end, and the
 * delimiter table follows them. a stage keeps marks of its own in dlm, dlm_len
 * and dlm_end, through SFBUF_RingGetDlm and SFBUF_RingSetDlm */
struct sfbuf_ring {
    char *top;
    Sint32 size;
    char *end;
    Sint32 margin;
    char *wp;
    char *rp;
    char *dlm;
    char *dlm_end;
    Sint32 dlm_len;
    Sint32 written;
    Sint32 read;
    Bool closed;
    char *dlm_tbl;
    Sint32 dlm_nword;
    Sint32 dlm_used;
    Sint32 tag;
};

/* a ring's available bytes, free or holding data */
struct sfbuf_ring_inf {
    char *area; /* the run up to the ring's end */
    Sint32 len;
    char *wrap; /* the run from its top */
    Sint32 wraplen;
    Bool closed;
    Sint32 written;
    Sint32 read;
};

/* a decoded picture, as a stage hands it out */
struct sfd_frm {
    Sint32 width;
    Sint32 height;
    Sint32 mb_width; /* the width in macroblocks */
    Sint32 mb_height; /* the height in macroblocks */
    Sint32 type; /* an MPV_PTYPE_* */
    Sint32 time; /* when it shows, in units of 1/tunit second */
    Sint32 tunit;
    Sint32 unk_1c;
    Uint8 *data; /* its macroblocks, MPV_MB_BYTES each */
    Sint32 unk_24;
    Sint32 unk_28;
};

/* a queue slot: stat is TRUE while frm is handed out */
struct sfbuf_vfrm_que {
    Bool stat;
    struct sfd_frm frm;
};

/* the slots of a queue of pictures */
#define SFBUF_NQUE 16

/* a queue of pictures */
struct sfbuf_vfrm {
    char *top;
    Sint32 size;
    Sint32 unk_08;
    Sint32 unk_0c;
    struct sfbuf_vfrm_que que[SFBUF_NQUE];
};

/* a ring of samples, one plane per channel, counted in samples */
struct sfbuf_aring {
    char *top;
    Sint32 size;
    Sint32 sfreq;
    Sint32 nbit;
    Sint32 nch;
    char *plane0;
    char *plane1;
    Sint32 nsmpl;
    Sint32 wpos;
    Sint32 rpos;
    Sint32 written;
    Sint32 read;
};

/* a sample ring's available samples */
struct sfbuf_aring_inf {
    Sint32 sfreq;
    Sint32 nbit;
    Sint32 nch;
    Sint32 len; /* the run up to the ring's end, in samples */
    Sint32 wraplen; /* the run from its top */
    char *area[2]; /* where len starts, in each channel */
    char *wrap[2]; /* where wraplen starts, in each channel */
    Sint32 written;
    Sint32 read;
};

/* a buffer between two stages */
struct sfbuf {
    Sint32 type; /* an SFBUF_TYPE_* */
    /* FALSE for a buffer of size 0, and after SFBUF_TakeBuf takes the area */
    Bool has_area;
    Bool prep; /* TRUE once its writer has made it ready to read */
    Bool term; /* TRUE once its writer has ended it */
    union {
        struct sfbuf_ring ring;
        struct sfbuf_vfrm vfrm;
        struct sfbuf_aring aring;
    } u;
    Sint32 wtrno; /* the stage that writes it */
    Sint32 rtrno; /* the stage that reads it */
};

/* the ADX decoder's ring and player, as SFD_SetAdxtPara sets them for the next
 * handle */
struct sfadxt_para {
    Sint32 rbufsize;
    Sint32 flapsize; /* the extra bytes after the ring */
    char *rbuf;
    Sint32 nch;
    Sint32 sfreq;
    Sint32 worksize;
    char *work; /* the player's work area, of worksize bytes */
};

/* the ADX decoder stage's state */
struct sfadxt {
    struct sfadxt_para para;
    ADXT adxt;
    SJ sj; /* the joint the player reads */
    /* the time the player last gave, in units of 1/tunit second */
    Sint32 time;
    Sint32 tunit;
};

/* the video decoder's frames, as SFD_SetMpvPara sets them for the next handle:
 * in rfb, two reference frames of two c_width by c_height chroma planes and a
 * y_width by y_height luma plane each; in frm, nfrm pictures of frm_width by
 * frm_height pixels, at 12 bits a pixel */
struct sfmpv_para {
    Sint32 c_width;
    Sint32 c_height;
    Sint32 y_width;
    Sint32 y_height;
    Uint8 *rfb;
    Sint32 frm_width;
    Sint32 frm_height;
    Sint32 nfrm; /* at most SFMPV_FRM_MAX */
    Uint8 *frm;
};

#define SFMPV_FRM_MAX 16

/* a picture's states: free, being decoded, and decoded */
enum sfmpv_frm_stat { SFMPV_FRM_FREE, SFMPV_FRM_DEC, SFMPV_FRM_READY };

/* a picture in frm */
struct sfmpv_frm {
    Sint32 stat; /* an SFMPV_FRM_* */
    Uint8 *data;
    struct mpv_picture picture;
};

/* the pictures that lack a reference since a reference picture was skipped:
 * P and B pictures, B pictures, or none */
#define SFMPV_DEFECT_PB 2
#define SFMPV_DEFECT_B 3
#define SFMPV_DEFECT_NONE 5

/* the video decoder stage's state */
struct sfmpv {
    struct sfmpv_para para;
    struct mpv_obj *dec;
    struct sfmpv_frm *held; /* the picture handed out, until it is released */
    Sint32 defect; /* an SFMPV_DEFECT_* */
    Bool term; /* the stream has ended: no picture follows */
    /* TRUE from a picture skipped as late until the next picture decodes.
     * while it is TRUE, no picture is judged late */
    Bool late;
    /* the indexes in rfb of the frames a decode request takes as its first and
     * its second. each I or P picture swaps them */
    Sint32 first;
    Sint32 second;
    struct mpv_frame rfb[2];
    struct sfmpv_frm frm[SFMPV_FRM_MAX];
};

/* a stage's outputs, obuf's length. a stage writes its one output at obuf[0],
 * but the system stage writes three, at SFD_SYS_VIDEO, SFD_SYS_AUDIO and
 * SFD_SYS_USER */
#define SFTRN_NOUT 3
#define SFD_SYS_VIDEO 0
#define SFD_SYS_AUDIO 1
#define SFD_SYS_USER 2

/* a stage of a handle: its state, the functions it runs, and the buffers it
 * reads and writes. a buffer is an SFD_BUF_*, or SFD_BUF_MAX for none */
struct sfd_trn {
    Sint32 unk_00[2];
    Bool prep; /* TRUE once the stage is ready */
    Bool term; /* TRUE once the stage has ended */
    union {
        struct mps_handle *mps;
        struct sfadxt adxt;
        struct sfmpv mpv;
    } u;
    const struct sfd_trif *trif;
    Sint32 ibuf; /* the buffer it reads */
    Sint32 obuf[SFTRN_NOUT]; /* the buffers it writes */
};

/* the tables of a handle's buffers, indexed by SFD_BUF_*, and of its stages,
 * indexed by SFD_TR_*. SFBUF_InitHn takes the buffers' table whole */
struct sfbuf_tbl {
    struct sfbuf buf[SFD_BUF_MAX];
};
struct sftrn_tbl {
    struct sfd_trn trn[SFD_TR_MAX];
};

/* a handle: one movie playing */
struct sfd_handle {
    struct sfd_create_param prm;
    void *work;
    Sint32 stat;
    Sint32 req;
    struct sfd_mvinf mvinf;
    struct sfd_plyinf plyinf;
    struct sfd_err err;
    Sint32 cond[SFD_COND_MAX];
    struct sftim tim;
    struct sfbuf_tbl buftbl;
    struct sftrn_tbl trntbl;
};

/* SFD_Init's parameters */
struct sfd_config {
    struct sfd_trlist *trlist;
    struct sfd_handle *hn;
    Sint32 nhn;
    Sint32 tunit;
};

/* the buffers' library record, which holds nothing */
struct sfbuf_work {
    Sint32 unk_00;
};

/* the library's record, SFLIB_libwork */
struct sflib_work {
    char ver[32];
    char copyright[32];
    struct sfd_config config;
    /* the SFD_SVR_* SFPLY_DecideSvrStat last returned */
    /* @hack type: svrstat as Uint32, whose 0 sflib_InitLibWork loads into R3
     * of its own (MOV #0,R3 at 0x8ced7e42). as Sint32, the 0 shares R12 with
     * the handle loop, sflib_InitLibWork runs 4 bytes long, and 64 of 89 words
     * differ */
    Uint32 svrstat;
    struct sfd_err err;
    struct sftim_clock clock;
    struct sfbuf_work buf;
    struct sfd_trlist trlist;
    Sint32 nhn;
    struct sfd_handle *hn;
};

/* SFD_GetSvrStat values: no handle in use, at least one still running, or every
 * handle in use ended or failed */
#define SFD_SVR_IDLE 0
#define SFD_SVR_BUSY 1
#define SFD_SVR_FIN 2

extern struct sflib_work SFLIB_libwork;

/* the stage function tables this player provides */
extern const struct sfd_trif SFD_tr_in_mem;
extern const struct sfd_trif SFD_tr_sd_mps;
extern const struct sfd_trif SFD_tr_ad_adxt;
extern const struct sfd_trif SFD_tr_vd_mpv;
extern const struct sfd_trif SFD_tr_vo_manu;
extern const struct sfd_trif SFD_tr_ao_auto_p;

/* the library's setup and shutdown, and its errors (sfd_lib.c) */
Sint32 SFD_Init(struct sfd_config *config);
Sint32 SFD_Finish(void);
void SFLIB_InitErrInf(struct sfd_err *err);
/* record ecode as hn's last error, or as the library's when hn is NULL, and
 * pass a nonzero ecode to that record's error function. returns ecode. a
 * positive hn->stat is negated, whatever ecode is */
Sint32 SFLIB_SetErr(struct sfd_handle *hn, Sint32 ecode);
Sint32 SFD_SetErrFn(struct sfd_handle *hn, sfd_errfn func, void *obj);
Sint32 SFD_GetErrInf(struct sfd_handle *hn, struct sfd_err *err);
/* 0 for a handle in use, and -1 for NULL or a free handle */
Sint32 SFLIB_CheckHn(struct sfd_handle *hn);

/* the handles' server, and each handle's creation, playback and input
 * (sfd_ply.c) */
void SFPLY_Init(void);
void SFD_VbIn(void);
void SFD_VbOut(void);
Sint32 SFD_ExecServer(void);
Sint32 SFPLY_DecideSvrStat(void);
struct sfd_handle *SFD_Create(struct sfd_create_param *prm, void *work);
void SFPLY_AddDecPic(struct sfd_handle *hn, Sint32 npic, Sint32 arg);
void SFPLY_AddSkipPic(struct sfd_handle *hn, Sint32 npic, Sint32 arg);
Sint32 SFD_Destroy(struct sfd_handle *hn);
Sint32 SFD_Start(struct sfd_handle *hn);
Sint32 SFD_Stop(struct sfd_handle *hn);
Sint32 SFD_GetWritePtr(struct sfd_handle *hn, struct sfbuf_ring_inf *inf);
Sint32 SFD_AddWritePtr(struct sfd_handle *hn, Sint32 len, Sint32 written);
Sint32 SFD_TermInput(struct sfd_handle *hn);
Sint32 SFD_GetFrm(struct sfd_handle *hn, struct sfd_frm **frm);
Sint32 SFD_RelFrm(struct sfd_handle *hn, struct sfd_frm *frm);

/* the server's and each handle's state, and each handle's conditions and
 * information, as callers read and set them (sfd_set.c) */
Sint32 SFD_GetSvrStat(void);
Sint32 SFD_GetHnStat(struct sfd_handle *hn);
Sint32 SFD_SetCond(struct sfd_handle *hn, Sint32 no, Sint32 val);
void SFSET_SetCond(struct sfd_handle *hn, Sint32 no, Sint32 val);
Sint32 SFD_GetCond(struct sfd_handle *hn, Sint32 no, Sint32 *val);
Sint32 SFSET_GetCond(struct sfd_handle *hn, Sint32 no);
Sint32 SFD_GetMvInf(struct sfd_handle *hn, struct sfd_mvinf *mvinf);
Sint32 SFD_GetPlyInf(struct sfd_handle *hn, struct sfd_plyinf *plyinf);
Sint32 SFD_SetAutoVideo(struct sfd_handle *hn, Bool on);
Sint32 SFD_SetAutoAudio(struct sfd_handle *hn, Bool on);

/* the time sources, the vsync count and timecodes (sfd_tim.c) */
void SFTIM_Init(struct sftim_clock *clock, Sint32 tunit);
void SFTIM_Finish(struct sftim_clock *clock);
void SFTIM_InitHn(struct sfd_handle *hn, struct sftim *tim);
void SFTIM_UpdateItime(struct sftim *tim, Sint32 itime);
Sint32 SFTIM_GetNextItime(struct sftim *tim);
void SFTIM_CntupVbIn(void);
Sint32 SFD_GetTime(struct sfd_handle *hn, Sint32 *time, Sint32 *tunit);
Sint32 SFTIM_GetTime(struct sfd_handle *hn, Sint32 *time, Sint32 *tunit);
Bool SFTIM_ChkRegularTime(struct sfd_handle *hn, Sint32 *time, Sint32 *tunit);
Sint32 SFD_OutUsrFrmSync(struct sfd_handle *hn);
Sint32 SFD_SetUsrIsSkipFn(struct sfd_handle *hn, sfd_skipfn func);
Bool SFD_IsDispTime(Sint32 time1, Sint32 tunit1, Sint32 time2, Sint32 tunit2);
Sint32 SFD_SetUsrTimeFn(struct sfd_handle *hn, sfd_timefn func);
void SFTIM_SetTimeFn(struct sfd_handle *hn, sfd_timefn func, Sint32 src);
void SFTIM_SetMasterTimeFn(struct sfd_handle *hn);
void SFTIM_Tc2Time(struct sftim_tcode *tcode, Sint32 *time, Sint32 *tunit);

/* the buffers between the stages (sfd_buf.c) */
void SFBUF_Init(struct sfbuf_work *work);
void SFBUF_Finish(struct sfbuf_work *work);
void SFBUF_InitHn(struct sfd_handle *hn, struct sfbuf_tbl *buftbl,
                  struct sfd_create_param *prm);
void SFBUF_FixAringBuf(struct sfd_handle *hn, Sint32 bufno, Sint32 nbit,
                       Sint32 nch, Sint32 sfreq);
Sint32 SFBUF_RingGetWrite(struct sfd_handle *hn, Sint32 bufno,
                          struct sfbuf_ring_inf *inf);
Sint32 SFBUF_RingAddWrite(struct sfd_handle *hn, Sint32 bufno, Sint32 len,
                          Sint32 written);
Sint32 SFBUF_RingGetRead(struct sfd_handle *hn, Sint32 bufno,
                         struct sfbuf_ring_inf *inf);
Sint32 SFBUF_RingAddRead(struct sfd_handle *hn, Sint32 bufno, Sint32 len);
void SFBUF_RingGetDlm(struct sfd_handle *hn, Sint32 bufno, char **dlm,
                      Sint32 *dlm_len, char **dlm_end, Sint32 *margin);
void SFBUF_RingSetDlm(struct sfd_handle *hn, Sint32 bufno, char *dlm,
                      Sint32 dlm_len, char *dlm_end);
Sint32 SFBUF_AringGetWrite(struct sfd_handle *hn, Sint32 bufno,
                           struct sfbuf_aring_inf *inf);
Sint32 SFBUF_AringAddWrite(struct sfd_handle *hn, Sint32 bufno, Sint32 nsmpl);
Sint32 SFBUF_AringGetRead(struct sfd_handle *hn, Sint32 bufno,
                          struct sfbuf_aring_inf *inf);
Sint32 SFBUF_AringAddRead(struct sfd_handle *hn, Sint32 bufno, Sint32 nsmpl);
Sint32 SFBUF_VfrmGetWrite(struct sfd_handle *hn, Sint32 bufno, Sint32 arg);
Sint32 SFBUF_VfrmAddWrite(struct sfd_handle *hn, Sint32 bufno, Sint32 arg);
Sint32 SFBUF_VfrmGetRead(struct sfd_handle *hn, Sint32 bufno, Sint32 arg);
Sint32 SFBUF_VfrmAddRead(struct sfd_handle *hn, Sint32 bufno, Sint32 arg);
void SFBUF_TakeBuf(struct sfd_handle *hn, Sint32 bufno, char **area,
                   Sint32 *size);
void SFBUF_SetPrepFlg(struct sfd_handle *hn, Sint32 bufno, Bool flag);
Bool SFBUF_GetPrepFlg(struct sfd_handle *hn, Sint32 bufno);
void SFBUF_SetTermFlg(struct sfd_handle *hn, Sint32 bufno, Bool flag);
Bool SFBUF_GetTermFlg(struct sfd_handle *hn, Sint32 bufno);

/* the stages, called through their function tables (sfd_trn.c) */
Sint32 SFTRN_Init(struct sfd_trlist *trlist, struct sfd_trlist *src);
Sint32 SFTRN_Finish(struct sfd_trlist *trlist);
Sint32 SFTRN_InitHn(struct sfd_handle *hn, struct sfd_trn *trn,
                    struct sfd_create_param *prm, void *work);
Sint32 SFTRN_CallTrSetup(struct sfd_handle *hn, Sint32 slot);
Sint32 SFTRN_CallTrtTrif(struct sfd_handle *hn, Sint32 trno, Sint32 slot,
                         Sint32 arg1, Sint32 arg2);
void SFTRN_SetPrepFlg(struct sfd_handle *hn, Sint32 trno, Bool flag);
Bool SFTRN_GetPrepFlg(struct sfd_handle *hn, Sint32 trno);
void SFTRN_SetTermFlg(struct sfd_handle *hn, Sint32 trno, Bool flag);
Bool SFTRN_GetTermFlg(struct sfd_handle *hn, Sint32 trno);

/* set the ADX decoder stage's parameters for the next handle created. rbuf
 * and work are rounded up to a 32-byte boundary, and their sizes kept. so the
 * ring's area needs up to 31 bytes past rbufsize + flapsize, and the player's
 * up to 31 past worksize */
void SFD_SetAdxtPara(struct sfadxt_para *para);

/* the video decoder stage's parameters for the next handle created, and its
 * decoder's conditions (sfd_mpv.c) */
void SFD_SetMpvPara(struct sfmpv_para *para);
Sint32 SFD_SetMpvCond(struct sfd_handle *hn, Sint32 no, Sint32 val);

/* UTY_MemcpyWord copies n 16-bit words from src to dst, and UTY_MemcpyByte n
 * bytes */
void UTY_MemcpyWord(Uint16 *dst, const Uint16 *src, Uint32 n);
void UTY_MemcpyByte(Uint8 *dst, const Uint8 *src, Uint32 n);

/* a 64-bit number as two words, high word first */
struct uty_uint64 {
    Uint32 hi;
    Uint32 lo;
};
struct uty_sint64 {
    Sint32 hi;
    Uint32 lo;
};

/* store the signed 64-bit product of a and b in *prod */
void UTY_Dmuls(Sint32 a, Sint32 b, struct uty_sint64 *prod);
/* UTY_CmpEq*, UTY_CmpLe* and UTY_CmpLt* return TRUE when a equals b, is at
 * most b, and is less than b. where the high words are equal and negative,
 * UTY_CmpLeSint64 and UTY_CmpLtSint64 compare the low words the other way
 * round. their result is then wrong where the low words differ, so that
 * UTY_CmpLeSint64 finds -1 at most -2 */
Bool UTY_CmpEqUint64(struct uty_uint64 *a, struct uty_uint64 *b);
Bool UTY_CmpEqSint64(struct uty_sint64 *a, struct uty_sint64 *b);
Bool UTY_CmpLeUint64(struct uty_uint64 *a, struct uty_uint64 *b);
Bool UTY_CmpLeSint64(struct uty_sint64 *a, struct uty_sint64 *b);
Bool UTY_CmpLtUint64(struct uty_uint64 *a, struct uty_uint64 *b);
Bool UTY_CmpLtSint64(struct uty_sint64 *a, struct uty_sint64 *b);
/* UTY_CmpPlSint64 returns TRUE when a is above 0, and UTY_CmpPzSint64 when a
 * is at or above 0 */
Bool UTY_CmpPlSint64(struct uty_sint64 *a);
Bool UTY_CmpPzSint64(struct uty_sint64 *a);
/* TRUE when time1, in units of 1/tunit1 second, is at or before time2, in units
 * of 1/tunit2 second. UTY_CmpLeSint64 compares time1 * tunit2 with
 * time2 * tunit1. the result is then wrong where both products are negative
 * with equal high words and unequal low words */
Bool UTY_CmpTime(Sint32 time1, Sint32 tunit1, Sint32 time2, Sint32 tunit2);

#endif
