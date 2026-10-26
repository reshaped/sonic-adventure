/* GSC, the middleware's streams from disc files into joints: its records
 * and interfaces */
#ifndef MOVIE_SDK_GSC_H
#define MOVIE_SDK_GSC_H
#include <sg_xpt.h>
#include <shinobi.h>
#include <sj.h>

#define GSC_ERR_STAT (-2) /* a stream in the wrong state for the call */
#define GSC_ERR_RANGE (-3) /* an argument out of range */
#define GSC_ERR_CHAIN (-10) /* the vertical blank chain took no server */
#define GSC_ERR_NOLIB (-11) /* the overlay library hasn't opened GSC */
#define GSC_ERR_NHN (-12) /* a stream count below 1 */
#define GSC_ERR_NOHN (-13) /* every stream in use */
#define GSC_ERR_HN (-14) /* a null stream */
#define GSC_ERR_SJ (-15) /* a null joint */
/* a negative joint size. getSjBufSize tests for one, but its queries return
 * GSC_ERR_SJ_SIZE first, so no call returns this */
#define GSC_ERR_BUFSIZE (-16)
#define GSC_ERR_SHORT (-17) /* a transfer short of its sectors */
#define GSC_ERR_STALL (-19) /* a read that stalls while a stop waits on it */

/* a stream's states: stopped, reading, and read to its end */
#define GSC_STAT_STOP 1
#define GSC_STAT_EXEC 2
#define GSC_STAT_END 3

/* the end offset that reads to the stream's end */
#define GSC_OFST_END 0x7fffffff

/* the alignment, in bytes, of the stream table mwGscInitG lays out in its
 * work */
#define GSC_WORK_ALIGN 32

/* the library's UUID, by which the overlay library knows it */
#define GSC_LID                                                                \
    {0x6c6af8c0,                                                               \
     0x01f9,                                                                   \
     0x11d2,                                                                   \
     {0xa5, 0xa0, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}}

/* a stream: a file on the disc, which the server reads into the joint sj */
struct gsc_stream {
    Bool used;
    Sint32 stat;
    /* the joint's size when the stream opened */
    Sint32 bufsize;
    /* the flow limits on the joint's data, in bytes. a read fills the joint
     * up to flow_high. a stream paused for room resumes once the joint drains
     * to flow_low */
    Sint32 flow_low;
    Sint32 flow_high;
    /* the read position, in bytes from the stream's start */
    Sint32 cur_ofst;
    Sint32 stm_size; /* the stream's size, in bytes */
    /* the most sectors one read asks for, and one transfer moves */
    Sint32 rd_sct;
    Sint32 tr_sct;
    /* the range mwGscSetOfstRange sets, in bytes. reading stops at ofst_end.
     * nothing reads ofst_start: reading goes on from cur_ofst */
    Sint32 ofst_start;
    Sint32 ofst_end;
    Sint32 unk_2c;
    Bool pause;
    Sint32 unk_34;
    Sint32 unk_38;
    SJ sj;
    GDFS gdfs;
};

/* the library's streams, and its read in flight, which fills a chunk of one
 * stream's joint a transfer at a time */
struct gsc_work {
    Sint32 nhn;
    struct gsc_stream *hn;
    Bool reading; /* a transfer is in flight */
    Sint32 req_sct; /* the read's size, in sectors */
    Sint32 rd_sct; /* the transfer's size, in sectors */
    Sint32 tr_byte; /* the bytes the read has transferred */
    Sint32 num_rd;
    Uint32 rd_tmr;
    Sint32 unk_20;
    Sint32 cur; /* the index in hn of the stream the read is for */
    SJCK ck; /* the chunk the read fills */
    void (*err_func)(void *obj, Sint32 code);
    void *err_obj;
    Sint32 err_code;
};

/* the library interface the overlay library returns */
struct gsc_lif {
    void (*QueryInterface)();
    void (*AddRef)();
    void (*Release)();
    Sint32 (*Init)(Sint32 nhn, void *work);
    Sint32 (*Finish)(void);
    void (*ExecServer)(void);
    struct gsc_stream *(*OpenFname)(char *fname, SJ sj);
    struct gsc_stream *(*OpenRange)(Sint32 stsct, Sint32 nsct, SJ sj);
    Sint32 (*Close)(struct gsc_stream *hn);
};

/* the library interface. mwGscInitOvl gets it from the overlay library, and
 * mwGscInit sets it to this library's own */
extern struct gsc_lif *mwg_gsc_i;

/* open the library through the overlay library, and init it through its
 * interface, whose Init lays out nhn streams in work as mwGscInitG does.
 * return 0 at once if the overlay library opened it already, GSC_ERR_NOLIB if
 * it can't, and else Init's result */
Sint32 mwGscInitOvl(Sint32 nhn, void *work);
/* finish the library through its interface, and close it in the overlay
 * library. return GSC_ERR_NOLIB unless mwGscInitOvl opened it, even after
 * mwGscInit. return Finish's error, with the library left open, or 0 */
Sint32 mwGscFinish(void);
/* the calls through mwg_gsc_i. while it is null, mwGscExecServer does
 * nothing, the opens return NULL, and mwGscClose returns GSC_ERR_NOLIB */
