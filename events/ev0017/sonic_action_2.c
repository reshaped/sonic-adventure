/* ev0017_sonic_action_2 and its motion. sonic_object, the model the action
 * moves, lies in 1ST_READ.BIN. dat_0cbacb68, the motion's NJS_MDATA2 table,
 * lies in the data before this object */
#include <ninja.h>

#include "ev0017/sonic_action_2.h"

extern NJS_MDATA2 dat_0cbacb68[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0017_sonic_motion_2 = {
#include "ev0017/sonic_action_2/ev0017_sonic_motion_2.inc"
};
NJS_ACTION ev0017_sonic_action_2 = {
#include "ev0017/sonic_action_2/ev0017_sonic_action_2.inc"
};
