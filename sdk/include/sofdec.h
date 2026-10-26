/* CRI's Sofdec movie and ADX sound libraries, as far as the sources use
 * them */
#ifndef SOFDEC_H
#define SOFDEC_H
#include <sg_mwply.h>
#include <sg_xpt.h>
#include <sj.h>

/* the records mwPlyInitSofdec and mwPlyCreateSofdec take, and the options in
 * the second. mwPlyInitSofdec reads none of MWS_PLY_INIT_SFD's fields. of
 * MWS_PLY_CPRM_SFD's fields, only ftype, dtype, work and wksize take effect */
typedef struct {
    Sint32 ftype;
    Sint32 hnmax;
    Sint32 vsync_mhz;
    Sint32 disp_latency;
} MWS_PLY_INIT_SFD;
typedef struct {
    Bool first_hpel;
} MWS_PLY_OPT_SFD;
typedef struct {
    MWS_PLY_OPT_SFD opt;
    Sint32 ftype;
    Sint32 dtype;
    Sint32 lx;
    Sint32 ly;
    Sint32 sx;
    Sint32 sy;
    Sint8 *work;
    Sint32 wksize;
} MWS_PLY_CPRM_SFD;
/* the ftype of a system stream, its video and audio multiplexed, and the dtype
 * that has the renderer place the movie by its size */
#define MWD_PLY_FTYPE_SFD 1
#define MWD_PLY_DTYPE_AUTO 0

void mwPlyPreInitSofdec(void);
/* a null iprm sets the display latency to 2 frames and the frame rate to
 * 29.97 Hz, the values the player starts with. a record leaves both as they
 * are, whatever it holds. the call invalidates the operand cache and sets CCR
 * to 0x185, which turns both caches on and makes every area copy-back. of the
 * two caches, only the operand cache is in index mode, and its RAM mode is
 * off. the call makes the caller a fiber, and replaces Kamui's wait-vsync
 * callback with a switch to the decoding fiber */
void mwPlyInitSofdec(MWS_PLY_INIT_SFD *iprm);
/* it invalidates the operand cache and sets CCR to 0x105, which takes the
 * operand cache out of index mode. it clears Kamui's wait-vsync callback */
void mwPlyFinishSofdec(void);
/* a player for cprm, which it copies. cprm->work, of cprm->wksize bytes, must
 * hold 0x2f1160 bytes and outlive the handle. it returns NULL when all 32
 * players are in use, or when Sofdec or its stream joint can't be made. a
 * failed joint leaves the new Sofdec handle undestroyed. the player's audio
 * is ADX talk's first stream if that stream is in use at the call. without
 * it, the volume and pan calls pass ADX talk a null handle */
MWPLY mwPlyCreateSofdec(MWS_PLY_CPRM_SFD *cprm);
/* sw is 0 or 1, unchecked. 1 makes a prediction whose vector has both
 * half-pel offsets from two samples, not four */
void mwPlySetFastHalfpel(MWPLY mwply, Sint32 sw);
/* the movie's position and size on the screen. each call keeps its values
 * for the renderers the player makes, and sets them on the renderer it has.
 * mwPlySetDispPos doesn't test for a renderer: before the movie's size is
 * known, and after a stop or a start, it stores through a null one */
void mwPlySetDispPos(MWPLY mwply, Float lx, Float ly);
void mwPlySetDispSize(MWPLY mwply, Float sx, Float sy);
/* the movie's brightness and its offset, which set the base and offset colors
 * of its vertices. each takes val, 0 to 255, as its red, green and blue. the
 * player keeps val for the renderers it makes, and the getters return it */
void mwPlySetBright(MWPLY mwply, Sint32 val);
void mwPlySetBrightOfst(MWPLY mwply, Sint32 val);
Sint32 mwPlyGetBright(MWPLY mwply);
Sint32 mwPlyGetBrightOfst(MWPLY mwply);

/* the stack that exceptions and interrupts run on, and its size in words.
 * mwPlyPreInitSofdec hands its end to syHwInit, and mwPlyInitSofdec hands it
 * to the fiber switches made from an interrupt */
