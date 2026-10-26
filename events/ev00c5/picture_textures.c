/* the picture's texture name and list */
#include <ninja.h>

#include "ev00c5/picture_textures.h"

#define TEXT_1_LENGTH 7
#include "ev00c5/picture_textures/text.inc"

static NJS_TEXNAME ev00c5_picture_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev00c5_picture_texlist = {
#include "ev00c5/picture_textures/ev00c5_picture_texlist.inc"
};
