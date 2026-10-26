/* HOTSHELTER1's and SHELTER_COLUMN's texture names and texlists, and a
 * material after them */
#include <ninja.h>

#include "stg12/hotshelter1_textures.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 10
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 11
#define TEXT_24_LENGTH 12
#define TEXT_25_LENGTH 18
#define TEXT_26_LENGTH 12
#define TEXT_27_LENGTH 12
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 11
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 8
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 8
#define TEXT_36_LENGTH 10
#define TEXT_37_LENGTH 9
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 9
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 10
#define TEXT_42_LENGTH 10
#define TEXT_43_LENGTH 11
#define TEXT_44_LENGTH 10
#define TEXT_45_LENGTH 13
#define TEXT_46_LENGTH 10
#define TEXT_47_LENGTH 16
#define TEXT_48_LENGTH 10
#define TEXT_49_LENGTH 10
#define TEXT_50_LENGTH 9
#define TEXT_51_LENGTH 10
#define TEXT_52_LENGTH 10
#define TEXT_53_LENGTH 10
#define TEXT_54_LENGTH 9
#define TEXT_55_LENGTH 11
#define TEXT_56_LENGTH 11
#define TEXT_57_LENGTH 11
#define TEXT_58_LENGTH 11
#define TEXT_59_LENGTH 14
#define TEXT_60_LENGTH 9
#define TEXT_61_LENGTH 12
#define TEXT_62_LENGTH 12
#define TEXT_63_LENGTH 12
#define TEXT_64_LENGTH 10
#define TEXT_65_LENGTH 14
#define TEXT_66_LENGTH 11
#define TEXT_67_LENGTH 10
#define TEXT_68_LENGTH 13
#define TEXT_69_LENGTH 8
#define TEXT_70_LENGTH 7
#define TEXT_71_LENGTH 7
#define TEXT_72_LENGTH 11
#define TEXT_73_LENGTH 7
#define TEXT_74_LENGTH 9
#define TEXT_75_LENGTH 10
#define TEXT_76_LENGTH 8
#define TEXT_77_LENGTH 9
#define TEXT_78_LENGTH 7
#define TEXT_79_LENGTH 12
#define TEXT_80_LENGTH 12
#define TEXT_81_LENGTH 12
#define TEXT_82_LENGTH 12
#include "stg12/hotshelter1_textures/text.inc"

static NJS_TEXNAME hotshelter1_textures[78] = {
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
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0}, {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
    {TEXT_57, 0, 0}, {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0},
    {TEXT_61, 0, 0}, {TEXT_62, 0, 0}, {TEXT_63, 0, 0}, {TEXT_64, 0, 0},
    {TEXT_65, 0, 0}, {TEXT_66, 0, 0}, {TEXT_67, 0, 0}, {TEXT_68, 0, 0},
    {TEXT_69, 0, 0}, {TEXT_70, 0, 0}, {TEXT_71, 0, 0}, {TEXT_72, 0, 0},
    {TEXT_73, 0, 0}, {TEXT_74, 0, 0}, {TEXT_75, 0, 0}, {TEXT_76, 0, 0},
    {TEXT_77, 0, 0}, {TEXT_78, 0, 0},
};
NJS_TEXLIST hotshelter1_texlist = {
#include "stg12/hotshelter1_textures/hotshelter1_texlist.inc"
};
static NJS_TEXNAME shelter_column_textures[5] = {
    {TEXT_1, 0, 0},  {TEXT_79, 0, 0}, {TEXT_80, 0, 0},
    {TEXT_81, 0, 0}, {TEXT_82, 0, 0},
};
NJS_TEXLIST shelter_column_texlist = {
#include "stg12/hotshelter1_textures/shelter_column_texlist.inc"
};
static NJS_MATERIAL dat_0ca5e9a4[1] = {
#include "stg12/hotshelter1_textures/dat_0ca5e9a4.inc"
};
