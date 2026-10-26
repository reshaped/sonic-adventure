/* sound ports: stream ports whose channels play ring buffers in sound memory,
 * and the transfers that fill them. a port reports an error by passing a
 * message to mwRnaErrCallFunc (mw_rna.h) */
#ifndef MW_SND_H
#define MW_SND_H
#include <sg_xpt.h>
#include <shinobi.h>

/* a sound port: a stream port of one or two channels. each channel has a sound
 * buffer (mw_sdbuf.h), whose number names its ring buffer bank, and a memory
 * block that transfers into the ring. a call that takes a ch doesn't check it,
 * and ch must name one of the port's channels */
typedef struct MwSnd MwSnd;

/* free all eight ports and the sound buffers. a port still open is dropped
 * without being closed, so close each first */
void mwSndInit(void);
/* open a port of nch channels, 1 or 2. sound buffer n's ring is the 0x4000
 * bytes at sound memory offset 0x200000 - (16 - n) * 0x4040. NULL for another
 * nch, when no port, sound buffer or memory block is free, or when the stream
 * port fails to open. after the last two failures, each memory block the open
 * destroys is reported as a failed sdMemBlkDestroy, whether or not the destroy
 * failed. when the stream port fails, the port's sound buffers also stay
 * taken. in their place, the open passes mwSndBufDestroy the handle the port
 * stores after them, once for each channel. mwSndBufDestroy zeroes the first
 * word that handle points to. with one channel, it is a second buffer handle
 * this open never set. with two, it is the first memory block's handle, after
 * the block is destroyed. mw_sdcs.c's @bug tags show the reports and the
 * wrong handle. a ring bank, parameter reset or base volume that fails to set
 * is reported, and the port still opens */
MwSnd *mwSndOpenPort(int nch);
/* close snd, free its sound buffers and memory blocks, and return 0, or
 * SDE_ERR_HANDLE_ILLEGAL_VALUE for NULL. a driver error is reported, not
 * returned */
int mwSndClosePort(MwSnd *snd);
/* do nothing, and return 0 */
int mwSndSetHdInfo(MwSnd *snd);
/* play the rings at freq Hz as samples of bps bits: 4 for ADPCM, or 8 or 16 for
 * PCM. bps must be one of those. freq must be a rate sdgFreqTable lists */
int mwSndPlay(MwSnd *snd, int bps, Uint16 freq);
int mwSndStop(MwSnd *snd);
int mwSndStopAll(void);
/* mwSndGetSfreq and mwSndGetBitPerSmpl store 0 and return 0, without reading
 * snd */
int mwSndGetSfreq(MwSnd *snd, int *sfreq);
int mwSndGetBitPerSmpl(MwSnd *snd, int *bps);
/* mwSndGetVol stores twice channel 0's driver volume in *vol, and mwSndGetPan
 * stores channel ch's pan, scaled to -15 through 15, in *pan. each returns the
 * driver's error. after a failed read, each stores a value computed from a
 * status the driver left unwritten */
int mwSndGetVol(MwSnd *snd, int *vol);
int mwSndGetPan(MwSnd *snd, Sint8 ch, int *pan);
/* store in *pos channel ch's play position in samples, or 0 while bit 0 of
 * channel 0's driver flags is clear. the sample width is the one mwSndPlay
 * took, so mwSndPlay must have run. it returns the driver's error, or 0 while
 * the bit is clear. a failed read of the flags counts as the bit set. after a
 * failed read of the position, it stores a value computed from a position the
 * driver left unwritten */
int mwSndGetPlayPos(MwSnd *snd, Sint8 ch, int *pos);
/* set both channels' volume from vol. it sends 127 for a vol of 0 or below,
 * and -127 for 255 or above. between, it sends -(vol * 10 / 18), no lower than
 * -127. an error on channel 0 returns before channel 1 is set */
int mwSndSetVol(MwSnd *snd, int vol);
int mwSndSetDrctLev(MwSnd *snd, Sint8 lev);
/* send the port to effect input ch at a level from lev. it sends 0 for a lev
 * of 0 or below, and 127 for 15 or above. between, it sends
 * (2 * lev - 15) * 127 / 15, which is negative below 8 */
int mwSndSetFx(MwSnd *snd, Sint8 ch, Sint8 lev);
/* set channel ch's pan, -15 through 15, scaled to -127 through 127. a pan
 * outside -15 through 15 isn't clamped, and wraps as the Sint8 it is sent as */
int mwSndSetPan(MwSnd *snd, Sint8 ch, int pan);
/* set the pitch to semi semitones plus fine cents, each clamped to -100 through
 * 100, and sent in 256ths of a semitone */
int mwSndSetPitch(MwSnd *snd, Sint16 semi, Sint16 fine);
/* play usech of the port's channels, and return the driver's error */
int mwSndSetUseCh(MwSnd *snd, Sint8 usech);
/* mwSndSetFilter and mwSndSetTrnsMode do nothing, and return 0 */
int mwSndSetFilter(MwSnd *snd, Sint16 coff, Sint16 q);
int mwSndSetTrnsMode(MwSnd *snd, Sint32 mode);
/* flush all queued host commands; snd and sw are ignored */
int mwSndFlushCmd(MwSnd *snd, Sint32 sw);
/* transfer size bytes at src through channel ch's memory block to sound memory
 * offset dst, and return the driver's error. src must stay valid until
 * mwSndGetMblkStat stores SDE_MEMBLK_STAT_TRANSFER_FINISHED. a size below 32
 * switches every memory block's transfers to the CPU. the call reports that
 * size as an error, but it still makes the transfer and returns the driver's
 * error */
int mwSndTrns(MwSnd *snd, Sint8 ch, int dst, void *src, int size);
/* store channel ch's memory block state in *stat, and return the driver's
 * error. after a failed read, it stores a state the driver left unwritten */
int mwSndGetMblkStat(MwSnd *snd, Sint8 ch, SDE_MEMBLK_STAT *stat);
/* store in *ptr and *size channel ch's ring buffer: its sound memory offset and
 * its size in bytes. it returns the driver's error */
int mwSndGetBufInfo(MwSnd *snd, Sint8 ch, Sint32 *ptr, Sint32 *size);
/* mwSndGetFx and mwSndGetFilter store zeros and return 0, without reading
 * snd */
int mwSndGetFx(MwSnd *snd, int *ch, int *lev);
int mwSndGetFilter(MwSnd *snd, int *coff, int *q);
/* do nothing, and return 0 */
int mwSndGetHdPrm(void *obj, Sint32 arg1, Sint16 arg2);
#endif
