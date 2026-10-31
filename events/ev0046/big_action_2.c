/* ev0046_big_action_2 and its motion, then ev0046_frog_texlist and its
 * texture names. the file ends on the material of one node of the frog's
 * model */
#include <ninja.h>

#include "ev0046/big_action_2.h"
/* ev0046_big_action_2's full body names big_object. lint reads its stub
 * body, which doesn't */
#include "player/big_models.h" /* NOLINT(misc-include-cleaner) */

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#include "ev0046/big_action_2/text.inc"

extern NJS_MDATA2 dat_0cb9e778[];

static NJS_MOTION ev0046_big_motion_2 = {
#include "ev0046/big_action_2/ev0046_big_motion_2.inc"
};
NJS_ACTION ev0046_big_action_2 = {
#include "ev0046/big_action_2/ev0046_big_action_2.inc"
};
static NJS_TEXNAME ev0046_frog_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST ev0046_frog_texlist = {
#include "ev0046/big_action_2/ev0046_frog_texlist.inc"
};
NJS_MATERIAL dat_0cb9ec84[1] = {
#include "ev0046/big_action_2/dat_0cb9ec84.inc"
};
