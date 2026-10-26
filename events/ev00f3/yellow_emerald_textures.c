/* ev00f3_yellow_emerald_texlist and its texture name, then the material of
 * one node of the blue emerald's model */
#include <ninja.h>

#include "ev00f3/yellow_emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev00f3/yellow_emerald_textures/text.inc"

static NJS_TEXNAME ev00f3_yellow_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev00f3_yellow_emerald_texlist = {
#include "ev00f3/yellow_emerald_textures/ev00f3_yellow_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cb9843c[1] = {
#include "ev00f3/yellow_emerald_textures/dat_0cb9843c.inc"
};
