/* the white emerald's action and its motion, then the blue emerald's
 * texture name. the data after this file holds the blue emerald's texlist */
#include <ninja.h>

#include "ev009b/white_emerald_action.h"

#define TEXT_1_LENGTH 13
#include "ev009b/white_emerald_action/text.inc"

extern NJS_OBJECT dat_0cb81814;
extern NJS_MDATA3 dat_0cb81968[];

static NJS_MOTION ev009b_white_emerald_motion = {
#include "ev009b/white_emerald_action/ev009b_white_emerald_motion.inc"
};
NJS_ACTION ev009b_white_emerald_action = {
#include "ev009b/white_emerald_action/ev009b_white_emerald_action.inc"
};
static NJS_TEXNAME ev009b_blue_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
