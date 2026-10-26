/* ev0084_knuckles_action and its motion */
#include <ninja.h>

#include "ev0084/knuckles_action.h"

extern NJS_MDATA2 dat_0cb82720[];
extern NJS_OBJECT knuckles_object;

static NJS_MOTION ev0084_knuckles_motion = {
#include "ev0084/knuckles_action/ev0084_knuckles_motion.inc"
};
NJS_ACTION ev0084_knuckles_action = {
#include "ev0084/knuckles_action/ev0084_knuckles_action.inc"
};
