/* the Chao's texlist, the names of its eleven textures, and a material that
 * nothing reads */
#include <ninja.h>

#include "shared/chao_textures.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 8
#include "shared/chao_textures/text.inc"

static NJS_TEXNAME event_chao_textures[11] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0},
};
NJS_TEXLIST event_chao_texlist = {
#include "shared/chao_textures/event_chao_texlist.inc"
};
static NJS_MATERIAL event_chao_material[1] = {
#include "shared/chao_textures/event_chao_material.inc"
};