#define MWSFD_INT_STACK_WORDS 2048
extern Uint32 mwsfdg_intStack[MWSFD_INT_STACK_WORDS];

/* ADXT_Init initializes ADX talk and the parts under it, and clears every
 * stream. ADXT_Reset is ADXT_Init again */
void ADXT_Init(void);
void ADXT_Reset(void);
void ADXT_Finish(void);
void ADXT_ExecServer(void);
/* a flag of 1, the default, makes ADXT_Init add ADXT_ExecServer to the
 * vertical blank chain. any other flag leaves the server to the caller */
void ADXT_ConfigVsyncSvr(Sint32 flag);

/* the handles of a stream's disc reader, stream decoder and sound output.
 * adx_sjd.c defines ADXSJD as a pointer to its record before it includes this
 * header */
typedef void *ADXSTM;
#ifndef ADXSJD_DEFINED
#define ADXSJD_DEFINED
typedef void *ADXSJD;
#endif
typedef void *ADXRNA;

/* the number of ADX talk's streams, and the most channels a stream has */
#define ADXT_MAX_OBJ 8
#define ADXT_MAX_NCH 2

/* a stream that ADX talk plays */
typedef struct _adx_talk {
    Sint8 used;
    Sint8 stat;
    Sint8 pmode;
    Sint8 maxnch;
    ADXSJD sjd;
    ADXSTM stm;
    ADXRNA rna;
    SJ sjf;
    SJ sji;
    SJ sjo[ADXT_MAX_NCH];
    Sint8 *ibuf;
    Sint32 ibuflen;
    Sint32 ibufxlen;
    Sint16 *obuf;
    Sint32 obufsize;
    Sint32 obufdist;
    Sint32 svrfreq;
    Sint16 maxsct;
    Sint16 minsct;
    Sint16 outvol;
    Sint16 outpan[ADXT_MAX_NCH];
    Sint32 maxdecsmpl;
    Sint32 lpcnt;
    Sint32 lp_skiplen;
    Sint32 trp;
    Sint32 wpos;
    Sint32 mofst;
    Sint16 ercode;
    Sint32 edecpos;
    Sint16 edeccnt;
    Sint16 eshrtcnt;
    Sint8 lpflg;
    Sint8 autorcvr;
    Sint8 fltmode;
    Sint8 execflag;
    Sint8 pstwait_flag;
    Sint8 pstready_flag;
} ADX_TALK;
typedef ADX_TALK *ADXT;
extern ADX_TALK adxt_obj[ADXT_MAX_OBJ];

/* ADXT_SetAutoRcvr's recoveries from an error: stop the stream, or restart
 * it, a disc stream from its first sector. a stuck sound stops the stream
 * under either. ADXT_RMODE_STOP is the default */
#define ADXT_RMODE_STOP 1
#define ADXT_RMODE_REPLAY 2

/* ADXT_GetStat's states */
#define ADXT_STAT_STOP 0 /* stopped */
#define ADXT_STAT_DECINFO 1 /* reading the stream's header */
#define ADXT_STAT_PREP 2 /* decoding, before it plays */
#define ADXT_STAT_PLAYING 3 /* playing */
#define ADXT_STAT_DECEND 4 /* decoded to the end, and still playing */
#define ADXT_STAT_PLAYEND 5 /* played to the end */

/* ADXT_GetErrCode's codes: none, the input short of data, and the sound
 * stuck */
#define ADXT_ERR_OK 0
#define ADXT_ERR_SHRTBUF (-1)
#define ADXT_ERR_SNDBLK (-2)
/* the play mode of a stream that ADXT_StartMem or ADXT_StartMemIdx starts
 * from memory */
#define ADXT_PMODE_MEM 2
/* the recovery that does nothing, and leaves the error code set */
#define ADXT_RMODE_NOACT 0
/* ADXT_SetOutPan's pans, from hard left to hard right */
#define ADXT_PAN_LEFT (-15)
#define ADXT_PAN_CENTER 0
#define ADXT_PAN_RIGHT 15
/* the pan that centers a mono stream and pans a stereo one hard left and
 * right */
