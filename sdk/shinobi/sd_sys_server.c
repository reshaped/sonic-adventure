#include "shinobi/sd_work.h"
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

Uint32 sdgG2DmaTriggerNum;
Uint32 sdgG2DmaEndNum;

SDE_ERR sdSysServer(void)
{
    if (sdgWrkPtr != NULL && sdgExistSndDrv == true) {
        if (sdgWrkPtr->host.second_num > 0) {
            sd2ndHostCmdSendList();
        }
        if (sdgWrkPtr->dma != NULL) {
            syG2DmaGetTriggerCounter(SYE_G2_DMA_TYPE_AICA, &sdgG2DmaTriggerNum);
            syG2DmaGetEndCounter(SYE_G2_DMA_TYPE_AICA, &sdgG2DmaEndNum);
            if (sdgG2DmaTriggerNum < sdgG2DmaEndNum) {
                sdSysErrTrap((void *)sdSysServer, SDE_ERR_UNKNOW_NUM);
            }
        }
    }
    return SDE_ERR_NOTHING;
}
