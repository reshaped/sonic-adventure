/* ev0050_tails_action and its motion */
#include <ninja.h>

#include "ev0050/tails_action.h"

extern NJS_MDATA2 ev0050_tails_motion_mdata[];
extern NJS_OBJECT tails_object;

static NJS_MOTION ev0050_tails_motion = {
#include "ev0050/tails_action/ev0050_tails_motion.inc"
};
NJS_ACTION ev0050_tails_action = {
#include "ev0050/tails_action/ev0050_tails_action.inc"
};
