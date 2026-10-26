/* the Sofdec player's record and error codes, and the calls that set up and
 * feed its Sofdec handle */
#ifndef MOVIE_SDK_MWSFD_H
#define MOVIE_SDK_MWSFD_H
#include <sg_mwply.h>
#include <sg_xpt.h>
#include <sj.h>
#include <sofdec.h>

struct sfd_handle;
struct gsc_stream;
struct rnv_obj;

/* a Sofdec player. the handle mwPlyCreateSofdec returns is the address of
 * base, its first member, so a handle casts back to its player */
struct mwsfd_player {
    MW_PLY_OBJ base;
    Bool used;
    MWE_PLY_STAT stat;
    MWS_PLY_CPRM_SFD cprm;
    Bool created;
    struct sfd_handle *sfd;
    /* the reader of the movie's file, from mwSfdStartFname until the player
     * stops, or NULL */
    struct gsc_stream *gsc;
    /* the ring joint gsc reads the file into, which feeds the decoder */
    SJ stream;
    /* the renderer, from when the movie's size is known until the player
     * stops or starts again, or NULL */
    struct rnv_obj *renderer;
    Bool frame_loaded; /* the last display pass gave the renderer a picture */
    Bool decoding; /* the last decoding pass found stat other than stopped */
    Sint32 dtype; /* the MWD_RNV_DTYPE_* placement a new renderer takes */
    Uint8 unk_50[8];
    ADXT adxt; /* ADX talk's stream that plays the movie's audio, or NULL */
    /* the settings a new renderer takes: the brightness and its offset, and
     * once disp_set is TRUE, the position and size */
    Sint32 bright;
    Sint32 bright_ofst;
    Bool disp_set;
    Float disp_lx;
    Float disp_ly;
    Float disp_sx;
    Float disp_sy;
};

/* the stream a file holds: a system stream, which multiplexes video and
 * audio, or a video stream alone */
enum mwsfd_file_type { MWSFD_FILE_SYSTEM = 1, MWSFD_FILE_VIDEO = 2 };

/* the player's error codes, which mwSfdSetErrCode records */
enum mwsfd_error {
    MWSFD_ERR_PARAM = -4,
    MWSFD_ERR_NO_HANDLE = -11,
    MWSFD_ERR_NULL_HANDLE = -12,
    MWSFD_ERR_GSC_INIT = -101,
    MWSFD_ERR_GSC_OPEN = -102,
    MWSFD_ERR_GSC_START = -103,
    MWSFD_ERR_INIT = -301,
    MWSFD_ERR_FILE_TYPE = -302,
    MWSFD_ERR_ERROR_FN = -303,
    MWSFD_ERR_CREATE = -305,
    MWSFD_ERR_DESTROY = -306,
    MWSFD_ERR_START = -307,
    MWSFD_ERR_STOP = -308
};

/* record code as the last error, and return it. a nonzero code also goes to
 * the player's error function, if one is set */
Sint32 mwSfdSetErrCode(Sint32 code);
/* set Sofdec up for a file_type stream, an MWSFD_FILE_*, with mwPlySfdErrFunc
 * as its error function. returns 0, or the MWSFD_ERR_* code it records. ply is
 * unread */
Sint32 mwPlySfdInit(struct mwsfd_player *ply, Sint32 file_type);
Sint32 mwPlySfdFinish(struct mwsfd_player *ply);
/* lay out work for a handle of a file_type stream, an MWSFD_FILE_*, and create
 * the handle. work needs 0x2f1160 bytes, and must outlive the handle and the
 * joint mwPlySfdCreateSj makes in it. a video stream's handle lays its buffers
 * out from address 0, not in work. ply is unread.
 *
 * returns the handle, for the caller to store in ply->sfd. returns NULL, with
 * the error code recorded, when Sofdec can't create the handle or set its
 * error function, and the handle is then left undestroyed. a work_size below
 * 0x2f1160 also returns NULL, after a report through mwMngError that reads
 * work_size as the address of its format */
struct sfd_handle *mwPlySfdCreate(struct mwsfd_player *ply, Sint32 file_type,
                                  char *work, Sint32 work_size);
void mwPlySfdDestroy(struct sfd_handle *sfd);
/* the skip test mwPlySfdSetCond gives a handle. it returns 0 for every
 * picture, so the handle skips none as late */
Sint32 mwSfdIsSkip(struct sfd_handle *handle, Sint32 type, Sint32 time,
                   Sint32 tunit);
/* set the conditions the player gives ply's handle, and give the handle
 * mwSfdIsSkip as its skip test. returns SFD_SetUsrIsSkipFn's result */
Sint32 mwPlySfdSetCond(struct mwsfd_player *ply);
Sint32 mwPlySfdStart(struct mwsfd_player *ply);
Sint32 mwPlySfdStop(struct mwsfd_player *ply);
void mwPlySfdGetWritePtr(struct mwsfd_player *ply, char **area, Sint32 *size,
                         Sint32 *written);
void mwPlySfdAddWritePtr(struct mwsfd_player *ply, Sint32 size, Sint32 written);
/* run Sofdec's server, then create ply's renderer once the movie's size is
 * known and ply has none. the renderer gets no callbacks. the struct rnv_cprm
 * passed to mwRnvCreate has no dtype set, so mwRnvCreate places the movie by
 * a dtype as the stack held it. no code calls this function */
void mwPlySfdExecServer(struct mwsfd_player *ply);
/* 1 once Sofdec's server is no longer busy, and 0 while it is */
Sint32 mwPlySfdIsEndDec(struct mwsfd_player *ply);
void mwPlySfdErrFunc(void *argument, Sint32 code);
void mwPlySfdSjInit(void);
void mwPlySfdSjFinish(void);
/* a ring joint over the stream region of the work the last mwPlySfdCreate
 * laid out, or NULL when every ring joint is in use */
SJ mwPlySfdCreateSj(void);
void mwPlySfdSjDestroy(struct mwsfd_player *ply);
/* copy what ply's stream joint holds into the handle's input ring, from its
 * write position up to the ring's end. the copy is at most that run rounded
 * down to 32 bytes, and Sofdec takes it as written. a player with no handle
 * copies nothing */
void mwPlySfdSjCopyToSfd(struct mwsfd_player *ply);
/* with on 1, set ply's handle to play audio, and give the handles
 * mwPlySfdCreate makes for system streams from now on an ADX decoder stage.
 * with any other value, take both away */
void mwPlySetAudioSw(struct mwsfd_player *ply, Sint32 on);

#endif
