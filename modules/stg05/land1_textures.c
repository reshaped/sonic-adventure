/* the texture names and texlist of act 1's land, and the materials of its
 * first model */
#include <ninja.h>

#include "stg05/land1_textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 10
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 11
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 11
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 10
#define TEXT_27_LENGTH 12
#define TEXT_28_LENGTH 12
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 11
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 8
#define TEXT_36_LENGTH 10
#define TEXT_37_LENGTH 12
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 10
#define TEXT_40_LENGTH 9
#define TEXT_41_LENGTH 11
#define TEXT_42_LENGTH 8
#define TEXT_43_LENGTH 8
#include "stg05/land1_textures/text.inc"

static NJS_TEXNAME mountain_land1_textures[43] = {
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
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0},
};
NJS_TEXLIST mountain_land1_texlist = {
#include "stg05/land1_textures/mountain_land1_texlist.inc"
};
/* the first model's materials. each file's data starts on a multiple of 8.
 * the materials start 4 bytes past one, right after the texlist, so they are
 * this file's. the model's strips start on a multiple of 8 after them, so
 * the strips can be another file's */
static NJS_MATERIAL dat_0c9aef9c[3] = {
#include "stg05/land1_textures/dat_0c9aef9c.inc"
};
