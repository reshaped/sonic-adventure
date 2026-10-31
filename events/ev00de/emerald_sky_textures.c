/* ev00de_emerald_sky_texlist and its texture name, then the material of one
 * node of dat_0cba549c, the model dat_0cba5668.c's action moves */
#include <ninja.h>

#include "ev00de/emerald_sky_textures.h"

#define TEXT_1_LENGTH 14
#include "ev00de/emerald_sky_textures/text.inc"

static NJS_TEXNAME ev00de_emerald_sky_textures[1] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST ev00de_emerald_sky_texlist = {
#include "ev00de/emerald_sky_textures/ev00de_emerald_sky_texlist.inc"
};
NJS_MATERIAL dat_0cba46bc[1] = {
#include "ev00de/emerald_sky_textures/dat_0cba46bc.inc"
};
