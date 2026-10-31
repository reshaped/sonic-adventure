/* the Egg Mobile's action as the event opens, and its motion. the texture
 * name table of event_chaos0_water_texlist follows. the texlist starts the
 * data after this file. the event loads it from EV_CHAOS0_MANJU */
#include <ninja.h>

#include "ev0008/eggmobile_action_7.h"

#define TEXT_1_LENGTH 11
#include "ev0008/eggmobile_action_7/text.inc"

extern NJS_MDATA2 dat_0cba8e60[];
extern NJS_OBJECT event_eggmobile_object;

static NJS_MOTION ev0008_eggmobile_motion_7 = {
#include "ev0008/eggmobile_action_7/ev0008_eggmobile_motion_7.inc"
};
NJS_ACTION ev0008_eggmobile_action_7 = {
#include "ev0008/eggmobile_action_7/ev0008_eggmobile_action_7.inc"
};
NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_1, 0, 0},
};
