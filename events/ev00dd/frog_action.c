/* ev00dd_frog_action and its motion */
#include <ninja.h>

#include "ev00dd/frog_action.h"

extern NJS_OBJECT dat_0cb844a4;
extern NJS_MDATA2 dat_0cb85ed8[];

static NJS_MOTION ev00dd_frog_motion = {
#include "ev00dd/frog_action/ev00dd_frog_motion.inc"
};
NJS_ACTION ev00dd_frog_action = {
#include "ev00dd/frog_action/ev00dd_frog_action.inc"
};
