/* ev00fe_bird_texlist and its texture names, and a material of the bird's
 * model */
#include <ninja.h>

#include "ev00fe/bird_textures.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#include "ev00fe/bird_textures/text.inc"

static NJS_TEXNAME ev00fe_bird_textures[7] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0},
};
NJS_TEXLIST ev00fe_bird_texlist = {
#include "ev00fe/bird_textures/ev00fe_bird_texlist.inc"
};
static NJS_MATERIAL dat_0cba7b4c[1] = {
#include "ev00fe/bird_textures/dat_0cba7b4c.inc"
};
