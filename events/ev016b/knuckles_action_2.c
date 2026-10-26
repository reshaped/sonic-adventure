/* Knuckles' last action in EV016B's script, and its motion */
#include <ninja.h>

#include "ev016b/knuckles_action_2.h"

extern NJS_MDATA2 dat_0cb8e5d8[];
extern NJS_OBJECT knuckles_object;

static NJS_MOTION ev016b_knuckles_motion_2 = {
#include "ev016b/knuckles_action_2/ev016b_knuckles_motion_2.inc"
};
NJS_ACTION ev016b_knuckles_action_2 = {
#include "ev016b/knuckles_action_2/ev016b_knuckles_action_2.inc"
};
