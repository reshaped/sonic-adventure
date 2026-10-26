/* copies of the headers an MPS handle read last */
#include <sg_xpt.h>

#include "movie/sdk/mps.h"

void MPSGET_Init(void)
{
}

void MPSGET_Finish(void)
{
}

Sint32 MPS_GetPackHd(struct mps_handle *mps, struct mps_pack_hd *pack)
{
    if (MPSLIB_CheckHn(mps) != 0) {
        return MPSLIB_SetErr(NULL, MPS_ERR_HN);
    }
    *pack = mps->pack;
    return 0;
}

Sint32 MPS_GetSysHd(struct mps_handle *mps, struct mps_sys_hd *sys, Sint32 kind)
{
    if (MPSLIB_CheckHn(mps) != 0) {
        return MPSLIB_SetErr(NULL, MPS_ERR_HN);
    }
    *sys = mps->sys_by_kind[kind];
    return 0;
}

Sint32 MPS_GetPketHd(struct mps_handle *mps, struct mps_pket_hd *pket)
{
    if (MPSLIB_CheckHn(mps) != 0) {
        return MPSLIB_SetErr(NULL, MPS_ERR_HN);
    }
    *pket = mps->pket;
    return 0;
}
