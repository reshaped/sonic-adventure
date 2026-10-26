/* memory block transfer status query */
#include "shinobi/sd_work.h"
#include <shinobi.h>

SDE_ERR sdMemBlkGetStat(SDMEMBLK handle, SDE_MEMBLK_STAT *stat)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    if (handle == NULL) {
        err = SDE_ERR_HANDLE_NULL;
        goto error;
    }
    if (stat == NULL) {
        err = SDE_ERR_PTR_NULL;
        goto error;
    }
    *stat = ((struct sd_memblk *)handle)->stat;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMemBlkGetStat, err);
    }
    return err;
}
