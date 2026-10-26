/* ev0114_sonic_action and its motion */
#include <ninja.h>

#include "ev0114/sonic_action.h"

extern NJS_MDATA2 dat_0cb817f0[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0114_sonic_motion = {
#include "ev0114/sonic_action/ev0114_sonic_motion.inc"
};
NJS_ACTION ev0114_sonic_action = {
#include "ev0114/sonic_action/ev0114_sonic_action.inc"
};
