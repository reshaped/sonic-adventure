/* the white emerald's texture name and its texlist, ev0009_emerald_texlist,
 * which the event loads from M_EM_WHITE. the material belongs to a node of
 * dat_0cbab67c, the emerald's model, which ev0009_emerald_action_0 and
 * ev0009_emerald_action_1 move */
#include <ninja.h>

#include "ev0009/emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev0009/emerald_textures/text.inc"

static NJS_TEXNAME ev0009_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev0009_emerald_texlist = {
#include "ev0009/emerald_textures/ev0009_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cbaa89c[1] = {
#include "ev0009/emerald_textures/dat_0cbaa89c.inc"
};
