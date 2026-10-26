#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdModuleOpenPort(SDHANDLE **handle, Sint32 type)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    struct sd_port *port;
    Sint32 i;

    switch (type) {
    case SD_PORT_MIDI:
        port = sdgWrkPtr->ports.kind.midi;
        for (i = 0; i < SDD_MIDI_PORT_MAX; i++) {
            if (port->used != true) {
                break;
            }
            port++;
        }
        if (i == SDD_MIDI_PORT_MAX) {
            port = NULL;
        }
        break;
    case SD_PORT_SHOT:
        port = sdgWrkPtr->ports.kind.shot;
        for (i = 0; i < SDD_SHOT_PORT_MAX; i++) {
            if (port->used != true) {
                break;
            }
            port++;
        }
        if (i == SDD_SHOT_PORT_MAX) {
            port = NULL;
        }
        break;
    case SD_PORT_PSTM:
        port = sdgWrkPtr->ports.kind.pstm;
        for (i = 0; i < SDD_PSTM_PORT_MAX; i++) {
            if (port->used != true) {
                break;
            }
            port++;
        }
        if (i == SDD_PSTM_PORT_MAX) {
            port = NULL;
        }
        break;
    default:
        err = SDE_ERR_UNKNOW_NUM;
        goto error;
    }
    if (port != NULL) {
        *handle = (SDHANDLE *)port;
        /* @hack spelling: the store through *handle, which SHC makes ahead of
         * the BRA (MOV.L R3,@(4,R2) at 0x8c65e1e6). through port, it moves into
         * the BRA's slot, the function runs 4 bytes short and 23 of 85 words
         * differ */
        ((struct sd_port *)*handle)->used = true;
    } else {
        err = SDE_ERR_HANDLE_NO_ENOUGH;
        *handle = NULL;
    }

error:
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdModuleOpenPort, err);
    }
    return err;
}
