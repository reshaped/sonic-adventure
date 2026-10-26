#include "shinobi/sd_work.h"
#include <shinobi.h>

SDE_ERR sdMemBlkSetAdr(SDMEMBLK handle, const void *src_blk_ptr)
{
    ((struct sd_memblk *)handle)->data = (void *)src_blk_ptr;
    return SDE_ERR_NOTHING;
}
