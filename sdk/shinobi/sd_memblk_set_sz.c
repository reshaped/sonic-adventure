#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdMemBlkSetSz(SDMEMBLK handle, Sint32 src_blk_sz)
{
    ((struct sd_memblk *)handle)->size = src_blk_sz;
    return SDE_ERR_NOTHING;
}
