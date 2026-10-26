/* ev001d_eggmobile_action and its motion. event_eggmobile_object, the model
 * the action moves, lies in the data after this object. dat_0cb84360, the
 * motion's NJS_MDATA2 table, lies in the data before this object */
#include <ninja.h>

#include "ev001d/eggmobile_action.h"

extern NJS_MDATA2 dat_0cb84360[];
extern NJS_OBJECT event_eggmobile_object;

static NJS_MOTION ev001d_eggmobile_motion = {
#include "ev001d/eggmobile_action/ev001d_eggmobile_motion.inc"
};
NJS_ACTION ev001d_eggmobile_action = {
#include "ev001d/eggmobile_action/ev001d_eggmobile_action.inc"
};
