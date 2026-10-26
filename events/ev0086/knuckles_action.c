/* ev0086_knuckles_action and its motion */
#include <ninja.h>

#include "ev0086/knuckles_action.h"

extern NJS_MDATA2 dat_0cb82db0[];
extern NJS_OBJECT knuckles_object;

static NJS_MOTION ev0086_knuckles_motion = {
#include "ev0086/knuckles_action/ev0086_knuckles_motion.inc"
};
NJS_ACTION ev0086_knuckles_action = {
#include "ev0086/knuckles_action/ev0086_knuckles_action.inc"
};
