/* the names of the emerald's textures, and their texlist */
#include <ninja.h>

#include "stg08/emerald_textures.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 15
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 14
#include "stg08/emerald_textures/text.inc"

NJS_TEXNAME icecap_emerald_textures[6] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
};
NJS_TEXLIST icecap_emerald_texlist = {
#include "stg08/emerald_textures/icecap_emerald_texlist.inc"
};
