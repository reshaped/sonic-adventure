/* ev0039_sonic_action_1 and its motion, then the texture name of
 * ev0039_blue_emerald_texlist */
#include <ninja.h>

#include "ev0039/sonic_action_1.h"

#define TEXT_1_LENGTH 13
#include "ev0039/sonic_action_1/text.inc"

extern NJS_MDATA2 dat_0cba1990[];
extern NJS_OBJECT sonic_object;

static NJS_MOTION ev0039_sonic_motion_1 = {
#include "ev0039/sonic_action_1/ev0039_sonic_motion_1.inc"
};
NJS_ACTION ev0039_sonic_action_1 = {
#include "ev0039/sonic_action_1/ev0039_sonic_action_1.inc"
};
static NJS_TEXNAME ev0039_blue_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
