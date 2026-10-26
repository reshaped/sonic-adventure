/* Amy's last action in EV016D's script, and its motion */
#include <ninja.h>

#include "ev016d/amy_action_2.h"

extern NJS_MDATA2 dat_0cb8dc38[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev016d_amy_motion_2 = {
#include "ev016d/amy_action_2/ev016d_amy_motion_2.inc"
};
NJS_ACTION ev016d_amy_action_2 = {
#include "ev016d/amy_action_2/ev016d_amy_action_2.inc"
};
