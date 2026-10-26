/* ev0062_ver2_wing_action and its motion */
#include <ninja.h>

#include "ev0062/ver2_wing_action.h"

extern NJS_OBJECT dat_0cb96084;
extern NJS_MDATA3 dat_0cb98278[];

static NJS_MOTION ev0062_ver2_wing_motion = {
#include "ev0062/ver2_wing_action/ev0062_ver2_wing_motion.inc"
};
NJS_ACTION ev0062_ver2_wing_action[1] = {
#include "ev0062/ver2_wing_action/ev0062_ver2_wing_action.inc"
};
