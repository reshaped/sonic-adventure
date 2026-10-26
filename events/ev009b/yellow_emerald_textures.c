/* the yellow emerald's texture name and texlist, then the first material of
 * a node of the emerald's model. the data after this file holds the node.
 * the script draws the yellow emerald with the purple emerald's action, which
 * moves another model */
#include <ninja.h>

#include "ev009b/yellow_emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev009b/yellow_emerald_textures/text.inc"

static NJS_TEXNAME ev009b_yellow_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev009b_yellow_emerald_texlist = {
#include "ev009b/yellow_emerald_textures/ev009b_yellow_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cb8692c[1] = {
#include "ev009b/yellow_emerald_textures/dat_0cb8692c.inc"
};
