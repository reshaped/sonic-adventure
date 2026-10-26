/* the sky's object, the names of its textures, and their texlist */
#include <ninja.h>

#include "stg03/sky_textures.h"

extern NJS_MODEL dat_0c9d5cd8;

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#include "stg03/sky_textures/text.inc"

NJS_OBJECT twinkle_sky_object = {
#include "stg03/sky_textures/twinkle_sky_object.inc"
};
static NJS_TEXNAME bg_shareobj_textures[2] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
};
NJS_TEXLIST bg_shareobj_texlist = {
#include "stg03/sky_textures/bg_shareobj_texlist.inc"
};
