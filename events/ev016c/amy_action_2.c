/* Amy's last action in EV016C's script, and its motion */
#include <ninja.h>

#include "ev016c/amy_action_2.h"

extern NJS_MDATA2 dat_0cb8da08[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev016c_amy_motion_2 = {
#include "ev016c/amy_action_2/ev016c_amy_motion_2.inc"
};
NJS_ACTION ev016c_amy_action_2 = {
#include "ev016c/amy_action_2/ev016c_amy_action_2.inc"
};
