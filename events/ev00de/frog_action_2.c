/* ev00de_frog_action_2 and its motion, then the texture name of
 * ev00de_emerald_yellow_texlist */
#include <ninja.h>

#include "ev00de/frog_action_2.h"

#define TEXT_1_LENGTH 15
#include "ev00de/frog_action_2/text.inc"

extern NJS_MDATA2 dat_0cba35d0[];
extern NJS_OBJECT dat_0cbc6618;

static NJS_MOTION ev00de_frog_motion_2 = {
#include "ev00de/frog_action_2/ev00de_frog_motion_2.inc"
};
NJS_ACTION ev00de_frog_action_2 = {
#include "ev00de/frog_action_2/ev00de_frog_action_2.inc"
};
static NJS_TEXNAME ev00de_emerald_yellow_textures[1] = {
    {TEXT_1, 0, 0},
};
