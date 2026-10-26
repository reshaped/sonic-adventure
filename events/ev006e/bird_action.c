/* the bird's action and its motion. the model the action moves and the
 * motion's NJS_MDATA3 table lie in the data before this object */
#include <ninja.h>

#include "ev006e/bird_action.h"

extern NJS_OBJECT dat_0cb91ecc;
extern NJS_MDATA3 dat_0cb940c0[];

static NJS_MOTION ev006e_bird_motion = {
#include "ev006e/bird_action/ev006e_bird_motion.inc"
};
NJS_ACTION ev006e_bird_action = {
#include "ev006e/bird_action/ev006e_bird_action.inc"
};
