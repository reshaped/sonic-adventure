/* the blue emerald's texture name and texlist, then the first material of
 * a node of the emerald's model. the data after this file holds the node */
#include <ninja.h>

#include "ev008b/blue_emerald_textures.h"

#define TEXT_1_LENGTH 13
#include "ev008b/blue_emerald_textures/text.inc"

static NJS_TEXNAME ev008b_blue_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev008b_blue_emerald_texlist = {
#include "ev008b/blue_emerald_textures/ev008b_blue_emerald_texlist.inc"
};
NJS_MATERIAL dat_0cba5004[1] = {
#include "ev008b/blue_emerald_textures/dat_0cba5004.inc"
};
