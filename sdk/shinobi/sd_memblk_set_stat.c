#include "shinobi/sd_work.h"
#include <shinobi.h>

SDE_ERR sdMemBlkSetStat(SDMEMBLK handle, SDE_MEMBLK_STAT stat)
{
    ((struct sd_memblk *)handle)->stat = stat;
    return SDE_ERR_NOTHING;
}
