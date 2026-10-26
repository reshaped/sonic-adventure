/* the purple emerald's texture name and its texlist, ev0006_emerald_texlist,
 * which the event loads from M_EM_PURPLE. the material belongs to a node of
 * dat_0cb864f4, the emerald's model, which ev0006_emerald_action moves */
#include <ninja.h>

#include "ev0006/emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev0006/emerald_textures/text.inc"

static NJS_TEXNAME ev0006_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev0006_emerald_texlist = {
#include "ev0006/emerald_textures/ev0006_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cb85714[1] = {
#include "ev0006/emerald_textures/dat_0cb85714.inc"
};
