/* ev00f3_eggman_action and its motion, then the texture name of
 * ev00f3_blue_emerald_texlist */
#include <ninja.h>

#include "ev00f3/eggman_action.h"

#define TEXT_1_LENGTH 13
#include "ev00f3/eggman_action/text.inc"

extern NJS_MDATA2 dat_0cb98230[];
extern NJS_OBJECT eggman_object;

static NJS_MOTION ev00f3_eggman_motion = {
#include "ev00f3/eggman_action/ev00f3_eggman_motion.inc"
};
NJS_ACTION ev00f3_eggman_action = {
#include "ev00f3/eggman_action/ev00f3_eggman_action.inc"
};
static NJS_TEXNAME ev00f3_blue_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
