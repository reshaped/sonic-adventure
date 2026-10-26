/* ev00ff_big_action and its motion */
#include <ninja.h>

#include "ev00ff/big_action.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

extern NJS_MDATA2 dat_0cba25f0[];

static NJS_MOTION ev00ff_big_motion = {
#include "ev00ff/big_action/ev00ff_big_motion.inc"
};
NJS_ACTION ev00ff_big_action = {
#include "ev00ff/big_action/ev00ff_big_action.inc"
};
