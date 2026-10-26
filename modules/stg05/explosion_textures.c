/* the names of the explosion's textures, and its texlist */
#include <ninja.h>

#include "stg05/explosion_textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#include "stg05/explosion_textures/text.inc"

static NJS_TEXNAME mexplosion_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST mexplosion_texlist = {
#include "stg05/explosion_textures/mexplosion_texlist.inc"
};