void mwGscExecServer(void);
struct gsc_stream *mwGscOpenFname(char *fname, SJ sj);
struct gsc_stream *mwGscOpenRange(Sint32 stsct, Sint32 nsct, SJ sj);
Sint32 mwGscClose(struct gsc_stream *hn);

/* the library's own functions, called without the overlay library */
/* point mwg_gsc_i at this library's interface, and init it as mwGscInitG
 * does */
Sint32 mwGscInit(Sint32 nhn, void *work);
/* stop the server's current stream, then reset each open stream's joint,
 * close the stream and clear the table. return 0, or the stop's error before
 * any close */
Sint32 mwGscReset(void);
/* lay out a table of nhn streams, all closed, in work, which must hold
 * GSC_WORK_ALIGN - 1 + nhn * sizeof(struct gsc_stream) bytes. the table starts
 * at work rounded up to a multiple of GSC_WORK_ALIGN. the clear starts at work
 * itself, so the bytes the rounding adds at the table's end stay as they were.
 * return 0, or GSC_ERR_NHN for nhn below 1 */
Sint32 mwGscInitG(Sint32 nhn, void *work);
Sint32 mwGscFinishG(void);
void mwGscExecServerG(void);
struct gsc_stream *mwGscOpenFnameG(char *fname, SJ sj);
struct gsc_stream *mwGscOpenRangeG(Sint32 stsct, Sint32 nsct, SJ sj);
Sint32 mwGscCloseG(struct gsc_stream *hn);
Sint32 mwGscStart(struct gsc_stream *hn);
Sint32 mwGscStop(struct gsc_stream *hn);
/* seek to ofst sectors from the start, from the read position, or from the end,
 * as whence says */
Sint32 mwGscSeekSct(struct gsc_stream *hn, Sint32 ofst, Sint32 whence);
void mwGscEntryErrFunc(void (*func)(void *obj, Sint32 code), void *obj);

Sint32 mwGscGetStat(struct gsc_stream *hn, Sint32 *stat);
Sint32 mwGscGetCurOfst(struct gsc_stream *hn, Sint32 *ofst);
Sint32 mwGscGetStmSize(struct gsc_stream *hn, Sint32 *size);
SJ mwGscGetSjHn(struct gsc_stream *hn);
Sint32 mwGscGetFlowLimit(struct gsc_stream *hn, Sint32 *low, Sint32 *high);
Sint32 mwGscGetRdSct(struct gsc_stream *hn, Sint32 *nsct);
Sint32 mwGscGetTrSct(struct gsc_stream *hn, Sint32 *nsct);
/* set the flow limits. low and high must each lie from 0 to bufsize, the
 * joint's size, with low no higher than high, or the call returns
 * GSC_ERR_RANGE */
Sint32 mwGscSetFlowLimit(struct gsc_stream *hn, Sint32 low, Sint32 high);
/* set rd_sct. nsct must lie from 0 to the whole sectors bufsize holds, or the
 * call returns GSC_ERR_RANGE */
Sint32 mwGscSetRdSct(struct gsc_stream *hn, Sint32 nsct);
/* set tr_sct. nsct must lie from 0 to rd_sct, or the call returns
 * GSC_ERR_RANGE */
Sint32 mwGscSetTrSct(struct gsc_stream *hn, Sint32 nsct);
/* set the range to ofst through end, in bytes, with GSC_OFST_END for the
 * stream's end. only end bounds the read, which goes on from cur_ofst. return
 * 0, GSC_ERR_HN for a null hn, or GSC_ERR_RANGE unless 0 <= ofst <= end <= the
 * stream's size */
Sint32 mwGscSetOfstRange(struct gsc_stream *hn, Sint32 ofst, Sint32 end);
GDFS mwGscGetGdfsHn(struct gsc_stream *hn);

/* the disc side of a stream: open a file by name or by sector range, and
 * store its size in bytes in *fsize */
GDFS mwGscGdOpenFname(char *fname, Sint32 *fsize);
GDFS mwGscGdOpenRange(Sint32 stsct, Sint32 nsct, Sint32 *fsize);
Sint32 mwGscGdClose(struct gsc_stream *hn);
/* return GSC_ERR_GD_HN for a stream whose file handle is null, and else 0.
 * the file's read goes on */
Sint32 mwGscGdStop(struct gsc_stream *hn);
Sint32 mwGscGdReqRd(struct gsc_stream *hn, Sint32 nsct, void *buf);
/* store the bytes the read has transferred in *total, and those since last
 * in *nbyte */
Sint32 mwGscGdGetNumRd(struct gsc_stream *hn, Sint32 last, Sint32 *nbyte,
                       Sint32 *total);
Sint32 mwGscGdGetGdfsStat(struct gsc_stream *hn, Sint32 *stat);
/* TRUE when hn's file handle reports its last request complete, and else
 * FALSE. hn->gdfs must be open: with it null, the result comes from an unset
 * word */
Bool mwGscGdIsComplete(struct gsc_stream *hn);
Sint32 mwGscGdSeek(struct gsc_stream *hn, Sint32 sctno);
void mwGscGdExecServer(void);

#endif
