#include "shinobi/sd_work.h"
#include <shinobi.h>

SDE_ERR sdMidiOpenPort(SDMIDI *handle)
{
    SDE_ERR err = SDE_ERR_NOTHING;

    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
    } else if (handle) {
        err = sdModuleOpenPort(handle, SD_PORT_MIDI);
    }
    if (err != SDE_ERR_NOTHING) {
        sdSysErrTrap((void *)sdMidiOpenPort, err);
    }
    return err;
}
