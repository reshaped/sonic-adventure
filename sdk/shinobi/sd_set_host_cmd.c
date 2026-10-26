#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdSetHostCmd(struct sd_host_command *command)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    int mask;
    struct sd_host_commands *host;
    struct sd_host_command_entry *entry;
    Sint32 num;
    Sint32 max;

    mask = get_imask();
    set_imask(IMASK_ALL);
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    host = &sdgWrkPtr->host;
    num = host->second_num;
    max = host->second_max;
    /* the list is full at SD_2ND_HOST_CMD_MAX commands, whatever second_max
     * is */
    if (num == SD_2ND_HOST_CMD_MAX) {
        err = SDE_ERR_HOST_CMD_BUF_NO_ENOUGH;
        goto error;
    }
    entry = &host->second[host->second_write++];
    if (host->second_write == max) {
        host->second_write = 0;
    }
    entry->pending = true;
    entry->command = *command;
    host->second_num++;

error:
    set_imask(mask);
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdSetHostCmd, err);
    }
    return err;
}
