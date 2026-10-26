/* the names of the sky's textures, and their texlist */
#include <ninja.h>

#include "stg08/sky_textures.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 11
#include "stg08/sky_textures/text.inc"

static NJS_TEXNAME icecap_sky_textures[4] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
    {TEXT_4, 0, 0},
};
NJS_TEXLIST icecap_sky_texlist = {
#include "stg08/sky_textures/icecap_sky_texlist.inc"
};
