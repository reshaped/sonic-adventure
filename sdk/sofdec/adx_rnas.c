/* ADX sound output, whose calls pass on to the middleware's mwRna calls.
 * ADXRNA_GetNumRoom returns 0, and ADXRNA_ClearBuf, ADXRNA_SetPcmType,
 * ADXRNA_SetTotalNumSmpl and ADXRNA_SetWavFname do nothing */
#include <mw_rna.h>
#include <sg_xpt.h>
#include <sj.h>
#include <sofdec.h>

void ADXRNA_Init(void)
{
    mwRnaInit();
}

void ADXRNA_Finish(void)
{
    mwRnaFinish();
}

ADXRNA ADXRNA_Create(SJ *sjo, Sint32 nch)
{
    return mwRnaCreate(sjo, nch);
}

void ADXRNA_Destroy(ADXRNA rna)
{
    mwRnaStop(rna);
    mwRnaDestroy(rna);
}

void ADXRNA_Start(ADXRNA rna)
{
    mwRnaStart(rna);
}

void ADXRNA_Stop(ADXRNA rna)
{
    mwRnaStop(rna);
}

void ADXRNA_SetTransSw(ADXRNA rna, Sint32 sw)
{
    mwRnaSetTransSw(rna, sw);
}

void ADXRNA_SetPlaySw(ADXRNA rna, Sint32 sw)
{
    mwRnaSetPlaySw(rna, sw);
}

void ADXRNA_GetTime(ADXRNA rna, Sint32 *ncount, Sint32 *tscale)
{
    mwRnaGetTime(rna, ncount, tscale);
}

Sint32 ADXRNA_GetNumData(ADXRNA rna)
{
    return mwRnaGetNumData(rna);
}

Sint32 ADXRNA_GetNumRoom(ADXRNA rna)
{
    return 0;
}

Sint32 ADXRNA_GetStat(ADXRNA rna)
{
    return mwRnaGetStat(rna);
}

void ADXRNA_ExecServer(void)
{
    mwRnaExecServer();
}

void ADXRNA_SetStartSmpl(ADXRNA rna, Sint32 smpl)
{
    mwRnaSetStartSmpl(rna, smpl);
}

void ADXRNA_SetNumChan(ADXRNA rna, Sint32 nch)
{
    mwRnaSetNumChan(rna, nch);
}

void ADXRNA_SetSfreq(ADXRNA rna, Sint32 sfreq)
{
    mwRnaSetSfreq(rna, sfreq);
}

void ADXRNA_SetOutVol(ADXRNA rna, Sint32 vol)
{
    mwRnaSetOutVol(rna, vol);
}

void ADXRNA_SetOutPan(ADXRNA rna, Sint32 ch, Sint32 pan)
{
    mwRnaSetOutPan(rna, ch, pan);
}

void ADXRNA_SetBitPerSmpl(ADXRNA rna, Sint32 bps)
{
    mwRnaSetBitPerSmpl(rna, bps);
}

Sint32 ADXRNA_GetSfreq(ADXRNA rna)
{
    return mwRnaGetSfreq(rna);
}

Sint32 ADXRNA_GetOutVol(ADXRNA rna, Sint32 ch)
{
    /* @bug logic: returns ch's pan, not a volume: the image jumps to
     * mwRnaGetOutPan here, through the pool word ADXRNA_GetOutPan uses
     * (0x8c66d8fc) */
    return mwRnaGetOutPan(rna, ch);
}

Sint32 ADXRNA_GetOutPan(ADXRNA rna, Sint32 ch)
{
    return mwRnaGetOutPan(rna, ch);
}

Sint32 ADXRNA_GetBitPerSmpl(ADXRNA rna)
{
    return mwRnaGetBitPerSmpl(rna);
}

void ADXRNA_ClearBuf(ADXRNA rna)
{
}

void ADXRNA_SetPcmType(ADXRNA rna, Sint32 type)
{
}

void ADXRNA_SetTotalNumSmpl(ADXRNA rna, Sint32 nsmpl)
{
}

void ADXRNA_SetWavFname(ADXRNA rna, char *fname)
{
}

Sint32 ADXRNA_SetStmHdInfo(ADXRNA rna, char *hdinfo)
{
    Sint32 result;

    mwRnaSetStmHdInfo(rna, hdinfo, &result);
    /* @bug uninitialized: mwRnaSetStmHdInfo never writes result, so this
     * returns the frame word passed for it as it was (MOV.L @R15,R0 at
     * 0x8c66d8a4). adx_tsvr.c's one call leaves the result unread */
    return result;
}