#define ADXT_PAN_AUTO (-128)
/* a stereo stream's channels, as ADXT_SetOutPan takes them */
#define ADXT_CH_L 0
#define ADXT_CH_R 1
/* a stream's input is short below ADXT_MIN_BUFDATA bytes. a playing stream
 * reports ADXT_ERR_SHRTBUF once its input has been short for more than
 * svrfreq server calls. one not yet playing waits ADXT_EWAIT_NOTPLY times as
 * long */
#define ADXT_MIN_BUFDATA 64
#define ADXT_EWAIT_NOTPLY 5

/* a stream over work, which it borrows until ADXT_Destroy. work holds, from
 * its first 32-byte boundary, 16448 bytes of output for each channel. the
 * input ring takes the rest, less 36 bytes, in whole 2048-byte sectors.
 * maxnch is 1 or 2. it returns NULL when all ADXT_MAX_OBJ streams are in use,
 * or a part can't be made. when a joint or the stream decoder can't be made,
 * its cleanup first reads and writes through NULL handles, as the @bug in
 * adx_tlk.c says */
ADXT ADXT_Create(Sint32 maxnch, void *work, Sint32 worksize);
void ADXT_Destroy(ADXT adxt);
void ADXT_StartAfs(ADXT adxt, Sint32 patid, Sint32 fid);
void ADXT_StartFname(ADXT adxt, char *fname);
/* borrows sj until Stop, another start, or Destroy */
void ADXT_StartSj(ADXT adxt, SJ sj);
void ADXT_StartMem(ADXT adxt, void *adxdat);
/* play file no of the index at acx, in memory. a no outside the index only
 * stops the stream */
void ADXT_StartMemIdx(ADXT adxt, void *acx, Sint32 no);
void ADXT_Stop(ADXT adxt);
Sint32 ADXT_GetStat(ADXT adxt);
void ADXT_GetTime(ADXT adxt, Sint32 *ncount, Sint32 *tscale);
void ADXT_SetOutVol(ADXT adxt, Sint32 vol);
void ADXT_SetSvrFreq(ADXT adxt, Sint32 freq);
void ADXT_SetAutoRcvr(ADXT adxt, Sint32 rmode);
void ADXT_EntryErrFunc(void (*func)(void *obj, char *msg), void *obj);
void ADXT_ExecHndl(ADXT adxt);
Sint32 ADXT_GetNumSmpl(ADXT adxt);
Sint32 ADXT_GetSfreq(ADXT adxt);
/* ch must be 0 or 1. the pan is stored before ch is checked against the
 * stream's channels, and a ch past them only reports an error */
void ADXT_SetOutPan(ADXT adxt, Sint32 ch, Sint32 pan);
Sint32 ADXT_GetOutPan(ADXT adxt, Sint32 ch);
Sint32 ADXT_GetOutVol(ADXT adxt);
void ADXT_SetReloadSct(ADXT adxt, Sint32 minsct);
/* requires a current input joint */
Sint32 ADXT_GetNumSctIbuf(ADXT adxt);
Sint32 ADXT_IsIbufSafety(ADXT adxt);
Sint32 ADXT_IsCompleted(ADXT adxt);
Sint32 ADXT_GetErrCode(ADXT adxt);
void ADXT_ClearErrCode(ADXT adxt);
Sint32 ADXT_GetLpCnt(ADXT adxt);
void ADXT_SetLpFlg(ADXT adxt, Sint32 flg);
SJ ADXT_GetInputSj(ADXT adxt);
void ADXT_SetWaitPlayStart(ADXT adxt, Sint32 flg);
Sint32 ADXT_IsReadyPlayStart(ADXT adxt);
void ADXT_SetFx(ADXT adxt, Sint32 fxch, Sint32 fxlvl);
void ADXT_GetFx(ADXT adxt, Sint32 *fxch, Sint32 *fxlvl);
void ADXT_SetFilter(ADXT adxt, Sint32 coff, Sint32 q);
void ADXT_GetFilter(ADXT adxt, Sint32 *coff, Sint32 *q);
void ADXT_SetTranspose(ADXT adxt, Sint32 transps, Sint32 detune);
void ADXT_GetTranspose(ADXT adxt, Sint32 *transps, Sint32 *detune);
/* start decoding adxt's stream from the input joint sji, or from the ring its
 * disc stream fills. the ADXT_Start calls and the replay recovery use them */
