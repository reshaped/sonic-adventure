/* ev00fe_bird_action and its motion, and the texture names of
 * event_chaos0_water_texlist */
#include <ninja.h>

#include "ev00fe/bird_action.h"

#define TEXT_1_LENGTH 11
#include "ev00fe/bird_action/text.inc"

extern NJS_OBJECT dat_0cbaac64;
extern NJS_MDATA2 dat_0cbac3d8[];

static NJS_MOTION ev00fe_bird_motion = {
#include "ev00fe/bird_action/ev00fe_bird_motion.inc"
};
NJS_ACTION ev00fe_bird_action = {
#include "ev00fe/bird_action/ev00fe_bird_action.inc"
};
NJS_TEXNAME event_chaos0_water_textures[1] = {
    {TEXT_1, 0, 0},
};
