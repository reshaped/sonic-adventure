/* Big's action and its motion, then Froggy's texture names and list. the
 * motion's NJS_MDATA2 table lies in the data before this object. the
 * NJS_MATERIAL at the end belongs to Froggy's model, in the data after this
 * object */
#include <ninja.h>

#include "ev00d3/big_action_1.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#include "ev00d3/big_action_1/text.inc"

extern NJS_MDATA2 dat_0cb924f0[];

static NJS_MOTION ev00d3_big_motion_1 = {
#include "ev00d3/big_action_1/ev00d3_big_motion_1.inc"
};
NJS_ACTION ev00d3_big_action_1 = {
#include "ev00d3/big_action_1/ev00d3_big_action_1.inc"
};
static NJS_TEXNAME ev00d3_frog_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST ev00d3_frog_texlist = {
#include "ev00d3/big_action_1/ev00d3_frog_texlist.inc"
};
NJS_MATERIAL dat_0cb929fc[1] = {
#include "ev00d3/big_action_1/dat_0cb929fc.inc"
};
