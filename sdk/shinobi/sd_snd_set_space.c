#include "shinobi/sd_work.h"
#include <shinobi.h>

#define HOST_SPACE 0x8a
#define SPACE_MONO 0xff
#define SPACE_STEREO 0x00

SDE_ERR sdSndSetSpace(const SDE_SPACE space)
{
    static struct sd_host_command space_command;
    SDE_ERR err = SDE_ERR_NOTHING;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    space_command.command = HOST_SPACE;
    switch (space) {
    case SDE_SPACE_MONO:
        space_command.params[0] = SPACE_MONO;
        break;
    case SDE_SPACE_STEREO:
        space_command.params[0] = SPACE_STEREO;
        break;
    default:
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }
    err = sdSetHostCmd(&space_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdSndSetSpace, err);
    }
    return err;
}
