/* the audio renderer: it copies each channel's samples from a stream joint to a
 * ring buffer in sound memory, which a sound port (mw_snd.h) plays. it reports
 * an error by passing a message to mwRnaErrCallFunc */
#ifndef MW_RNA_H
#define MW_RNA_H
#include <sg_mwply.h>
#include <sg_xpt.h>
#include <sj.h>

typedef struct mwrna_obj *MWRNA;

void mwRnaInit(void);
void mwRnaFinish(void);
/* a renderer of nch channels, 1 or 2, which reads channel i from sj[i]. the
 * renderer keeps the joints, and they must stay valid until mwRnaDestroy. it
 * returns NULL for another nch, a null sj or joint, no free renderer, or a
 * sound port mwSndOpenPort fails to open */
MWRNA mwRnaCreate(SJ *sj, Sint32 nch);
void mwRnaDestroy(MWRNA rna);
void mwRnaStart(MWRNA rna);
void mwRnaStop(MWRNA rna);
void mwRnaFlush(MWRNA rna);
/* mwRnaPause turns the transfer and play off and marks rna paused, which keeps
 * MWRNA_SW_AUTO from turning play on. mwRnaContinue unpauses rna. if the
 * transfer or play was on at the pause, it turns the transfer on, runs
 * mwRnaExecServer once and turns play on. both come back on even where only
 * one was, as mw_rna.c's @bug on swsave says. mwRnaPause on a paused renderer,
 * and mwRnaContinue on one not paused, do nothing */
void mwRnaPause(MWRNA rna);
void mwRnaContinue(MWRNA rna);
Sint32 mwRnaIsPause(MWRNA rna);
void mwRnaSetTransSw(MWRNA rna, Sint32 sw);
void mwRnaSetPlaySw(MWRNA rna, Sint32 sw);
void mwRnaExecServer(void);
/* store in *ncount the samples played since the transfer was turned on, and in
 * *tscale the samples per second */
void mwRnaGetTime(MWRNA rna, Sint32 *ncount, Sint32 *tscale);
/* the bits of mwRnaGetStat's result */
/* set while samples are transferred */
#define MWRNA_SW_TRANS 1
/* set while the port plays */
#define MWRNA_SW_PLAY 2
/* set from mwRnaStart until mwRnaStop. while it is set, the server turns play
 * on once mwRnaGetNumData reaches the count mwRnaSetStartSmpl had set at
 * mwRnaStart, unless the renderer is paused. it turns play off once the
 * transfer is off and every written sample has played */
#define MWRNA_SW_AUTO 4
Sint32 mwRnaGetStat(MWRNA rna);
Sint32 mwRnaGetTransSw(MWRNA rna);
Sint32 mwRnaGetPlaySw(MWRNA rna);
Sint32 mwRnaGetAutoSw(MWRNA rna);
Sint32 mwRnaGetNumData(MWRNA rna);
Sint32 mwRnaGetNumRoom(MWRNA rna);
/* use nch channels, from 1 to the count mwRnaCreate took, and pan them: 0 for
 * one channel, and -15 and 15 for two */
void mwRnaSetNumChan(MWRNA rna, Sint32 nch);
void mwRnaSetStartSmpl(MWRNA rna, Sint32 smpl);
void mwRnaSetBitPerSmpl(MWRNA rna, Sint32 bps);
void mwRnaSetSfreq(MWRNA rna, Sint32 sfreq);
void mwRnaSetOutVol(MWRNA rna, Sint32 vol);
/* set channel ch's pan, clamped to -15 through 15. ch must be 0 or 1, which the
 * call doesn't check */
void mwRnaSetOutPan(MWRNA rna, Sint32 ch, Sint32 pan);
void mwRnaSetFx(MWRNA rna, Sint32 ch, Sint32 lev);
void mwRnaSetPitch(MWRNA rna, Sint32 semi, Sint32 fine);
Sint32 mwRnaGetBitPerSmpl(MWRNA rna);
Sint32 mwRnaGetSfreq(MWRNA rna);
Sint32 mwRnaGetOutVol(MWRNA rna);
Sint32 mwRnaGetOutPan(MWRNA rna, Sint32 ch);
void mwRnaGetFx(MWRNA rna, Sint32 *ch, Sint32 *lev);
void mwRnaGetPitch(MWRNA rna, Sint32 *semi, Sint32 *fine);
void mwRnaEntryErrFunc(MW_PLY_ERRFN errfn, void *obj);
Sint32 mwRnaIsOvrflw(MWRNA rna);
void mwRnaClearOvrflw(MWRNA rna);
void mwRnaSetFilter(MWRNA rna, Sint32 coff, Sint32 q);
void mwRnaGetFilter(MWRNA rna, Sint32 *coff, Sint32 *q);
/* report a null rna as an error. for any other rna the call changes nothing:
 * it returns before it copies the port's format, because mwSndGetBitPerSmpl
 * always stores 0 bits per sample. hdinfo and result are never read */
void mwRnaSetStmHdInfo(MWRNA rna, char *hdinfo, Sint32 *result);
void mwRnaGetHdPrm(MWRNA rna, Sint32 arg1, Sint32 arg2);

/* mwRnaErrEntryFunc sets the function that receives the renderer's and the
 * sound ports' error messages, and the obj it is called with. mwRnaErrCallFunc
 * passes ecode to that function, and does nothing while none is set */
void mwRnaErrEntryFunc(MW_PLY_ERRFN errfn, void *obj);
void mwRnaErrCallFunc(char *ecode);
/* move the read position to the port's play position, rounded down to 16
 * samples. the samples played since come off mwRnaGetNumData's count. when more
 * were played than were written, the count goes to 0 and the overflow flag is
 * set. while the transfer is on, a set overflow flag moves the write position
 * to the read position and clears the flag */
void mwlRnaAddRdPos(MWRNA rna);
#endif
