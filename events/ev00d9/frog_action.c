/* Froggy's action and its motion. Froggy's model and the motion's
 * NJS_MDATA2 table lie in the data before this object */
#include <ninja.h>

#include "ev00d9/frog_action.h"

extern NJS_OBJECT dat_0cb8a03c;
extern NJS_MDATA2 dat_0cb8b910[];

static NJS_MOTION ev00d9_frog_motion = {
#include "ev00d9/frog_action/ev00d9_frog_motion.inc"
};
NJS_ACTION ev00d9_frog_action = {
#include "ev00d9/frog_action/ev00d9_frog_action.inc"
};
