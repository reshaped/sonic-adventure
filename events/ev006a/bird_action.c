/* the bird's action and its motion. the action moves the bird's first model,
 * dat_0cba20f8, which sonic_fingers.h declares. the motion's NJS_MDATA2 table,
 * dat_0cba7770, lies in the data before this object */
#include <ninja.h>

#include "ev006a/bird_action.h"
#include "ev006a/sonic_fingers.h" /* NOLINT(misc-include-cleaner) */

extern NJS_MDATA2 dat_0cba7770[];

static NJS_MOTION ev006a_bird_motion = {
#include "ev006a/bird_action/ev006a_bird_motion.inc"
};
NJS_ACTION ev006a_bird_action = {
#include "ev006a/bird_action/ev006a_bird_action.inc"
};
