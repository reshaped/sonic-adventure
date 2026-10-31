/* the action Tails repeats after the claw snatches the emerald from him,
 * and its motion. the purple emerald's texture name table follows. its
 * texlist, ev0008_emerald_texlist, starts the data after this file. the
 * event loads it from M_EM_PURPLE */
#include <ninja.h>

#include "ev0008/tails_action_3.h"

#define TEXT_1_LENGTH 15
#include "ev0008/tails_action_3/text.inc"

extern NJS_MDATA2 dat_0cb9bc70[];
extern NJS_OBJECT tails_object;

static NJS_MOTION ev0008_tails_motion_3 = {
#include "ev0008/tails_action_3/ev0008_tails_motion_3.inc"
};
NJS_ACTION ev0008_tails_action_3 = {
#include "ev0008/tails_action_3/ev0008_tails_action_3.inc"
};
NJS_TEXNAME ev0008_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
