/* the Egg Mobile's action and its motion, which the script plays once
 * before Eggman's beam takes the emerald */
#include <ninja.h>

#include "ev0009/eggmobile_action.h"

extern NJS_OBJECT event_eggmobile_object;
extern NJS_MDATA2 dat_0cbae1e8[];

static NJS_MOTION ev0009_eggmobile_motion = {
#include "ev0009/eggmobile_action/ev0009_eggmobile_motion.inc"
};
NJS_ACTION ev0009_eggmobile_action = {
#include "ev0009/eggmobile_action/ev0009_eggmobile_action.inc"
};
