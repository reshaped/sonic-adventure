/* Big's action and its motion, then Froggy's texture names and list. the
 * motion's NJS_MDATA2 table lies in the data before this object. the
 * NJS_MATERIAL at the end belongs to Froggy's model, in the data after this
 * object */
#include <ninja.h>

#include "ev00d2/big_action.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#include "ev00d2/big_action/text.inc"

extern NJS_MDATA2 dat_0cb83528[];

static NJS_MOTION ev00d2_big_motion = {
#include "ev00d2/big_action/ev00d2_big_motion.inc"
};
NJS_ACTION ev00d2_big_action = {
#include "ev00d2/big_action/ev00d2_big_action.inc"
};
static NJS_TEXNAME ev00d2_frog_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST ev00d2_frog_texlist = {
#include "ev00d2/big_action/ev00d2_frog_texlist.inc"
};
static NJS_MATERIAL dat_0cb83a34[1] = {
#include "ev00d2/big_action/dat_0cb83a34.inc"
};
