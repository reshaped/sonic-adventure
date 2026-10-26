/* the boss's effects: egm3spr's 35 textures and their texlist. six sprites
 * draw from them, each with its frames, and egg_viper_sprites lists the
 * sprites */
#include <ninja.h>

#include "b_egm3/effects.h"
#include "b_egm3/egg_viper.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 14
#define TEXT_13_LENGTH 14
#define TEXT_14_LENGTH 14
#define TEXT_15_LENGTH 14
#define TEXT_16_LENGTH 14
#define TEXT_17_LENGTH 14
#define TEXT_18_LENGTH 14
#define TEXT_19_LENGTH 14
#define TEXT_20_LENGTH 11
#define TEXT_21_LENGTH 11
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 11
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 11
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 10
#define TEXT_33_LENGTH 10
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 10
#include "b_egm3/effects/text.inc"

static NJS_TEXNAME egg_viper_effect_textures[35] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0},
};
NJS_TEXLIST egg_viper_effect_texlist = {
#include "b_egm3/effects/egg_viper_effect_texlist.inc"
};
static NJS_TEXANIM dat_0c957914[16] = {
#include "b_egm3/effects/dat_0c957914.inc"
};
static NJS_SPRITE dat_0c957a54 = {
#include "b_egm3/effects/dat_0c957a54.inc"
};
static struct egg_viper_sprite dat_0c957a74 = {
#include "b_egm3/effects/dat_0c957a74.inc"
};
static NJS_TEXANIM dat_0c957a80[8] = {
#include "b_egm3/effects/dat_0c957a80.inc"
};
static NJS_SPRITE dat_0c957b20 = {
#include "b_egm3/effects/dat_0c957b20.inc"
};
static struct egg_viper_sprite dat_0c957b40 = {
#include "b_egm3/effects/dat_0c957b40.inc"
};
static NJS_TEXANIM dat_0c957b4c[1] = {
#include "b_egm3/effects/dat_0c957b4c.inc"
};
static NJS_SPRITE dat_0c957b60 = {
#include "b_egm3/effects/dat_0c957b60.inc"
};
static struct egg_viper_sprite dat_0c957b80 = {
#include "b_egm3/effects/dat_0c957b80.inc"
};
static NJS_TEXANIM dat_0c957b8c[1] = {
#include "b_egm3/effects/dat_0c957b8c.inc"
};
static NJS_SPRITE dat_0c957ba0 = {
#include "b_egm3/effects/dat_0c957ba0.inc"
};
static struct egg_viper_sprite dat_0c957bc0 = {
#include "b_egm3/effects/dat_0c957bc0.inc"
};
static NJS_TEXANIM dat_0c957bcc[8] = {
#include "b_egm3/effects/dat_0c957bcc.inc"
};
static NJS_SPRITE dat_0c957c6c = {
#include "b_egm3/effects/dat_0c957c6c.inc"
};
static struct egg_viper_sprite dat_0c957c8c = {
#include "b_egm3/effects/dat_0c957c8c.inc"
};
static NJS_TEXANIM dat_0c957c98[12] = {
#include "b_egm3/effects/dat_0c957c98.inc"
};
static NJS_SPRITE dat_0c957d88 = {
#include "b_egm3/effects/dat_0c957d88.inc"
};
static struct egg_viper_sprite dat_0c957da8 = {
#include "b_egm3/effects/dat_0c957da8.inc"
};
struct egg_viper_sprite *egg_viper_sprites[7] = {
    &dat_0c957a74, &dat_0c957b40, &dat_0c957b80, &dat_0c957bc0,
    &dat_0c957c8c, &dat_0c957da8, NULL,
};
