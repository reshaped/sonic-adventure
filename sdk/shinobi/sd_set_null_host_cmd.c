#include "shinobi/sd_work.h"
#include <shinobi.h>
#include <string.h>

SDE_ERR sdSetNullHostCmd(void)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    struct sd_host_command command;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    command.command = 0;
    memset(command.params, 0, sizeof(command.params));
    sdSetHostCmd(&command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdSetNullHostCmd, err);
    }
    return err;
}
