/* the action Sonic plays once before he runs off, and its motion */
#include <ninja.h>

#include "ev0011/sonic_action_4.h"

extern NJS_MDATA2 dat_0cb8c520[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0011_sonic_motion_4 = {
#include "ev0011/sonic_action_4/ev0011_sonic_motion_4.inc"
};
NJS_ACTION ev0011_sonic_action_4 = {
#include "ev0011/sonic_action_4/ev0011_sonic_action_4.inc"
};
