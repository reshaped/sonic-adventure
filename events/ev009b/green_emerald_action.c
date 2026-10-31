/* the green emerald's action and its motion, then the purple emerald's
 * texture name. the data after this file holds the purple emerald's texlist */
#include <ninja.h>

#include "ev009b/green_emerald_action.h"

#define TEXT_1_LENGTH 15
#include "ev009b/green_emerald_action/text.inc"

extern NJS_OBJECT dat_0cb837bc;
extern NJS_MDATA3 dat_0cb83910[];

static NJS_MOTION ev009b_green_emerald_motion = {
#include "ev009b/green_emerald_action/ev009b_green_emerald_motion.inc"
};
NJS_ACTION ev009b_green_emerald_action[1] = {
#include "ev009b/green_emerald_action/ev009b_green_emerald_action.inc"
};
NJS_TEXNAME ev009b_purple_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
