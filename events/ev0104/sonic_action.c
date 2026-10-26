/* ev0104_sonic_action and its motion */
#include <ninja.h>

#include "ev0104/sonic_action.h"

extern NJS_MDATA2 dat_0cb81818[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0104_sonic_motion = {
#include "ev0104/sonic_action/ev0104_sonic_motion.inc"
};
NJS_ACTION ev0104_sonic_action = {
#include "ev0104/sonic_action/ev0104_sonic_action.inc"
};
