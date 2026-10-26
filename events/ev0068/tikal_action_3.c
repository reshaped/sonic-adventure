/* Tikal's ev0068_tikal_action_3 and its motion, then the texture name that
 * event_chaos0_water_texlist lists. the texlist lies in the data after this
 * object. the motion's NJS_MDATA2 table lies in the data before this object.
 * Tikal's model, tikal_object, lies in TIKAL_PROG.PRS */
#include <ninja.h>

#include "ev0068/tikal_action_3.h"

#define TEXT_1_LENGTH 11
#include "ev0068/tikal_action_3/text.inc"

extern NJS_OBJECT tikal_object;
extern NJS_MDATA2 dat_0cba5120[];

static NJS_MOTION ev0068_tikal_motion_3 = {
#include "ev0068/tikal_action_3/ev0068_tikal_motion_3.inc"
};
NJS_ACTION ev0068_tikal_action_3 = {
#include "ev0068/tikal_action_3/ev0068_tikal_action_3.inc"
};
static NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_1, 0, 0},
};
