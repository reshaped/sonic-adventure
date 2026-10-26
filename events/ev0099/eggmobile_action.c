/* ev0099_eggmobile_action and its motion */
#include <ninja.h>

#include "ev0099/eggmobile_action.h"

extern NJS_MDATA2 dat_0cb896d0[];
extern NJS_OBJECT event_eggmobile_object;

static NJS_MOTION ev0099_eggmobile_motion = {
#include "ev0099/eggmobile_action/ev0099_eggmobile_motion.inc"
};
NJS_ACTION ev0099_eggmobile_action = {
#include "ev0099/eggmobile_action/ev0099_eggmobile_action.inc"
};
