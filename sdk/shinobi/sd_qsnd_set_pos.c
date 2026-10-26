#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define HOST_QSOUND_POSITION 0x88
#define QSOUND_POSITION_SIZE 7

SDE_ERR sdQsndSetPos(const Sint8 *pos)
{
    /* @hack spelling: position_command, static in the function, whose params
     * SHC addresses through a pool word of their own (MOV.L at 0x8c6185a8). at
     * file scope, SHC addresses them from the command's address, the function
     * runs 4 bytes short and 27 of 38 words differ */
    static struct sd_host_command position_command;
    SDE_ERR err;
    /* @hack type: i a byte, which the loop sign-extends for each index
     * (EXTS.B R5,R3 at 0x8c6185a2); as a Sint32, 29 of 38 words differ */
    Sint8 i;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    position_command.command = HOST_QSOUND_POSITION;
    for (i = 0; i < QSOUND_POSITION_SIZE; i++) {
        position_command.params[i] = *pos++;
    }
    err = sdSetHostCmd(&position_command);

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdQsndSetPos, err);
    }
    return err;
}
