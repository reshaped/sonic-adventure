/* E-102's third action in EV016E's script, and its motion */
#include <ninja.h>

#include "ev016e/e102_action_2.h"

extern NJS_MDATA2 dat_0cb8e7d0[];
extern NJS_OBJECT e102_object;

static NJS_MOTION ev016e_e102_motion_2 = {
#include "ev016e/e102_action_2/ev016e_e102_motion_2.inc"
};
NJS_ACTION ev016e_e102_action_2 = {
#include "ev016e/e102_action_2/ev016e_e102_action_2.inc"
};
