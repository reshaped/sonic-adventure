#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdModuleClosePort(struct sd_port *port)
{
    SDE_ERR err;

    /* @hack spelling: err assigned in the test, which tests sdModuleCheckPort's
     * R0 and copies it to R14 in the BF/S's slot (TST R0,R0 at 0x8c677ac4).
     * assigned apart, SHC copies it first and tests R14, and 3 of 23 words
     * differ */
    if ((err = sdModuleCheckPort(port)) != SDE_ERR_NOTHING) {
        goto error;
    }
    port->used = false;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdModuleClosePort, err);
    }
    return err;
}
