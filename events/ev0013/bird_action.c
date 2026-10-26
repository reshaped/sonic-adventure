/* the action the bird repeats until the script kills the bird, and its
 * motion */
#include <ninja.h>

#include "ev0013/bird_action.h"

extern NJS_OBJECT dat_0cb925cc;
extern NJS_MDATA3 dat_0cb947c0[];

static NJS_MOTION ev0013_bird_motion = {
#include "ev0013/bird_action/ev0013_bird_motion.inc"
};
NJS_ACTION ev0013_bird_action = {
#include "ev0013/bird_action/ev0013_bird_action.inc"
};
