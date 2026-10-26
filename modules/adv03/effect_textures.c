/* EFF_PAST's 24 textures, bigfire01 to 16 then watersmoke01 to 08, and
 * their texlist, which past_pvms names. past_fire_texlist holds the first 16,
 * the fire's */
#include <ninja.h>

#include "adv03/effect_textures.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 12
#define TEXT_18_LENGTH 12
#define TEXT_19_LENGTH 12
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 12
#include "adv03/effect_textures/text.inc"

static NJS_TEXNAME eff_past_textures[24] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
};
NJS_TEXLIST eff_past_texlist = {
#include "adv03/effect_textures/eff_past_texlist.inc"
};
NJS_TEXLIST past_fire_texlist = {
#include "adv03/effect_textures/past_fire_texlist.inc"
};
