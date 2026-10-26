/* Eggman's textures, from EGGMAN, and a material that only a model in the
 * data after them names. that model counts no materials, though its one
 * meshset draws with this one */
#include <ninja.h>

#include "player/textures.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 5
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 6
#define TEXT_19_LENGTH 6
#include "player/eggman_textures/text.inc"

static NJS_TEXNAME eggman_textures[19] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0},
};
NJS_TEXLIST eggman_texlist = {
#include "player/eggman_textures/eggman_texlist.inc"
};
static NJS_MATERIAL dat_8c586e24[1] = {
#include "player/eggman_textures/dat_8c586e24.inc"
};
