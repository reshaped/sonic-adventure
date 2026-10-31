/* ev00f7_emerald_red_texlist and its texture name, then the material of one
 * node of the blue emerald's model */
#include <ninja.h>

#include "ev00f7/emerald_red_textures.h"

#define TEXT_1_LENGTH 14
#include "ev00f7/emerald_red_textures/text.inc"

static NJS_TEXNAME ev00f7_emerald_red_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev00f7_emerald_red_texlist = {
#include "ev00f7/emerald_red_textures/ev00f7_emerald_red_texlist.inc"
};
NJS_MATERIAL dat_0cbeb5a4[1] = {
#include "ev00f7/emerald_red_textures/dat_0cbeb5a4.inc"
};
