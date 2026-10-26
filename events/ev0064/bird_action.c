/* ev0064_bird_action and its motion */
#include <ninja.h>

#include "ev0064/bird_action.h"

extern NJS_OBJECT dat_0cb95bf4;
extern NJS_MDATA3 dat_0cb97de8[];

static NJS_MOTION ev0064_bird_motion = {
#include "ev0064/bird_action/ev0064_bird_motion.inc"
};
NJS_ACTION ev0064_bird_action = {
#include "ev0064/bird_action/ev0064_bird_action.inc"
};
