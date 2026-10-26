/* ev0023_sonic_action_4 and its motion. sonic_object, the model the action
 * moves, lies in 1ST_READ.BIN. dat_0cb8d6b0, the motion's NJS_MDATA2 table,
 * lies in the data before this object */
#include <ninja.h>

#include "ev0023/sonic_action_4.h"

extern NJS_MDATA2 dat_0cb8d6b0[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0023_sonic_motion_4 = {
#include "ev0023/sonic_action_4/ev0023_sonic_motion_4.inc"
};
NJS_ACTION ev0023_sonic_action_4 = {
#include "ev0023/sonic_action_4/ev0023_sonic_action_4.inc"
};
