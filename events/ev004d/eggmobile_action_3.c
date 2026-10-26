/* ev004d_eggmobile_action_3 and its motion */
#include <ninja.h>

#include "ev004d/eggmobile_action_3.h"

extern NJS_MDATA2 ev004d_eggmobile_motion_3_mdata[];
extern NJS_OBJECT event_eggmobile_object;

static NJS_MOTION ev004d_eggmobile_motion_3 = {
#include "ev004d/eggmobile_action_3/ev004d_eggmobile_motion_3.inc"
};
NJS_ACTION ev004d_eggmobile_action_3 = {
#include "ev004d/eggmobile_action_3/ev004d_eggmobile_action_3.inc"
};
