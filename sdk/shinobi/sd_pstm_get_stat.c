#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdPstmGetStat(SDPSTM handle, const Sint8 target_ch,
                      SDS_PSTM_STAT *pstm_stat)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    /* @hack redundant: the else after the first goto, which has SHC call
     * sdPstmGetErr through R3 and sdPstmGetFlg through R2 (0x8c676ee8,
     * 0x8c676ef4). without it, the two trade and 4 of 98 words differ */
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    } else if (!handle) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    sdPstmGetErr(handle, target_ch, &pstm_stat->m_Err);
    sdPstmGetFlg(handle, target_ch, &pstm_stat->m_Flg);
    pstm_stat->m_HostCmdRsvNum = 0;
    pstm_stat->m_HostCmdDelayTime = 0;
    sdPstmGetVol(handle, target_ch, &pstm_stat->m_Vol);
    sdPstmGetPan(handle, target_ch, &pstm_stat->m_Pan);
    sdPstmGetPitch(handle, target_ch, &pstm_stat->m_Pitch);
    sdPstmGetSpeed(handle, target_ch, &pstm_stat->m_Speed);
    sdPstmGetFxLev(handle, target_ch, &pstm_stat->m_FxLev);
    sdPstmGetDrctLev(handle, target_ch, &pstm_stat->m_DrctLev);
    sdPstmGetCurAdr(handle, target_ch, &pstm_stat->m_CurAdr);
    sdPstmGetTotalSmpFrame(handle, target_ch, &pstm_stat->m_TotalSmpFrame);

error:
    if (err != SDE_ERR_NOTHING) {
        /* @bug logic: the trap names sdMidiGetStat, not this function: the
         * pool word at 0x8c676fb4 holds sdMidiGetStat's address */
        sdSysErrTrap((void *)sdMidiGetStat, err);
    }
    return err;
}
