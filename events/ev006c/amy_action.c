/* Amy's action and its motion. Amy's model lies in 1ST_READ.BIN. the
 * motion's NJS_MDATA2 table lies in the data before this object */
#include <ninja.h>

#include "ev006c/amy_action.h"

extern NJS_MDATA2 dat_0cb82f40[];
extern NJS_OBJECT amy_object;

static NJS_MOTION ev006c_amy_motion = {
#include "ev006c/amy_action/ev006c_amy_motion.inc"
};
NJS_ACTION ev006c_amy_action = {
#include "ev006c/amy_action/ev006c_amy_action.inc"
};
