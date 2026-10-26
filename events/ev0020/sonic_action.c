/* ev0020_sonic_action and its motion. sonic_object, the model the action
 * moves, lies in 1ST_READ.BIN. dat_0cb85558, the motion's NJS_MDATA2 table,
 * lies in the data before this object */
#include <ninja.h>

#include "ev0020/sonic_action.h"

extern NJS_MDATA2 dat_0cb85558[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0020_sonic_motion = {
#include "ev0020/sonic_action/ev0020_sonic_motion.inc"
};
NJS_ACTION ev0020_sonic_action = {
#include "ev0020/sonic_action/ev0020_sonic_action.inc"
};