void adxt_start_sj(ADXT adxt, SJ sji);
void adxt_start_stm(ADXT adxt);

void ADXF_ExecServer(void);
/* initialize the ADX file reader */
void ADXF_Init(void);
/* read the file table of the AFS archive fname into ptinfo, as partition
 * ptid. a stream can then start a file in it by number. the table holds
 * nmax files at most. ptid must be 0 to 255. ptinfo must hold 24 bytes and 2
 * for each file, and stays in use until ADXF_Init, ADXF_Finish or the next
 * load of ptid. returns 0 once the table is read. returns -3 for a bad ptid
 * or a null ptinfo, and -1 when the archive can't be opened or its table
 * read. returns -4 when the archive's header lists over 65536 files */
Sint32 ADXF_LoadPartition(Sint32 ptid, char *fname, void *ptinfo, Sint32 nmax);
/* store file flid's start frame address in *fad and its size in sectors in
 * *fnsct, and return 0. the entry comes from the newest archive added to
 * partition ptid that has one for flid, or else from the loaded archive.
 * a ptid of 256 or more, or a flid at or past the newest archive's file count,
 * sets both outputs to -1 and returns -3. ptid must not be
 * negative, and below 256 it must hold a partition. flid must not be
 * negative. a flid no added archive has must be below the loaded archive's
 * file count, or the call reads past that archive's table */
Sint32 ADXF_GetFileRange(Sint32 ptid, Sint32 flid, Sint32 *fad, Sint32 *fnsct);

/* 1 once an expander decodes an odd number of stereo blocks. the library
 * never clears it */
extern Sint32 adxpd_internal_error;

/* the library's critical sections */
void ADXCRS_Init(void);
void ADXCRS_Lock(void);
void ADXCRS_Unlock(void);
/* the stereo block decoder. ibuf's blocks alternate, left first, and nblk
 * counts both channels'. it returns the blocks decoded before one whose
 * scale ends the stream, and then leaves delayl and delayr as they were.
 * otherwise it returns nblk, though an odd nblk leaves its last block
 * undecoded. a sample past 16 bits comes out as 32767 or -32768, since FTRC,
 * the conversion to int, saturates. the call's later samples are predicted
 * from the sample's unclamped value, but delayl and delayr get the last two
 * samples saturated */
Sint32 ADX_DecodeSteFloat(Sint8 *ibuf, Sint32 nblk, Sint16 *obufl,
                          Sint16 *delayl, Sint16 *obufr, Sint16 *delayr,
                          Sint16 k0, Sint16 k1);
/* the float decoder's step for each 4-bit code: the code read as signed,
 * times 65536. AdxQtblFloat holds it by code, AdxQtblFloat0 by byte for the
 * byte's high code, and AdxQtblFloat1 for its low code */
extern const Float AdxQtblFloat[16];
extern const Float AdxQtblFloat0[256];
extern const Float AdxQtblFloat1[256];
/* the mono block decoder, which returns the blocks it decoded */
Sint32 ADX_DecodeMono2(Sint8 *ibuf, Sint32 nblk, Sint16 *obuf, Sint16 *delay,
                       Sint16 k0, Sint16 k1);
/* the predictor's two coefficients for a filter that cuts off at cutoff hertz
 * at sample rate sfreq. k0 weighs the last sample and k1 the one before, both
 * in units of 1/4096 */
void ADX_GetCoefficient(Sint32 cutoff, Sint32 sfreq, Sint16 *k0, Sint16 *k1);
/* ADX header readers, of the header of hdrsize bytes at hdr, whose fields are
 * big-endian. each returns -1 for an hdrsize too short for the fields it
 * reads, and else -2 for a header that doesn't start with the halfword
 * 0x8000 */

/* read the format, and in *dtofs the data's offset. returns 0, or -1 for an
 * hdrsize below 16. a failure writes no output */
