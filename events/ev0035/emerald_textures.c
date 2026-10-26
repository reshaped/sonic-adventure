/* ev0035_emerald_texlist and its texture name. the material belongs to a
 * node of dat_0cb99cac, the model script.c's emerald actions move */
#include <ninja.h>

#include "ev0035/emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev0035/emerald_textures/text.inc"

static NJS_TEXNAME ev0035_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev0035_emerald_texlist = {
#include "ev0035/emerald_textures/ev0035_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cb98ecc[1] = {
#include "ev0035/emerald_textures/dat_0cb98ecc.inc"
};
