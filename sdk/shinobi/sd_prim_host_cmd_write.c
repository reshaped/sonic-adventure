#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

SDE_ERR sdPrimaryHostCmdWrite(struct sd_host_command_entry *command)
{
    struct sd_primary_command *slot;
    SDE_ERR err = SDE_ERR_NOTHING;

    if (sdgWrkPtr->host.primary_num >= SD_PRIMARY_COMMAND_MAX) {
        err = SDE_ERR_HOST_CMD_BUF_NO_ENOUGH;
        goto error;
    }
    slot = &sdgWrkPtr->host.primary[sdgWrkPtr->host.primary_num++];
    slot->command = command->command.command;
    memcpy(slot->params, command->command.params, sizeof(slot->params));
    command->pending = false;
    command->command.command = 0;
    sdgWrkPtr->host.second_num--;

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdPrimaryHostCmdWrite, err);
    }
    return err;
}
