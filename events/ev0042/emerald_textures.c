/* ev0042_emerald_texlist and its texture name, then the material of one
 * node of the model ev0042_emerald_action moves */
#include <ninja.h>

#include "ev0042/emerald_textures.h"

#define TEXT_1_LENGTH 14
#include "ev0042/emerald_textures/text.inc"

static NJS_TEXNAME ev0042_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev0042_emerald_texlist = {
#include "ev0042/emerald_textures/ev0042_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cb98da4[1] = {
#include "ev0042/emerald_textures/dat_0cb98da4.inc"
};
