/* the names of E-103's textures, and its texlist */
#include <ninja.h>

#include "stg02/e103.h"
#include "stg02/e103_textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 12
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 11
#define TEXT_32_LENGTH 10
#define TEXT_33_LENGTH 10
#define TEXT_34_LENGTH 11
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 12
#define TEXT_38_LENGTH 13
#define TEXT_39_LENGTH 14
#define TEXT_40_LENGTH 12
#define TEXT_41_LENGTH 9
#include "stg02/e103_textures/text.inc"

static NJS_TEXNAME windy_e103_textures[41] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0},
};
NJS_TEXLIST windy_e103_texlist = {
#include "stg02/e103_textures/windy_e103_texlist.inc"
};
/* the material of the first model after it, E-103's. SHC starts each object's
 * data on a multiple of 8. the material starts right after the texlist, 4
 * bytes past one, so it is this file's */
NJS_MATERIAL dat_0c9ec72c[1] = {
#include "stg02/e103_textures/dat_0c9ec72c.inc"
};
