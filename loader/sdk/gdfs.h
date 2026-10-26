/* SDK 1.20J GD file system records, and the calls the library's members
 * share. unlike 1.30J's, it has no tray state, no wait value for a busy
 * request and no GDFS_ACT_STOP */
#ifndef LOADER_SDK_GDFS_H
#define LOADER_SDK_GDFS_H
#include <sg_ov.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* the GD file system's ID in the overlay library */
#define GDFS_LID                                                               \
    {0xac19e50a,                                                               \
     0x0797,                                                                   \
     0x11d2,                                                                   \
     {0xac, 0xd3, 0x00, 0x10, 0x4b, 0x2a, 0xe8, 0xd5}}

typedef struct GDS_FS_HANDLE GDFS_HANDLE;
typedef struct GDS_FS_WORK GDFS_WORK;
typedef struct GDS_DCF GD_DCF;

/* a file handle, or the system handle, which the library's own requests
 * use. a GDFS_ACT_DAPOS request keeps a pointer in fid, and a
 * GDFS_ACT_SUBCODE request keeps one in fad */
struct GDS_FS_HANDLE {
    GDFS_WORK *wk;
    Sint32 fid; /* the file's entry number in its directory, from 1, or 0 */
    Sint32 fad; /* the file's first frame address */
    Sint32 fsize; /* bytes */
    Sint32 fsctsize; /* sectors, rounded up from fsize */
    Sint32 ofs; /* the next sector to read, counted from fad */
    Sint32 trnsed; /* bytes the request has transferred */
    Sint32 rsize; /* sectors the read requested */
    Sint32 trsize; /* bytes gdFsTrans32 requested */
    /* the read end, transfer end and error callbacks, each with the argument
     * it is passed first */
    GDFS_FUNC rdendcb;
    void *rdcb_1st;
    GDFS_FUNC trendcb;
    void *trcb_1st;
    GDFS_ERRFUNC errcb;
    void *errcb_1st;
    Sint32 gdchn; /* the request's number from the drive, 0 for none */
    Sint32 ex_errcode; /* the drive's extended error code for err */
    Sint16 act; /* the request type, a GDFS_ACT_ value */
    Sint16 trflag; /* the transfer state, a GDD_FS_TRANS_ value */
    Sint16 used; /* whether the handle is open */
    Sint16 tmode; /* the transfer mode, a GDD_FS_TMODE_ value */
    Sint16 stat; /* the request state, a GDD_STAT_ value */
    Sint16 err; /* the last failure's GDD_ERR_ code */
};

/* request types, in act */
#define GDFS_ACT_NONE 0
#define GDFS_ACT_READ 1
#define GDFS_ACT_SEEK 2
#define GDFS_ACT_GDREAD 4 /* a read the drive holds for gdFsTrans32 */
#define GDFS_ACT_CMD 6
#define GDFS_ACT_SUBCODE 7 /* a subcode request, decoded into a GDFS_DAINFO */
#define GDFS_ACT_DAPOS 8 /* a CD-DA command, then a head position request */
#define GDFS_ACT_DRVSTAT 10

/* a directory entry as the library keeps it */
typedef struct GDS_FS_DIRREC_ENT {
    Sint32 sector; /* the first ISO 9660 sector, 150 below its frame address */
    Sint32 fsize; /* bytes */
    Uint8 flag; /* GDD_FF_ bits */
    Uint8 sid; /* the name's search ID, which a search compares first */
    Sint8 fname[GDD_FS_FNAMESIZE + 1];
    Sint8 pad[2];
} GDFS_DIRREC_ENT;

/* a directory as the library keeps it */
typedef struct GDS_FS_DIRREC_TBL {
    Sint32 dir_num; /* the entries it holds */
    Sint32 max_ent; /* the entries it has room for */
    Sint32 dir_fad; /* the frame address it was read from, 0 for none */
    Sint32 pad;
    GDFS_DIRREC_ENT dirrec_tbl[1]; /* max_ent entries, declared as one */
} GDFS_DIRREC_TBL;

/* indexes into get_cmd_stat's info. REQ_TRNSED receives the bytes a running
 * or complete request has moved, and REQ_ERR, the same word, a failed
 * request's GDD_ERR_ code. REQ_EX_ERRCODE receives a failed request's
 * extended error code */
#define REQ_TRNSED 0
#define REQ_ERR 0
#define REQ_EX_ERRCODE 1
/* indexes into get_drv_stat's info: the drive state, and the disc type,
 * which is DISC_GDROM for a GD-ROM */
