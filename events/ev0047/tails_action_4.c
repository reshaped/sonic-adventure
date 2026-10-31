/* ev0047_tails_action_4 and its motion, then the texture name of
 * ev0047_emerald_texlist */
#include <ninja.h>

#include "ev0047/tails_action_4.h"

#define TEXT_1_LENGTH 14
#include "ev0047/tails_action_4/text.inc"

extern NJS_MDATA2 dat_0cba3798[];
extern NJS_OBJECT tails_object;

static NJS_MOTION ev0047_tails_motion_4 = {
#include "ev0047/tails_action_4/ev0047_tails_motion_4.inc"
};
NJS_ACTION ev0047_tails_action_4 = {
#include "ev0047/tails_action_4/ev0047_tails_action_4.inc"
};
NJS_TEXNAME ev0047_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
