/* second host command list initialization */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

struct sd_host_command_entry sdg2ndHostCmdDefault = {{0, {0}}, 0, 0};
struct sd_host_command_entry *sdg2ndHostCmdBufPtr;

SDE_ERR sd2ndHostCmdInitList(struct sd_host_command_entry *buffer, Uint32 count)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Uint32 i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    sdgWrkPtr->host.second = buffer;
    sdgWrkPtr->host.second_max = count;
    sdgWrkPtr->host.second_read = 0;
    sdgWrkPtr->host.second_write = 0;
    sdg2ndHostCmdBufPtr = buffer;
    memset(buffer, 0, count * sizeof(struct sd_host_command_entry));
    for (i = 0; i < count; i++) {
        sdgWrkPtr->host.second[i] = sdg2ndHostCmdDefault;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sd2ndHostCmdInitList, err);
    }
    return err;
}