Sint32 ADX_DecodeInfo(const Sint8 *hdr, Sint32 hdrsize, Sint16 *dtofs,
                      Sint8 *enc, Sint8 *bps, Sint8 *blklen, Sint8 *nch,
                      Sint32 *sfreq, Sint32 *total, Sint32 *blksmpl);
/* read the filter's cutoff into *cof. returns 0, or -1 for an hdrsize below
 * 18 or data that starts within those bytes. a failure leaves *cof as it
 * was */
Sint32 ADX_DecodeInfoExADPCM2(const Sint8 *hdr, Sint32 hdrsize, Sint16 *cof);
/* read *ins_nsmpl and *nloop, and when *nloop is 1, the loop's type and its
 * start and end as samples and as byte offsets. returns 0 for a loop, or -1
 * for an hdrsize below 48 or data that starts within those bytes. it also
 * returns -2 when *nloop isn't 1, as for a header without a loop. after the
 * other failures, *nloop is 0 */
Sint32 ADX_DecodeInfoExLoop(const Sint8 *hdr, Sint32 hdrsize, Sint32 *ins_nsmpl,
                            Sint16 *nloop, Sint16 *type, Sint32 *start_pos,
                            Sint32 *start_ofst, Sint32 *end_pos,
                            Sint32 *end_ofst);
/* SPSD header readers. info is the header, from the "SPSD" that starts it:
 * offsets count from there, and its words are little-endian */

/* returns info, and *hdsize gets the header's size: where its data starts */
void *nlsndStreamGetHdInfo(void *info, Sint32 *hdsize);
/* nlsndStreamGetParam's parameter numbers. from NLSND_CH_BYTE_1 on, a name
 * says where its parameter is read, not what it holds */
#define NLSND_SFREQ 0 /* the sample rate */
#define NLSND_BPS 1 /* bits per sample: 16, 8 or 4 */
#define NLSND_NSMPL 2 /* samples in each channel */
#define NLSND_DTLEN 3 /* the data's length in bytes */
#define NLSND_NCH 4 /* channels: 1, 2 or 4 */
/* byte 1 or 2 of the four at 0x30 + 4 * ch */
#define NLSND_CH_BYTE_1 5
#define NLSND_CH_BYTE_2 6
/* the low or high nibble of byte 0x33, channel 0's fourth */
#define NLSND_CH0_BYTE_3_LOW 7
#define NLSND_CH0_BYTE_3_HIGH 8
/* 0 if byte 0x1f is nonzero, else the halfword at 0x16 divided by 64, at
 * least 1 */
#define NLSND_UNK_16 9
#define NLSND_UNK_1E 10 /* byte 0x1e */
/* parameter no of the header, or -1 for any other no. info must be 4-byte
 * aligned. NLSND_BPS and NLSND_NSMPL require a sample format, byte 8, of 0 to
 * 3. NLSND_NSMPL and NLSND_NCH require a channel code, byte 9's low two bits,
 * of 0 to 2. ch matters only to NLSND_CH_BYTE_1 and NLSND_CH_BYTE_2 */
Sint32 nlsndStreamGetParam(void *info, Sint32 no, Sint16 ch);

/* ADX expanders, which decode blocks of ADX data into samples, by handle. an
 * expander is stopped, started, decoding, or done */
typedef struct adxpd_obj *ADXPD;
#define ADXPD_STAT_STOP 0
#define ADXPD_STAT_PREP 1
#define ADXPD_STAT_EXEC 2
#define ADXPD_STAT_END 3
void ADXPD_Init(void);
ADXPD ADXPD_Create(void);
void ADXPD_SetCoef(ADXPD xpd, Sint32 sfreq, Sint32 cutoff);
void ADXPD_Destroy(ADXPD xpd);
Sint32 ADXPD_GetStat(ADXPD xpd);
/* give a stopped expander the blocks its next start decodes: nblk blocks of
 * one channel at ibuf, into obuf. returns 1 if it was stopped and took them,
 * else 0 */
Sint32 ADXPD_EntryMono(ADXPD xpd, Sint8 *ibuf, Sint32 nblk, Sint16 *obuf);
/* as ADXPD_EntryMono for two channels, into obufl and obufr. ibuf's blocks
 * alternate, left first, and nblk counts both channels' */
