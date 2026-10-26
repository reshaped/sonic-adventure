/* sound library initialization */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdLibInit(void *wrk_ptr, Sint32 mem_blk_handle_max,
                  Sint32 second_host_cmd_max)
{
    SDE_ERR err;

    err = sdSysWrkInit(NULL);
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdLibInit, err);
    }
    return err;
}
