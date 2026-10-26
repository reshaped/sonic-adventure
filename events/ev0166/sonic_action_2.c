/* Sonic's last action in EV0166's script, and its motion */
#include <ninja.h>

#include "ev0166/sonic_action_2.h"

extern NJS_MDATA2 dat_0cb8b608[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0166_sonic_motion_2 = {
#include "ev0166/sonic_action_2/ev0166_sonic_motion_2.inc"
};
NJS_ACTION ev0166_sonic_action_2 = {
#include "ev0166/sonic_action_2/ev0166_sonic_action_2.inc"
};
