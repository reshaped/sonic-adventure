/* the texture names and texlists of the coaster's sparkles and the candles'
 * flames */
#include <ninja.h>

#include "stg03/effect_textures.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#include "stg03/effect_textures/text.inc"

static NJS_TEXNAME eff_twinkle_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST eff_twinkle_texlist = {
#include "stg03/effect_textures/eff_twinkle_texlist.inc"
};
static NJS_TEXNAME eff_candle_textures[16] = {
    {TEXT_4, 0, 0},  {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},
    {TEXT_8, 0, 0},  {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0},
    {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
    {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0},
};
NJS_TEXLIST eff_candle_texlist = {
#include "stg03/effect_textures/eff_candle_texlist.inc"
};
