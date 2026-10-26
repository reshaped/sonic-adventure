/* the emerald's action and its motion. the model the action moves and the
 * motion's NJS_MDATA3 table lie in the data before this object. the texture
 * name at the end belongs to event_chaos0_water_texlist, in the data after
 * this object */
#include <ninja.h>

#include "ev00d0/emerald_action.h"

#define TEXT_1_LENGTH 11
#include "ev00d0/emerald_action/text.inc"

extern NJS_OBJECT dat_0cbbd5e4;
extern NJS_MDATA3 dat_0cbbd738[];

static NJS_MOTION ev00d0_emerald_motion = {
#include "ev00d0/emerald_action/ev00d0_emerald_motion.inc"
};
NJS_ACTION ev00d0_emerald_action = {
#include "ev00d0/emerald_action/ev00d0_emerald_action.inc"
};
static NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_1, 0, 0},
};
