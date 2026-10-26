#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdModuleCheckPort(struct sd_port *port)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    if (port == NULL) {
        err = SDE_ERR_HANDLE_NULL;
    } else if (port->used != true) {
        err = SDE_ERR_HANDLE_ILLEGAL_VALUE;
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdModuleCheckPort, err);
    }
    return err;
}
