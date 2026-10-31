/* ev00dd_big_action and its motion, ev00dd_frog_texlist and its texture
 * names, then the material of one node of Froggy's model */
#include <ninja.h>

#include "ev00dd/big_action.h"
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#include "ev00dd/big_action/text.inc"

extern NJS_MDATA2 dat_0cb82120[];

static NJS_MOTION ev00dd_big_motion = {
#include "ev00dd/big_action/ev00dd_big_motion.inc"
};
NJS_ACTION ev00dd_big_action = {
#include "ev00dd/big_action/ev00dd_big_action.inc"
};
static NJS_TEXNAME ev00dd_frog_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST ev00dd_frog_texlist = {
#include "ev00dd/big_action/ev00dd_frog_texlist.inc"
};
NJS_MATERIAL dat_0cb8262c[1] = {
#include "ev00dd/big_action/dat_0cb8262c.inc"
};
