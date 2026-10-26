/* Tikal's action and its motion. Tikal's model lies in TIKAL_PROG.PRS. the
 * motion's NJS_MDATA2 table lies in the data before this object */
#include <ninja.h>

#include "ev00dc/tikal_action_2.h"

extern NJS_OBJECT tikal_object;
extern NJS_MDATA2 dat_0cb96198[];

static NJS_MOTION ev00dc_tikal_motion_2 = {
#include "ev00dc/tikal_action_2/ev00dc_tikal_motion_2.inc"
};
NJS_ACTION ev00dc_tikal_action_2 = {
#include "ev00dc/tikal_action_2/ev00dc_tikal_action_2.inc"
};
