/* Tails' last action in EV0168's script, and its motion */
#include <ninja.h>

#include "ev0168/tails_action_2.h"

extern NJS_MDATA2 dat_0cb89848[];
extern NJS_OBJECT tails_object;

static NJS_MOTION ev0168_tails_motion_2 = {
#include "ev0168/tails_action_2/ev0168_tails_motion_2.inc"
};
NJS_ACTION ev0168_tails_action_2 = {
#include "ev0168/tails_action_2/ev0168_tails_action_2.inc"
};
