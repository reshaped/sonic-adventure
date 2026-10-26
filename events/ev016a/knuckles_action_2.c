/* Knuckles' last action in EV016A's script, and its motion */
#include <ninja.h>

#include "ev016a/knuckles_action_2.h"

extern NJS_MDATA2 dat_0cb8e170[];
extern NJS_OBJECT knuckles_object;

static NJS_MOTION ev016a_knuckles_motion_2 = {
#include "ev016a/knuckles_action_2/ev016a_knuckles_motion_2.inc"
};
NJS_ACTION ev016a_knuckles_action_2 = {
#include "ev016a/knuckles_action_2/ev016a_knuckles_action_2.inc"
};