Sint32 ADXPD_EntrySte(ADXPD xpd, Sint8 *ibuf, Sint32 nblk, Sint16 *obufl,
                      Sint16 *obufr);
void ADXPD_Start(ADXPD xpd);
void ADXPD_Stop(ADXPD xpd);
void ADXPD_Reset(ADXPD xpd);
Sint32 ADXPD_GetNumBlk(ADXPD xpd);
void ADXPD_ExecHndl(ADXPD xpd);

/* ADX basic decoders, which read a stream's header and decode its data through
 * an expander into a ring of samples, by handle. a decoder is stopped, started,
 * decoding, or done, and its data is ADX, WAV or SPSD */
typedef struct adxb_obj *ADXB;
#define ADXB_STAT_STOP 0
#define ADXB_STAT_PREP 1
#define ADXB_STAT_EXEC 2
#define ADXB_STAT_END 3
#define ADXB_FORMAT_ADX 0
#define ADXB_FORMAT_WAV 1
#define ADXB_FORMAT_SPSD 2
/* an ADXB_GETWR function returns the buffer a decoder writes. it stores the
 * write position in the ring in *wpos, the room there in *nroom and the
 * samples left before decoding stops in *nleft. the decoder calls an ADXB_ADDWR
 * function once it has written wsize bytes, wnsmpl samples */
typedef Sint16 *(*ADXB_GETWR)(void *obj, Sint32 *wpos, Sint32 *nroom,
                              Sint32 *nleft);
typedef void (*ADXB_ADDWR)(void *obj, Sint32 wsize, Sint32 wnsmpl);
void ADXB_Init(void);
/* NULL when all eight are in use. pcmbuf holds pcmbsize samples of the first
 * channel, and the second channel starts pcmbdist samples on */
ADXB ADXB_Create(Sint32 maxnch, Sint16 *pcmbuf, Sint32 pcmbsize,
                 Sint32 pcmbdist);
void ADXB_Destroy(ADXB adxb);
/* the header's length, or 0 for a header it can't read */
Sint32 ADXB_DecodeHeader(ADXB adxb, Sint8 *hdr, Sint32 size);
void ADXB_EntryGetWrFunc(ADXB adxb, ADXB_GETWR func, void *obj);
Sint16 *ADXB_GetPcmBuf(ADXB adxb);
Sint32 ADXB_GetFormat(ADXB adxb);
Sint32 ADXB_GetSfreq(ADXB adxb);
Sint32 ADXB_GetNumChan(ADXB adxb);
Sint32 ADXB_GetOutBps(ADXB adxb);
Sint32 ADXB_GetBlkSmpl(ADXB adxb);
Sint32 ADXB_GetBlkLen(ADXB adxb);
Sint32 ADXB_GetTotalNumSmpl(ADXB adxb);
Sint32 ADXB_GetCof(ADXB adxb);
Sint32 ADXB_GetLpInsNsmpl(ADXB adxb);
Sint32 ADXB_GetNumLoop(ADXB adxb);
Sint32 ADXB_GetLpStartPos(ADXB adxb);
Sint32 ADXB_GetLpStartOfst(ADXB adxb);
Sint32 ADXB_GetLpEndPos(ADXB adxb);
Sint32 ADXB_GetLpEndOfst(ADXB adxb);
Sint32 ADXB_GetStat(ADXB adxb);
/* the data the next start decodes, ibuflen bytes at ibuf */
void ADXB_EntryData(ADXB adxb, Sint8 *ibuf, Sint32 ibuflen);
void ADXB_Start(ADXB adxb);
void ADXB_Stop(ADXB adxb);
void ADXB_Reset(ADXB adxb);
Sint32 ADXB_GetDecDtLen(ADXB adxb);
Sint32 ADXB_GetDecNumSmpl(ADXB adxb);
void ADXB_ExecHndl(ADXB adxb);

/* ADX stream decoders, each from an input joint through a basic decoder into a
 * joint for each channel. a decoder is stopped, reading its stream's header,
 * decoding, or done with the stream */
