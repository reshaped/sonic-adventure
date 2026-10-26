/* Big's last action in EV0171's script, and its motion */
#include <ninja.h>

#include "ev0171/big_action_2.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

extern NJS_MDATA2 dat_0cb8e4c0[];

static NJS_MOTION ev0171_big_motion_2 = {
#include "ev0171/big_action_2/ev0171_big_motion_2.inc"
};
NJS_ACTION ev0171_big_action_2 = {
#include "ev0171/big_action_2/ev0171_big_action_2.inc"
};
