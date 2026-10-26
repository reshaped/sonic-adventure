/* Big's last action in EV0170's script, and its motion */
#include <ninja.h>

#include "ev0170/big_action_2.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

extern NJS_MDATA2 dat_0cb8e470[];

static NJS_MOTION ev0170_big_motion_2 = {
#include "ev0170/big_action_2/ev0170_big_motion_2.inc"
};
NJS_ACTION ev0170_big_action_2 = {
#include "ev0170/big_action_2/ev0170_big_action_2.inc"
};
