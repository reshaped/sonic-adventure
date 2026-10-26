/* second host command list write and read indices */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdGet2ndHostCmdIndex(Sint32 *write_index, Sint32 *read_index)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else {
        *read_index = sdgWrkPtr->host.second_read;
        *write_index = sdgWrkPtr->host.second_write;
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdGet2ndHostCmdIndex, err);
    }
    return err;
}