#define DRV_STAT 0
#define DRV_DISC 1
#define DISC_GDROM 0x80

/* the drive functions, through the boot ROM. every Sint32 member but trans,
 * get_cmd_stat and check_trans issues a drive command. each returns the
 * command's request number, or 0 when the drive refuses it */
struct GDS_DCF {
    Bool (*init_system)(void);
    /* for a tmode other than GDD_FS_TMODE_CPU and GDD_FS_TMODE_DMA, read
     * issues nothing and returns GDD_ERR_ILLTMODE */
    Sint32 (*read)(Sint32 fad, Sint32 nsct, void *buf, Sint32 tmode);
    Sint32 (*req_gd_read)(Sint32 fad, Sint32 nsct);
    Sint32 (*trans)(Sint32 gdchn, void *buf, Sint32 nbytes);
    /* a request's state, a GDD_STAT_ value, with info filled at the REQ_
     * indexes. an aborted request reads as GDD_STAT_BUSY */
    Sint32 (*get_cmd_stat)(Sint32 gdchn, Sint32 *info);
    void (*read_abort)(Sint32 gdchn);
    Sint32 (*seek)(Sint32 fad);
    Sint32 (*init_drive)(void);
    /* FALSE while the drive is busy. otherwise it fills info at the DRV_
     * indexes */
    Bool (*get_drv_stat)(Sint32 *info);
    Sint32 (*check_trans)(Sint32 gdchn, Sint32 *size);
    Sint32 (*da_play)(Sint32 st_track, Sint32 end_track, Sint32 reptime);
    Sint32 (*da_play_sct)(Sint32 st_sct, Sint32 end_sct, Sint32 reptime);
    Sint32 (*da_pause)(void);
    Sint32 (*da_release)(Sint32 reptime);
    Sint32 (*get_toc)(Sint32 type, void *buf);
    Sint32 (*get_scd)(Sint32 type, Sint32 size, void *buf);
    Bool (*exec_server)(void);
    void (*reset)(void);
    /* request the drive state, and the track, frame address and index under the
     * head */
    Sint32 (*get_pos)(Sint32 *stat, Sint32 *track, Sint32 *fad, Sint32 *index);
    Sint32 (*get_ver)(char *ver);
};

/* the library work. syshdl points to the system handle, hndlist[0], and
 * hndtbl to the max_open file handles that follow it */
struct GDS_FS_WORK {
    Sint32 max_open;
    Sint32 pathtbl_fad; /* the path table's frame address */
    Sint32 pathtbl_size; /* bytes */
    const GD_DCF *dcf;
    GDFS_DIRREC curdir; /* the current directory */
    Sint32 errstat; /* the global error, a GDD_ERR_ code */
    Bool f_init; /* set once the drive has started */
    volatile Bool f_svr; /* set while gdFsGetStatG runs */
    volatile GDFS now_work; /* the handle whose request is running, or NULL */
    GDFS syshdl;
    GDFS hndtbl;
    /* the global error callback, and the argument it is passed first */
    GDFS_ERRFUNC g_errcb;
    void *g_errcb_1st;
    Bool daplayed; /* a CD-DA play is kept for gdFsDaReleaseG to start again */
    /* the drive version, a digit to a byte: 1.10 is 0x10100. 0 when the drive
     * doesn't give it */
    Sint32 gdc_ver;
    Sint32 pad;
    /* the buffer for the library's own reads, and for a subcode request's
     * data */
    Uint32 sctbuf[((GDD_FS_SCTSIZE * 2) + 64) / 4];
    GDFS_HANDLE hndlist[1];
};

/* the head position, from a frame's subcode Q channel, as gdDecSubcode
 * gives it. minute, second and frame are the time within the track */
struct subcode_position {
    Uint32 track;
    Uint32 minute;
    Uint32 second;
    Uint32 frame;
    /* the control byte in bits 24 to 31, above the absolute time in frames */
    Uint32 disc_frame;
    Uint32 index;
};

/* the library's interface, which ovLibGetLibIf returns. each member past
 * the first three serves the gdFs call it is named for. gdg_fs_lif_tbl
 * leaves the first three NULL */
