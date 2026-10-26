/* an action that pairs big_object, Big's model in 1ST_READ.BIN, with a
 * motion of 48 frames */
#include <ninja.h>

#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */
#include "stg12/big_action.h"

extern NJS_MDATA2 dat_0ca90120[];

static NJS_MOTION shelter_big_motion = {
#include "stg12/big_action/shelter_big_motion.inc"
};
NJS_ACTION shelter_big_action = {
#include "stg12/big_action/shelter_big_action.inc"
};
