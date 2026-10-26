/* the emerald's action and its motion. the motion moves dat_0cb864f4, the
 * emerald's model, as Tails shows the emerald to Sonic */
#include <ninja.h>

#include "ev0006/emerald_action.h"

extern NJS_OBJECT dat_0cb864f4;
extern NJS_MDATA2 dat_0cb86a68[];

static NJS_MOTION ev0006_emerald_motion = {
#include "ev0006/emerald_action/ev0006_emerald_motion.inc"
};
NJS_ACTION ev0006_emerald_action = {
#include "ev0006/emerald_action/ev0006_emerald_action.inc"
};
