/* E-102's third action in EV016F's script, and its motion */
#include <ninja.h>

#include "ev016f/e102_action_2.h"

extern NJS_MDATA2 dat_0cb8e0f0[];
extern NJS_OBJECT e102_object;

static NJS_MOTION ev016f_e102_motion_2 = {
#include "ev016f/e102_action_2/ev016f_e102_motion_2.inc"
};
NJS_ACTION ev016f_e102_action_2 = {
#include "ev016f/e102_action_2/ev016f_e102_action_2.inc"
};
