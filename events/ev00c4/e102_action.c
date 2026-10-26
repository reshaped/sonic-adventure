/* E-102's action and its motion. E-102's model lies in 1ST_READ.BIN. the
 * motion's NJS_MDATA2 table lies in the data before this object */
#include <ninja.h>

#include "ev00c4/e102_action.h"

extern NJS_MDATA2 dat_0cb850a8[];
extern NJS_OBJECT e102_object;

static NJS_MOTION ev00c4_e102_motion = {
#include "ev00c4/e102_action/ev00c4_e102_motion.inc"
};
NJS_ACTION ev00c4_e102_action = {
#include "ev00c4/e102_action/ev00c4_e102_action.inc"
};
