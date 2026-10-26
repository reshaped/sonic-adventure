#include "shinobi/sd_work.h"
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

SYG2DMA sdgG2DmaHandle;

SDE_ERR sdSysWrkInit(void *wrk_ptr)
{
    /* @hack spelling: primary and second_max through locals, which SHC
     * computes ahead of the calls and keeps in the frame (MOV.L R2,@(4,R15)
     * at 0x8c65e3e6, MOV.L R4,@R15 at 0x8c65e3ec). passed directly, the
     * function runs 8 bytes short and 49 of 51 words differ */
    struct sd_primary_command *primary;
    Uint32 second_max;

    sdgWrkPtr = (struct sd_work *)(((Uint32)sdgDefaultWrkImg + 31) & ~31UL);
    second_max = SD_2ND_HOST_CMD_MAX;
    primary =
        (struct sd_primary_command *)(((Uint32)sdgDefPrimaryHostCmdBuf + 31) &
                                      ~31UL);
    sdMemBlkInit(sdgDefMemBlkBuf, SD_MEMBLK_MAX);
    sdMemBlkQueInit(sdgDefMemBlkQue, SD_MEMBLK_MAX);
    sdPrimaryHostCmdInitList(primary);
    sd2ndHostCmdInitList(sdgDef2ndHostCmdBuf, second_max);
    sdModuleRefInit();
    sdgWrkPtr->transfer_mode = SDE_MEMBLK_TRANSFER_MODE_CPU;
    syG2DmaOpen(&sdgWrkPtr->dma, SYE_G2_DMA_TYPE_AICA);
    sdgG2DmaHandle = sdgWrkPtr->dma;
    return SDE_ERR_NOTHING;
}