struct gdfs_lif {
    void (*QueryInterface)();
    void (*AddRef)();
    void (*Release)();
    Sint32 (*Init)(Sint32 max_open, void *gdfs_work, Sint32 max_dirent,
                   void *dirbuf);
    Sint32 (*Reinit)(void);
    void (*Finish)(void);
    Sint32 (*SetGlobalErr)(Sint32 err);
    Sint32 (*ReadW)(Sint32 fad, Sint32 nsct, void *buf);
    Sint32 (*LoadDir)(const char *dirname, GDFS_DIRREC gf_dirrec);
    Sint32 (*LoadDirFast)(const char *dirname, GDFS_DIRREC gf_dirrec);
    Sint32 (*SetDir)(GDFS_DIRREC gf_dirrec);
    Sint32 (*GetDirInfo)(const char *name, GDFS_DIRINFO *dirinfo);
    GDFS (*GetWorkHn)(void);
    GDFS (*GetSysHn)(void);
    GDFS (*CreateHn)(GDFS gdfs, Sint32 fid, Sint32 fad, Sint32 fsize);
    GDFS (*Open)(const char *fname, GDFS_DIRREC gf_dirrec);
    GDFS (*OpenFast)(const char *fname, GDFS_DIRREC gf_dirrec);
    GDFS (*OpenRange)(Sint32 stsct, Sint32 nsct);
    void (*ExecServer)(void);
    Sint32 (*GetGlobalErr)(void);
    Sint32 (*GetToc)(Sint32 type, void *buf);
    Sint32 (*GetDrvStat)(void);
    Sint32 (*ReqDrvStat)(void);
    Sint32 (*DaPlay)(Sint32 st_track, Sint32 end_track, Sint32 reptime);
    Sint32 (*DaPlaySct)(Sint32 st_sct, Sint32 end_sct, Sint32 reptime);
    Sint32 (*DaStop)(void);
    Sint32 (*DaPause)(void);
    Sint32 (*DaRelease)(Sint32 reptime);
    Sint32 (*DaGetInfo)(GDFS_DAINFO *dainfo);
    void (*EntryErrFuncAll)(GDFS_ERRFUNC erfunc, void *obj);
};

/* the library work: gdFsInitG's gdfs_work, or NULL before it is set and
 * after gdFsFinishG */
extern GDFS_WORK *gd_wk;
/* the library's handle, which main.c puts in the library table */
extern OVS_LIB_OBJ gdg_fs_libhdl;
/* vertical blanks since the library last started the drive, by which
 * gdFsWaitCompleteReq times its wait */
extern volatile Uint32 gdg_fs_vcnt;

/* calls through the library interface that shinobi.h doesn't declare. as with
 * those it lists, each returns 0 or NULL while the library is closed */

/* read nsct sectors from frame address fad into buf through the system
 * handle, and wait for them. it returns GDD_ERR_BUSY while the system handle's
 * request is running, and otherwise gdFsRead's result */
Sint32 gdFsReadW(Sint32 fad, Sint32 nsct, void *buf);
Sint32 gdFsSetGlobalErr(Sint32 err);
Sint32 gdFsGetGlobalErr(void);
/* open the closed handle gdfs on fsize bytes from frame address fad, as entry
 * fid, with no callbacks and the default transfer mode. it returns gdfs, or
 * NULL when gdfs is open */
GDFS gdFsCreateHn(GDFS gdfs, Sint32 fid, Sint32 fad, Sint32 fsize);
Sint32 gdFsLoadDirFast(const char *dirname, GDFS_DIRREC gf_dirrec);
GDFS gdFsOpenFast(const char *fname, GDFS_DIRREC gf_dirrec);

/* the functions behind the interface, which gdg_fs_lif_tbl holds. each has
 * the contract of the call it serves. each but gdFsInitG needs gd_wk, and only
 * gdFsReinitG checks it */
Sint32 gdFsInitG(Sint32 max_open, void *gdfs_work, Sint32 max_dirent,
                 void *dirbuf);
