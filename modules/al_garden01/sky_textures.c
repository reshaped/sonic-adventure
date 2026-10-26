/* the texture list of the garden's sky, and its texture names */
#include <ninja.h>

#include "al_garden01/sky_textures.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 15
#define TEXT_7_LENGTH 15
#define TEXT_8_LENGTH 15
#include "al_garden01/sky_textures/text.inc"

static NJS_TEXNAME sky_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST egg_carrier_garden_sky_texlist = {
#include "al_garden01/sky_textures/egg_carrier_garden_sky_texlist.inc"
};
