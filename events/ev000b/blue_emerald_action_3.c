/* the action the blue emerald plays once as Eggman gives the emeralds to
 * Chaos 2, and its motion. the green emerald's texture name table follows.
 * its texlist, ev000b_green_emerald_texlist, starts the data after this
 * file. the event loads it from M_EM_GREEN */
#include <ninja.h>

#include "ev000b/blue_emerald_action_3.h"

#define TEXT_1_LENGTH 15
#include "ev000b/blue_emerald_action_3/text.inc"

extern NJS_OBJECT dat_0cba2100;
extern NJS_MDATA2 dat_0cba3190[];

static NJS_MOTION ev000b_blue_emerald_motion_3 = {
#include "ev000b/blue_emerald_action_3/ev000b_blue_emerald_motion_3.inc"
};
NJS_ACTION ev000b_blue_emerald_action_3 = {
#include "ev000b/blue_emerald_action_3/ev000b_blue_emerald_action_3.inc"
};
NJS_TEXNAME ev000b_green_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
