#include "shinobi/sd_work.h"
#include <shinobi.h>

SDE_ERR sdMidiGetStat(SDMIDI handle, SDS_MIDI_STAT *midi_stat)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    sdMidiGetErr(handle, &midi_stat->m_Err);
    sdMidiGetFlg(handle, &midi_stat->m_Flg);
    midi_stat->m_HostCmdRsvNum = 0;
    midi_stat->m_HostCmdDelayTime = 0;
    sdMidiGetVol(handle, &midi_stat->m_Vol);
    sdMidiGetPan(handle, &midi_stat->m_Pan);
    sdMidiGetPitch(handle, &midi_stat->m_Pitch);
    sdMidiGetSpeed(handle, &midi_stat->m_Speed);
    sdMidiGetFxLev(handle, &midi_stat->m_FxLev);
    sdMidiGetDrctLev(handle, &midi_stat->m_DrctLev);
    sdMidiGetCurAdr(handle, &midi_stat->m_CurAdr);
    sdMidiGetTotalBeatTime(handle, &midi_stat->m_TotalBeatTime);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiGetStat, err);
    }
    return err;
}
