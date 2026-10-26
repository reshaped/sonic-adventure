/* the sound library's default buffers and its G2 bus result */
#include "shinobi/sd_work.h"
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

Uint8 sdgDefaultWrkImg[0x58c];
struct SDS_MEM_BLK sdgDefMemBlkBuf[SD_MEMBLK_MAX];
struct sd_memblk_queue sdgDefMemBlkQue[SD_MEMBLK_MAX];
Uint8 sdgDefPrimaryHostCmdBuf[(SD_PRIMARY_COMMAND_MAX *
                               sizeof(struct sd_primary_command)) +
                              32];
struct sd_host_command_entry sdgDef2ndHostCmdBuf[SD_2ND_HOST_CMD_MAX];
SYE_G2_ERR sdgG2LibResult;
