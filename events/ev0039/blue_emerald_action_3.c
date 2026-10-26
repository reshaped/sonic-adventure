/* ev0039_blue_emerald_action_3 and its motion, then the texture name of
 * ev0039_green_emerald_texlist */
#include <ninja.h>

#include "ev0039/blue_emerald_action_3.h"

#define TEXT_1_LENGTH 15
#include "ev0039/blue_emerald_action_3/text.inc"

extern NJS_OBJECT dat_0cba2c28;
extern NJS_MDATA2 dat_0cba3cb8[];

static NJS_MOTION ev0039_blue_emerald_motion_3 = {
#include "ev0039/blue_emerald_action_3/ev0039_blue_emerald_motion_3.inc"
};
NJS_ACTION ev0039_blue_emerald_action_3 = {
#include "ev0039/blue_emerald_action_3/ev0039_blue_emerald_action_3.inc"
};
static NJS_TEXNAME ev0039_green_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
