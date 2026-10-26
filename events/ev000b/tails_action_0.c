/* the action Tails repeats throughout the event, and its motion. the blue
 * emerald's texture name table follows. its texlist,
 * ev000b_blue_emerald_texlist, starts the data after this file. the event
 * loads it from M_EM_BLUE */
#include <ninja.h>

#include "ev000b/tails_action_0.h"

#define TEXT_1_LENGTH 13
#include "ev000b/tails_action_0/text.inc"

extern NJS_MDATA2 dat_0cba0eb8[];
extern NJS_OBJECT tails_object;

static NJS_MOTION ev000b_tails_motion_0 = {
#include "ev000b/tails_action_0/ev000b_tails_motion_0.inc"
};
NJS_ACTION ev000b_tails_action_0 = {
#include "ev000b/tails_action_0/ev000b_tails_action_0.inc"
};
static NJS_TEXNAME ev000b_blue_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
