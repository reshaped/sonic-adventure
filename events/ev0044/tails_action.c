/* ev0044_tails_action and its motion */
#include <ninja.h>

#include "ev0044/tails_action.h"

extern NJS_MDATA2 dat_0cb82dd0[];
extern NJS_OBJECT tails_object;

static NJS_MOTION ev0044_tails_motion = {
#include "ev0044/tails_action/ev0044_tails_motion.inc"
};
NJS_ACTION ev0044_tails_action = {
#include "ev0044/tails_action/ev0044_tails_action.inc"
};
