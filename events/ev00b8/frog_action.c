/* ev00b8_frog_action and its motion */
#include <ninja.h>

#include "ev00b8/frog_action.h"

extern NJS_MDATA2 dat_0cb81cb8[];
extern NJS_OBJECT frog_object;

static NJS_MOTION ev00b8_frog_motion = {
#include "ev00b8/frog_action/ev00b8_frog_motion.inc"
};
NJS_ACTION ev00b8_frog_action = {
#include "ev00b8/frog_action/ev00b8_frog_action.inc"
};
