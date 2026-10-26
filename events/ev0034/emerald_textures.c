/* ev0034_emerald_texlist and its texture name. the material belongs to a
 * node of dat_0cb9e3cc, the model script.c's emerald actions move */
#include <ninja.h>

#include "ev0034/emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev0034/emerald_textures/text.inc"

static NJS_TEXNAME ev0034_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev0034_emerald_texlist[1] = {
#include "ev0034/emerald_textures/ev0034_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cb9d5ec[1] = {
#include "ev0034/emerald_textures/dat_0cb9d5ec.inc"
};
