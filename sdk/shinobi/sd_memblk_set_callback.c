#include "shinobi/sd_work.h"
#include <shinobi.h>

/* the result for a NULL handle, SDE_ERR_HANDLE_NULL, is returned without
 * going to sdSysErrTrap */
SDE_ERR sdMemBlkSetCallback(SDMEMBLK handle,
                            SD_MEMBLK_CALLBACK_FUNC callback_func,
                            const void *callback_first_arg)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    if (handle == NULL) {
        err = SDE_ERR_HANDLE_NULL;
    } else {
        ((struct sd_memblk *)handle)->callback = callback_func;
        ((struct sd_memblk *)handle)->callback_first_arg =
            (void *)callback_first_arg;
    }
    return err;
}