#define ADXSJD_STAT_STOP 0
#define ADXSJD_STAT_DECINFO 1
#define ADXSJD_STAT_DECODE 2
#define ADXSJD_STAT_DECEND 3
void ADXSJD_Init(void);
void ADXSJD_Finish(void);
/* create a decoder from sji into the maxnch joints at sjo, or NULL when all
 * eight are in use or ADXB_Create fails. after a failed ADXB_Create, the call
 * has written two words, at addresses 0x88 and 0x8c, as adx_sjd.c's @bug memory
 * says. sjo[0] must be a ring buffer joint, whose buffer the basic decoder
 * writes into. it writes the second channel just past sjo[0]'s ring and its
 * extra bytes, so sjo[1]'s ring must start there */
ADXSJD ADXSJD_Create(SJ sji, Sint32 maxnch, SJ *sjo);
void ADXSJD_Destroy(ADXSJD sjd);
Sint32 ADXSJD_GetStat(ADXSJD sjd);
void ADXSJD_SetInSj(ADXSJD sjd, SJ sji);
void ADXSJD_SetOutSj(ADXSJD sjd, Sint32 ch, SJ sjo);
void ADXSJD_SetMaxDecSmpl(ADXSJD sjd, Sint32 nsmpl);
void ADXSJD_Start(ADXSJD sjd);
void ADXSJD_Stop(ADXSJD sjd);
void ADXSJD_ExecHndl(ADXSJD sjd);
void ADXSJD_ExecServer(void);
Sint32 ADXSJD_GetDecDtLen(ADXSJD sjd);
Sint32 ADXSJD_GetDecNumSmpl(ADXSJD sjd);
void ADXSJD_SetDecPos(ADXSJD sjd, Sint32 pos);
Sint32 ADXSJD_GetDecPos(ADXSJD sjd);
/* call func with obj after each step that leaves the trap count at or past the
 * trap's sample count */
void ADXSJD_EntryTrapFunc(ADXSJD sjd, void (*func)(void *obj), void *obj);
void ADXSJD_SetTrapNumSmpl(ADXSJD sjd, Sint32 nsmpl);
Sint32 ADXSJD_GetTrapNumSmpl(ADXSJD sjd);
void ADXSJD_SetTrapCnt(ADXSJD sjd, Sint32 cnt);
Sint32 ADXSJD_GetTrapCnt(ADXSJD sjd);
void ADXSJD_SetTrapDtLen(ADXSJD sjd, Sint32 len);
Sint32 ADXSJD_GetTrapDtLen(ADXSJD sjd);
Sint32 ADXSJD_GetFormat(ADXSJD sjd);
Sint32 ADXSJD_GetSfreq(ADXSJD sjd);
Sint32 ADXSJD_GetNumChan(ADXSJD sjd);
Sint32 ADXSJD_GetOutBps(ADXSJD sjd);
Sint32 ADXSJD_GetBlkSmpl(ADXSJD sjd);
Sint32 ADXSJD_GetBlkLen(ADXSJD sjd);
Sint32 ADXSJD_GetTotalNumSmpl(ADXSJD sjd);
Sint32 ADXSJD_GetCof(ADXSJD sjd);
Sint32 ADXSJD_GetNumLoop(ADXSJD sjd);
Sint32 ADXSJD_GetLpInsNsmpl(ADXSJD sjd);
Sint32 ADXSJD_GetLpStartPos(ADXSJD sjd);
Sint32 ADXSJD_GetLpStartOfst(ADXSJD sjd);
Sint32 ADXSJD_GetLpEndPos(ADXSJD sjd);
Sint32 ADXSJD_GetLpEndOfst(ADXSJD sjd);
/* an SPSD stream's header, its first 64 bytes */
char *ADXSJD_GetSpsdInfo(ADXSJD sjd);

void ADXERR_Init(void);
void ADXERR_EntryErrFunc(void (*func)(void *obj, char *msg), void *obj);
/* format a message as fmt says and pass it to the function ADXERR_EntryErrFunc
 * set */
void ADXERR_CallErrFunc(char *fmt, ...);

