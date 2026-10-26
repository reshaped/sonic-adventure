/* port table initialization, and host command reserve and delay settings */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

#define RSV_NUM_MAX 32
#define DELAY_TIME_MAX 16

SDE_ERR sdModuleRefInit(void)
{
    Uint32 i;

    for (i = 0; i < SD_PORT_MAX; i++) {
        sdgWrkPtr->ports.all[i].number = 0;
        sdgWrkPtr->ports.all[i].used = false;
    }
    for (i = 0; i < SDD_MIDI_PORT_MAX; i++) {
        sdgWrkPtr->ports.kind.midi[i].number = i;
        sdgWrkPtr->ports.kind.midi[i].type = SD_PORT_MIDI;
    }
    for (i = 0; i < SDD_SHOT_PORT_MAX; i++) {
        sdgWrkPtr->ports.kind.shot[i].number = i;
        sdgWrkPtr->ports.kind.shot[i].type = SD_PORT_SHOT;
    }
    for (i = 0; i < SDD_PSTM_PORT_MAX; i++) {
        sdgWrkPtr->ports.kind.pstm[i].number = i;
        sdgWrkPtr->ports.kind.pstm[i].type = SD_PORT_PSTM;
    }
    sdgWrkPtr->ports.kind.system.number = 0;
    sdgWrkPtr->ports.kind.system.type = SD_PORT_SYSTEM;
    return SDE_ERR_NOTHING;
}

SDE_ERR sdSetHostCmdRsvNum(struct sd_port *port, Sint8 num)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    /* @hack redundant: a goto to the next label, which has SHC call
     * sdSysErrTrap through R2 (0x8c671610). without it, SHC calls sdSysErrTrap
     * through R3, and 2 of 20 words differ */
    if (num < 0 || num > RSV_NUM_MAX) {
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdSetHostCmdRsvNum, err);
    }
    return err;
}

SDE_ERR sdSetHostCmdDelayTime(struct sd_port *port, Sint8 time)
{
    SDE_ERR err;

    if (time < 0 || time > DELAY_TIME_MAX) {
        err = SDE_ERR_PRM_OVER_RANGE;
        goto error;
    }
    /* @bug memory: the time is stored only when sdModuleCheckPort fails, so
     * a NULL port is written through. err then holds 1, not the check's error:
     * TST R0,R0 and MOVT R14 at 0x8c671644 and 0x8c671646, and the store at
     * 0x8c671654 */
    err = sdModuleCheckPort(port) != SDE_ERR_NOTHING;
    if (err != SDE_ERR_NOTHING) {
        port->delay_time = time;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdSetHostCmdDelayTime, err);
    }
    return err;
}
