/* ev0032_emerald_texlist and its texture name. the material belongs to a
 * node of dat_0cb8631c, the model ev0032_emerald_action moves */
#include <ninja.h>

#include "ev0032/emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev0032/emerald_textures/text.inc"

static NJS_TEXNAME ev0032_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev0032_emerald_texlist = {
#include "ev0032/emerald_textures/ev0032_emerald_texlist.inc"
};
NJS_MATERIAL dat_0cb8553c[1] = {
#include "ev0032/emerald_textures/dat_0cb8553c.inc"
};