/* streams from the disc. a stream is stopped, reading, read to its end, or
 * failed */
#define ADXSTM_STAT_STOP 1
#define ADXSTM_STAT_READ 2
#define ADXSTM_STAT_READEND 3
#define ADXSTM_STAT_ERROR 4
Sint32 ADXSTM_Init(void);
void ADXSTM_Finish(void);
Sint32 ADXSTM_Start(ADXSTM stm);
void ADXSTM_Close(ADXSTM stm);
void ADXSTM_ExecServer(void);
Sint32 ADXSTM_SetBufSize(ADXSTM stm, Sint32 bufsize, Sint32 rdsize);
void ADXSTM_Stop(ADXSTM stm);
/* seek to sector sct, and return the sector */
Sint32 ADXSTM_Seek(ADXSTM stm, Sint32 sct);
/* the function called at the stream's end, and the sector where that is */
void ADXSTM_EntryEosFunc(ADXSTM stm, void (*func)(void *obj), void *obj);
void ADXSTM_SetEos(ADXSTM stm, Sint32 eossct);
Sint32 ADXSTM_GetStat(ADXSTM stm);
/* open a stream over nsct sectors from frame address fad. NULL when
 * gdFsOpenRange fails, which ADXERR_CallErrFunc reports. when every stream is
 * in use it returns NULL too, but leaves the range open and first writes
 * "OpenRange" at address 0x40, through the null stream (0x8c66e1b2) */
ADXSTM ADXSTM_OpenRange(Sint32 fad, Sint32 nsct, SJ sj);
/* a file's start frame address, and its size in sectors and in bytes */
Sint32 ADXSTM_GetFad(const char *fname, Sint32 *fad);
Sint32 ADXSTM_GetFsizeSct(const char *fname, Sint32 *fsize);
Sint32 ADXSTM_GetFsizeByte(const char *fname, Sint32 *fsize);
/* *ofst gets the current sector less the one at ADXSTM_Start */
Sint32 ADXSTM_GetCurOfst(ADXSTM stm, Sint32 *ofst);

/* sound outputs, each of which plays a stream's channels from the joints at
 * sjo, by handle */
ADXRNA ADXRNA_Create(SJ *sjo, Sint32 nch);
void ADXRNA_Destroy(ADXRNA rna);
void ADXRNA_GetTime(ADXRNA rna, Sint32 *ncount, Sint32 *tscale);
void ADXRNA_ExecServer(void);
void ADXRNA_Init(void);
void ADXRNA_Finish(void);
void ADXRNA_SetTransSw(ADXRNA rna, Sint32 sw);
void ADXRNA_SetPlaySw(ADXRNA rna, Sint32 sw);
Sint32 ADXRNA_GetNumData(ADXRNA rna);
/* returns 0, whatever the output holds */
Sint32 ADXRNA_GetNumRoom(ADXRNA rna);
void ADXRNA_SetNumChan(ADXRNA rna, Sint32 nch);
void ADXRNA_SetSfreq(ADXRNA rna, Sint32 sfreq);
void ADXRNA_SetOutVol(ADXRNA rna, Sint32 vol);
void ADXRNA_SetOutPan(ADXRNA rna, Sint32 ch, Sint32 pan);
void ADXRNA_SetBitPerSmpl(ADXRNA rna, Sint32 bps);
/* does nothing */
void ADXRNA_SetTotalNumSmpl(ADXRNA rna, Sint32 nsmpl);
/* returns an uninitialized word: mwRnaSetStmHdInfo never writes the result it
 * is passed */
Sint32 ADXRNA_SetStmHdInfo(ADXRNA rna, char *hdinfo);

/* Sofdec's own fill and copy of nword 4-byte words */
void UTY_MemsetDword(void *dst, Uint32 value, Uint32 nword);
void UTY_MemcpyDword(void *dst, const void *src, Uint32 nword);
/* UTY_PushFr stores FR12 to FR15 and the second bank within the 84 bytes at a
 * 4-byte aligned save, and UTY_PopFr loads them back */
void UTY_PushFr(void *save);
void UTY_PopFr(void *save);

#endif
