/* ev0063_bird_action and its motion */
#include <ninja.h>

#include "ev0063/bird_action.h"

extern NJS_OBJECT dat_0cba2724;
extern NJS_MDATA3 dat_0cba4918[];

static NJS_MOTION ev0063_bird_motion = {
#include "ev0063/bird_action/ev0063_bird_motion.inc"
};
NJS_ACTION ev0063_bird_action = {
#include "ev0063/bird_action/ev0063_bird_action.inc"
};
