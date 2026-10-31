/* Big's action and its motion, then Froggy's texture names and list. the
 * motion's NJS_MDATA2 table lies in the data before this object. the
 * NJS_MATERIAL at the end belongs to Froggy's model, in the data after this
 * object */
#include <ninja.h>

#include "ev00da/big_action_2.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#include "ev00da/big_action_2/text.inc"

extern NJS_MDATA2 dat_0cb8a778[];

static NJS_MOTION ev00da_big_motion_2 = {
#include "ev00da/big_action_2/ev00da_big_motion_2.inc"
};
NJS_ACTION ev00da_big_action_2 = {
#include "ev00da/big_action_2/ev00da_big_action_2.inc"
};
static NJS_TEXNAME ev00da_frog_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST ev00da_frog_texlist = {
#include "ev00da/big_action_2/ev00da_frog_texlist.inc"
};
NJS_MATERIAL dat_0cb8ac84[1] = {
#include "ev00da/big_action_2/dat_0cb8ac84.inc"
};
