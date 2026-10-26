/* the eight textures the blasts' ball cycles through, and their texlist */
#include <ninja.h>

#include "b_egm2/baku_textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#include "b_egm2/baku_textures/text.inc"

static NJS_TEXNAME egm2_baku_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST egm2_baku_texlist = {
#include "b_egm2/baku_textures/egm2_baku_texlist.inc"
};
