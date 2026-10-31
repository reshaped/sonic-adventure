/* the emerald's texture name and list. the NJS_MATERIAL at the end belongs to
 * the emerald's model, in the data after this object */
#include <ninja.h>

#include "ev00d0/emerald_textures.h"

#define TEXT_1_LENGTH 15
#include "ev00d0/emerald_textures/text.inc"

static NJS_TEXNAME ev00d0_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev00d0_emerald_texlist = {
#include "ev00d0/emerald_textures/ev00d0_emerald_texlist.inc"
};
NJS_MATERIAL dat_0cbbc804[1] = {
#include "ev00d0/emerald_textures/dat_0cbbc804.inc"
};
