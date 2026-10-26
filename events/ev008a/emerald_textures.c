/* the green emerald's texture name and texlist, then the first material of
 * a node of the emerald's model. the data after this file holds the node */
#include <ninja.h>

#include "ev008a/emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev008a/emerald_textures/text.inc"

static NJS_TEXNAME ev008a_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev008a_emerald_texlist = {
#include "ev008a/emerald_textures/ev008a_emerald_texlist.inc"
};
static NJS_MATERIAL dat_0cb9e364[1] = {
#include "ev008a/emerald_textures/dat_0cb9e364.inc"
};