Sint32 gdFsReinitG(void);
void gdFsFinishG(void);
Sint32 gdFsSetGlobalErrG(Sint32 err);
Sint32 gdFsReadWG(Sint32 fad, Sint32 nsct, void *buf);
Sint32 gdFsLoadDirG(const char *dirname, GDFS_DIRREC gf_dirrec);
Sint32 gdFsLoadDirFastG(const char *dirname, GDFS_DIRREC gf_dirrec);
Sint32 gdFsSetDirG(GDFS_DIRREC gf_dirrec);
Sint32 gdFsGetDirInfoG(const char *name, GDFS_DIRINFO *dirinfo);
GDFS gdFsGetWorkHnG(void);
GDFS gdFsGetSysHnG(void);
GDFS gdFsCreateHnG(GDFS gdfs, Sint32 fid, Sint32 fad, Sint32 fsize);
GDFS gdFsOpenG(const char *fname, GDFS_DIRREC gf_dirrec);
GDFS gdFsOpenFastG(const char *fname, GDFS_DIRREC gf_dirrec);
GDFS gdFsOpenRangeG(Sint32 stsct, Sint32 nsct);
void gdFsExecServerG(void);
Sint32 gdFsGetGlobalErrG(void);
Sint32 gdFsGetTocG(Sint32 type, void *buf);
Sint32 gdFsGetDrvStatG(void);
Sint32 gdFsReqDrvStatG(void);
Sint32 gdFsDaPlayG(Sint32 st_track, Sint32 end_track, Sint32 reptime);
Sint32 gdFsDaPlaySctG(Sint32 st_sct, Sint32 end_sct, Sint32 reptime);
Sint32 gdFsDaStopG(void);
Sint32 gdFsDaPauseG(void);
Sint32 gdFsDaReleaseG(Sint32 reptime);
Sint32 gdFsDaGetInfoG(GDFS_DAINFO *dainfo);
void gdFsEntryErrFuncAllG(GDFS_ERRFUNC erfunc, void *obj);
/* the server's step: poll the drive about now_work's request, and bring the
 * handle up to date. it returns the request state get_cmd_stat gives. once a
 * GDFS_ACT_DAPOS request completes, it returns the head position request's
 * number instead, or 0 when the drive refuses it */
Sint32 gdFsGetStatG(void);
/* the little-endian 32-bit value at p, as ISO 9660 records hold them */
Sint32 gdFsGetLE4(const Uint8 *p);

/* reset gdfs for a new request on fsize bytes from frame address fad, as
 * entry fid, and mark it open. its callbacks and transfer mode stay. it
 * returns gdfs, or NULL when gdfs is NULL or its request is running */
GDFS gdFsReopenHn(GDFS gdfs, Sint32 fid, Sint32 fad, Sint32 fsize);
/* run the server until gdfs's request ends, and return its result. gdfs must
 * not be NULL. it returns GDD_ERR_OK once the request completes, and gdfs->err
 * once it fails. it returns GDD_ERR_ILLREQ for an idle handle, and
 * GDD_ERR_FATAL for a closed one or any other state. it returns GDD_ERR_TOUT
 * after 1200 vertical blanks while gdfs->trnsed keeps the value it had when
 * the wait began. once trnsed changes, the wait has no limit */
Sint32 gdFsWaitCompleteReq(GDFS gdfs);
Sint32 gdFsSetTmode(GDFS gdfs, Sint32 tmode);

/* copy size bytes from src to dst, one at a time. unlike memcpy, src comes
 * first */
void gdFsMemcpy(const void *src, void *dst, Sint32 size);
/* the position of name in gf_dirrec, from 1, or 0. case matters only when exact
 * is TRUE */
Sint32 gdFsSearchName(const char *name, GDFS_DIRREC gf_dirrec, Bool exact);
/* read the directory at frame address fad into gf_dirrec, a sector at a time
 * through buf. size need cover only the first sector: the directory's first
 * record, its own, gives the size the read then uses. it returns GDD_ERR_OK at
 * once when gf_dirrec was read from fad already. it returns gdFsReadW's error
 * when a read fails. it returns GDD_ERR_DIROVER when the table fills before
 * the directory's last sector, or with a record still left in it. both
 * failures leave dir_fad 0 */
Sint32 gdFsGetDirEnt(Uint8 *buf, Sint32 fad, GDFS_DIRREC gf_dirrec,
                     Sint32 size);
/* the ISO 9660 sector where path's directory starts, or 0 when no directory
 * matches or a read fails. path starts with a slash or backslash, which the
 * search skips. slashes and backslashes separate its names, which match
 * whatever their case. the path table is size bytes at frame address fad, and
 * is read a sector at a time into buf. a record that runs past a sector's end
 * is joined in the bytes before buf, which must have room for one */
Sint32 gdFsSearchPath(Uint8 *buf, Sint32 fad, Sint32 size, const char *path);

const GD_DCF *gdFsGetGdDcf(void);
/* decode the head position from a frame's whole subcode into position. it
 * returns 0, or a negative value when the subcode holds no valid position */
Sint32 gdDecSubcode(Sint8 *subcode, struct subcode_position *position);

#endif
