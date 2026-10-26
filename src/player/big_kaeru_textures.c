/* the texture names and the texlist of Froggy, Big's frog */
#include <ninja.h>

#include "player/big_kaeru_textures.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#include "player/big_kaeru_textures/text.inc"

static NJS_TEXNAME big_kaeru_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST big_kaeru_texlist = {
#include "player/big_kaeru_textures/big_kaeru_texlist.inc"
};
