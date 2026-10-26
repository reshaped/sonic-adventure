/* ev001e_sonic_action_6 and its motion. sonic_object, the model the action
 * moves, lies in 1ST_READ.BIN. dat_0cb9ab00, the motion's NJS_MDATA2 table,
 * lies in the data before this object */
#include <ninja.h>

#include "ev001e/sonic_action_6.h"

extern NJS_MDATA2 dat_0cb9ab00[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev001e_sonic_motion_6 = {
#include "ev001e/sonic_action_6/ev001e_sonic_motion_6.inc"
};
NJS_ACTION ev001e_sonic_action_6 = {
#include "ev001e/sonic_action_6/ev001e_sonic_action_6.inc"
};
