/* Big's stand facing down a slope: its motion, then the action that plays
 * the motion on big_object */
#include <ninja.h>

#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */
#include "player/big_stand_downhill.h"

extern NJS_MDATA2 dat_8c57c3d0[];

static NJS_MOTION big_stand_downhill_motion = {
#include "player/big_stand_downhill/big_stand_downhill_motion.inc"
};
NJS_ACTION big_stand_downhill_action = {
#include "player/big_stand_downhill/big_stand_downhill_action.inc"
};
