/* Sonic's last action in EV0165's script, and its motion */
#include <ninja.h>

#include "ev0165/sonic_action_2.h"

extern NJS_MDATA2 dat_0cb8a700[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0165_sonic_motion_2 = {
#include "ev0165/sonic_action_2/ev0165_sonic_motion_2.inc"
};
NJS_ACTION ev0165_sonic_action_2 = {
#include "ev0165/sonic_action_2/ev0165_sonic_action_2.inc"
};
