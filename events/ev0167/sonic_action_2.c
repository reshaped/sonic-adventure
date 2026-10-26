/* Sonic's last action in EV0167's script, and its motion */
#include <ninja.h>

#include "ev0167/sonic_action_2.h"

extern NJS_MDATA2 dat_0cb8da00[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0167_sonic_motion_2 = {
#include "ev0167/sonic_action_2/ev0167_sonic_motion_2.inc"
};
NJS_ACTION ev0167_sonic_action_2 = {
#include "ev0167/sonic_action_2/ev0167_sonic_action_2.inc"
};
