/* ev0046_emerald_texlist and its texture name, then the material of one
 * node of the model ev0046_emerald_action_4 moves */
#include <ninja.h>

#include "ev0046/emerald_textures.h"

#define TEXT_1_LENGTH 14
#include "ev0046/emerald_textures/text.inc"

static NJS_TEXNAME ev0046_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev0046_emerald_texlist = {
#include "ev0046/emerald_textures/ev0046_emerald_texlist.inc"
};
NJS_MATERIAL dat_0cba8694[1] = {
#include "ev0046/emerald_textures/dat_0cba8694.inc"
};
