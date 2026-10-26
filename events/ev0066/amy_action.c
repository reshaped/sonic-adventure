/* ev0066_amy_action and its motion */
#include <ninja.h>

#include "ev0066/amy_action.h"

extern NJS_MDATA2 dat_0cb83828[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev0066_amy_motion = {
#include "ev0066/amy_action/ev0066_amy_motion.inc"
};
NJS_ACTION ev0066_amy_action = {
#include "ev0066/amy_action/ev0066_amy_action.inc"
};
