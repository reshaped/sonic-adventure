/* the MPEG system stream parser's records and interfaces */
#ifndef MOVIE_SDK_MPS_H
#define MOVIE_SDK_MPS_H
#include <sg_xpt.h>

/* the start code kinds MPSDEL_CheckDelim returns, as bits, which MPS_DecHd
 * ORs into *dlm. a code it doesn't know is MPS_DLM_NONE, and MPS_DLM_END is
 * the stream's end code */
#define MPS_DLM_NONE 0
#define MPS_DLM_PACK 0x10000
#define MPS_DLM_SYS 0x20000
#define MPS_DLM_PKET 0x40000
#define MPS_DLM_END 0x80000

/* the error a call returns for a handle not in use, after it records the
 * error as the library's. MPS_SetErrFn returns MPS_ERR_SETERRFN, and
 * MPS_GetErrInf returns MPS_ERR_GETERRINF. every other call that takes a
 * handle returns MPS_ERR_HN, for a null handle too */
#define MPS_ERR_SETERRFN ((Sint32)0xff020101)
#define MPS_ERR_GETERRINF ((Sint32)0xff020102)
#define MPS_ERR_HN ((Sint32)0xff020103)

/* a header field's value until a header sets it, and a time stamp's when the
 * packet has none */
#define MPS_UNREAD (-1)

/* a handle's states: free, or in use */
#define MPS_STAT_FREE 1
#define MPS_STAT_USED 2

typedef void (*mps_err_func)(void *obj, Sint32 code);

/* an error function and its object, and the last error code */
struct mps_err {
    mps_err_func func;
    void *obj;
    Sint32 code;
};

struct mps_pack_hd {
    /* TRUE when the pack's first two bits are 00, as an MPEG-1 pack's are */
    Sint32 mpeg1;
    /* the 33-bit system clock reference, in 90 kHz ticks, divided by 4, which
     * keeps it a positive Sint32 */
    Sint32 scr;
    Sint32 mux_rate; /* the top 15 of the mux rate's 22 bits */
};

struct mps_sys_hd {
    Sint32 len; /* the bytes after the length field */
    Sint32 rate_bound;
    Sint32 audio_bound;
    Sint32 video_bound;
    Sint32 fixed;
    Sint32 csps;
    Sint32 audio_lock;
    Sint32 video_lock;
};

/* packet stream kinds: audio, video, private, padding, or another */
#define MPS_PKET_AUDIO 0
#define MPS_PKET_VIDEO 1
#define MPS_PKET_PRIVATE 2
#define MPS_PKET_PADDING 3
#define MPS_PKET_OTHER 4

/* a packet header. buf_size keeps the last value a packet gave, and a padding
 * or private stream 2 packet leaves pts and dts as they were too */
struct mps_pket_hd {
    Sint32 stream_id;
    Sint32 kind; /* the stream's kind, an MPS_PKET_* */
    /* the stream's number within its kind: from 0 for audio and video, 1 and 2
     * for the private streams 0xbd and 0xbf, and else 0 */
    Sint32 number;
    Sint32 len; /* the bytes after the length field */
    Sint32 buf_size; /* the buffer size, in bytes */
    /* the presentation and decoding time stamps, in 90 kHz ticks, divided by
     * 4 as scr is */
    Sint32 pts;
    Sint32 dts;
    Sint32 data_len; /* the data bytes after the header */
};

/* sys_by_kind's indexes: a system header that bounds audio streams, one that
 * bounds video streams but no audio, and one that bounds neither */
#define MPS_SYS_AUDIO 0
#define MPS_SYS_VIDEO 1
#define MPS_SYS_OTHER 2
#define MPS_NSYS 3

/* a parser handle, with the headers it read last */
struct mps_handle {
    Sint32 stat; /* an MPS_STAT_* */
    struct mps_err err;
    struct mps_pack_hd pack;
    struct mps_sys_hd sys;
    /* the last system header of each MPS_SYS_* kind */
    struct mps_sys_hd sys_by_kind[MPS_NSYS];
    struct mps_pket_hd pket;
};

/* the caller's work, as the library lays it out. it must hold nhn handles */
struct mps_lib_work {
    char version[32];
    char copyright[32];
    struct mps_err err; /* the library's own */
    Sint32 nhn;
    /* the first of nhn handles, the rest past the struct's end */
    struct mps_handle hn[1];
};

/* lay the library out in work, with nhn handles, all free. return 0 */
Sint32 MPS_Init(Sint32 nhn, void *work);
/* destroy every handle in use */
void MPS_Finish(void);
/* send mps's errors, or the library's when mps is NULL, to func with obj.
 * return 0 */
Sint32 MPS_SetErrFn(struct mps_handle *mps, mps_err_func func, void *obj);
/* copy mps's error function, object and last code, or the library's when mps
 * is NULL, to *err. return 0 */
Sint32 MPS_GetErrInf(struct mps_handle *mps, struct mps_err *err);
/* return a free handle, now in use with every header field MPS_UNREAD, or NULL
 * when none is free */
struct mps_handle *MPS_Create(void);
/* free mps. return 0 */
Sint32 MPS_Destroy(struct mps_handle *mps);

/* read the headers at buf until a packet's, which it reads too. an end code,
 * an unknown code, or fewer than 4 of nbyte's bytes left stops the read sooner.
 * *used gets the bytes the headers take, and *dlm the MPS_DLM_* bits of their
 * start codes. a system header is also kept in sys_by_kind. each header must
 * lie whole in buf. return 0 */
Sint32 MPS_DecHd(struct mps_handle *mps, Uint8 *buf, Sint32 nbyte, Sint32 *used,
                 Sint32 *dlm);
/* copy the pack header, the system header of an MPS_SYS_* kind, or the packet
 * header that mps read last. return 0 */
Sint32 MPS_GetPackHd(struct mps_handle *mps, struct mps_pack_hd *pack);
Sint32 MPS_GetSysHd(struct mps_handle *mps, struct mps_sys_hd *sys,
                    Sint32 kind);
Sint32 MPS_GetPketHd(struct mps_handle *mps, struct mps_pket_hd *pket);

/* the calls between the parser's own sources */
void MPSDEC_Init(void);
void MPSDEC_Finish(void);
void MPSGET_Init(void);
void MPSGET_Finish(void);
void MPSLIB_InitErrInf(struct mps_err *err);
/* record code as mps's error, or the library's when mps is NULL, pass a
 * nonzero code to its error function, and return code */
Sint32 MPSLIB_SetErr(struct mps_handle *mps, Sint32 code);
/* return 0 for a handle in use, and -1 for a null or free one */
Sint32 MPSLIB_CheckHn(struct mps_handle *mps);
/* the MPS_DLM_* kind of the start code in the four bytes at buf */
Sint32 MPSDEL_CheckDelim(Uint8 *buf);

#endif
