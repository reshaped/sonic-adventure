/* Froggy's action and its motion. the motion's NJS_MDATA2 table lies in the
 * data before this object */
#include <ninja.h>

#include "ev00d4/frog_action.h"

extern NJS_MDATA2 dat_0cb8c178[];
extern NJS_OBJECT frog_object;

static NJS_MOTION ev00d4_frog_motion = {
#include "ev00d4/frog_action/ev00d4_frog_motion.inc"
};
NJS_ACTION ev00d4_frog_action = {
#include "ev00d4/frog_action/ev00d4_frog_action.inc"
};
