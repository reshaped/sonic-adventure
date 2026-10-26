/* the textures of OBJ_CHAOS7 and EFF_WT_COLUMN with their texlists, and the
 * sky's model */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/textures.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 14
#define TEXT_21_LENGTH 11
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 11
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 11
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 13
#define TEXT_31_LENGTH 12
#define TEXT_32_LENGTH 12
#define TEXT_33_LENGTH 12
#define TEXT_34_LENGTH 12
#define TEXT_35_LENGTH 11
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 12
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 12
#define TEXT_42_LENGTH 9
#define TEXT_43_LENGTH 9
#define TEXT_44_LENGTH 9
#define TEXT_45_LENGTH 9
#define TEXT_46_LENGTH 9
#define TEXT_47_LENGTH 9
#define TEXT_48_LENGTH 9
#define TEXT_49_LENGTH 11
#define TEXT_50_LENGTH 8
#define TEXT_51_LENGTH 8
#define TEXT_52_LENGTH 8
#define TEXT_53_LENGTH 11
#define TEXT_54_LENGTH 11
#define TEXT_55_LENGTH 11
#define TEXT_56_LENGTH 11
#define TEXT_57_LENGTH 11
#define TEXT_58_LENGTH 11
#define TEXT_59_LENGTH 9
#define TEXT_60_LENGTH 9
#define TEXT_61_LENGTH 11
#define TEXT_62_LENGTH 13
#define TEXT_63_LENGTH 11
#define TEXT_64_LENGTH 11
#define TEXT_65_LENGTH 10
#define TEXT_66_LENGTH 10
#define TEXT_67_LENGTH 8
#define TEXT_68_LENGTH 9
#define TEXT_69_LENGTH 9
#define TEXT_70_LENGTH 9
#define TEXT_71_LENGTH 9
#define TEXT_72_LENGTH 9
#define TEXT_73_LENGTH 9
#define TEXT_74_LENGTH 9
#define TEXT_75_LENGTH 9
#define TEXT_76_LENGTH 9
#define TEXT_77_LENGTH 9
#define TEXT_78_LENGTH 9
#define TEXT_79_LENGTH 9
#define TEXT_80_LENGTH 9
#define TEXT_81_LENGTH 9
#define TEXT_82_LENGTH 9
#define TEXT_83_LENGTH 7
#define TEXT_84_LENGTH 7
#define TEXT_85_LENGTH 7
#define TEXT_86_LENGTH 12
#define TEXT_87_LENGTH 13
#define TEXT_88_LENGTH 14
#define TEXT_89_LENGTH 12
#define TEXT_90_LENGTH 12
#define TEXT_91_LENGTH 12
#define TEXT_92_LENGTH 12
#define TEXT_93_LENGTH 12
#define TEXT_94_LENGTH 11
#define TEXT_95_LENGTH 11
#define TEXT_96_LENGTH 10
#define TEXT_97_LENGTH 10
#define TEXT_98_LENGTH 6
#define TEXT_99_LENGTH 12
#define TEXT_100_LENGTH 12
#define TEXT_101_LENGTH 12
#define TEXT_102_LENGTH 12
#define TEXT_103_LENGTH 12
#define TEXT_104_LENGTH 12
#define TEXT_105_LENGTH 12
#define TEXT_106_LENGTH 12
#define TEXT_107_LENGTH 12
#define TEXT_108_LENGTH 12
#define TEXT_109_LENGTH 12
#define TEXT_110_LENGTH 12
#define TEXT_111_LENGTH 12
#define TEXT_112_LENGTH 12
#define TEXT_113_LENGTH 11
#define TEXT_114_LENGTH 11
#define TEXT_115_LENGTH 11
#define TEXT_116_LENGTH 11
#define TEXT_117_LENGTH 11
#include "b_chaos7/textures/text.inc"

static NJS_TEXNAME obj_chaos7_textures[112] = {
    {TEXT_1, 0, 0},   {TEXT_2, 0, 0},   {TEXT_3, 0, 0},   {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},   {TEXT_6, 0, 0},   {TEXT_7, 0, 0},   {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},   {TEXT_10, 0, 0},  {TEXT_11, 0, 0},  {TEXT_12, 0, 0},
    {TEXT_13, 0, 0},  {TEXT_14, 0, 0},  {TEXT_15, 0, 0},  {TEXT_16, 0, 0},
    {TEXT_17, 0, 0},  {TEXT_18, 0, 0},  {TEXT_19, 0, 0},  {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},  {TEXT_22, 0, 0},  {TEXT_23, 0, 0},  {TEXT_24, 0, 0},
    {TEXT_25, 0, 0},  {TEXT_26, 0, 0},  {TEXT_27, 0, 0},  {TEXT_28, 0, 0},
    {TEXT_29, 0, 0},  {TEXT_30, 0, 0},  {TEXT_31, 0, 0},  {TEXT_32, 0, 0},
    {TEXT_33, 0, 0},  {TEXT_34, 0, 0},  {TEXT_35, 0, 0},  {TEXT_36, 0, 0},
    {TEXT_37, 0, 0},  {TEXT_38, 0, 0},  {TEXT_39, 0, 0},  {TEXT_40, 0, 0},
    {TEXT_41, 0, 0},  {TEXT_42, 0, 0},  {TEXT_43, 0, 0},  {TEXT_44, 0, 0},
    {TEXT_45, 0, 0},  {TEXT_46, 0, 0},  {TEXT_47, 0, 0},  {TEXT_48, 0, 0},
    {TEXT_49, 0, 0},  {TEXT_50, 0, 0},  {TEXT_51, 0, 0},  {TEXT_52, 0, 0},
    {TEXT_53, 0, 0},  {TEXT_54, 0, 0},  {TEXT_55, 0, 0},  {TEXT_56, 0, 0},
    {TEXT_57, 0, 0},  {TEXT_58, 0, 0},  {TEXT_59, 0, 0},  {TEXT_60, 0, 0},
    {TEXT_61, 0, 0},  {TEXT_62, 0, 0},  {TEXT_63, 0, 0},  {TEXT_64, 0, 0},
    {TEXT_65, 0, 0},  {TEXT_66, 0, 0},  {TEXT_67, 0, 0},  {TEXT_68, 0, 0},
    {TEXT_69, 0, 0},  {TEXT_70, 0, 0},  {TEXT_71, 0, 0},  {TEXT_72, 0, 0},
    {TEXT_73, 0, 0},  {TEXT_74, 0, 0},  {TEXT_75, 0, 0},  {TEXT_76, 0, 0},
    {TEXT_77, 0, 0},  {TEXT_78, 0, 0},  {TEXT_79, 0, 0},  {TEXT_80, 0, 0},
    {TEXT_81, 0, 0},  {TEXT_82, 0, 0},  {TEXT_83, 0, 0},  {TEXT_84, 0, 0},
    {TEXT_85, 0, 0},  {TEXT_86, 0, 0},  {TEXT_87, 0, 0},  {TEXT_88, 0, 0},
    {TEXT_89, 0, 0},  {TEXT_90, 0, 0},  {TEXT_91, 0, 0},  {TEXT_92, 0, 0},
    {TEXT_93, 0, 0},  {TEXT_94, 0, 0},  {TEXT_95, 0, 0},  {TEXT_96, 0, 0},
    {TEXT_97, 0, 0},  {TEXT_98, 0, 0},  {TEXT_99, 0, 0},  {TEXT_100, 0, 0},
    {TEXT_101, 0, 0}, {TEXT_102, 0, 0}, {TEXT_103, 0, 0}, {TEXT_104, 0, 0},
    {TEXT_105, 0, 0}, {TEXT_106, 0, 0}, {TEXT_107, 0, 0}, {TEXT_108, 0, 0},
    {TEXT_109, 0, 0}, {TEXT_110, 0, 0}, {TEXT_111, 0, 0}, {TEXT_112, 0, 0},
};
NJS_TEXLIST obj_chaos7_texlist = {
#include "b_chaos7/textures/obj_chaos7_texlist.inc"
};
NJS_TEXNAME column_textures[5] = {
    {TEXT_113, 0, 0}, {TEXT_114, 0, 0}, {TEXT_115, 0, 0},
    {TEXT_116, 0, 0}, {TEXT_117, 0, 0},
};
NJS_TEXLIST eff_wt_column_texlist = {
#include "b_chaos7/textures/eff_wt_column_texlist.inc"
};
static NJS_MATERIAL dat_0c96615c[3] = {
#include "b_chaos7/textures/dat_0c96615c.inc"
};
static Sint16 dat_0c966198[69] = {
#include "b_chaos7/textures/dat_0c966198.inc"
};
static Sint16 dat_0c966222[84] = {
#include "b_chaos7/textures/dat_0c966222.inc"
};
static Sint16 dat_0c9662ca[37] = {
#include "b_chaos7/textures/dat_0c9662ca.inc"
};
static NJS_COLOR dat_0c966314[62] = {
#include "b_chaos7/textures/dat_0c966314.inc"
};
static NJS_COLOR dat_0c96640c[80] = {
#include "b_chaos7/textures/dat_0c96640c.inc"
};
static NJS_COLOR dat_0c96654c[32] = {
#include "b_chaos7/textures/dat_0c96654c.inc"
};
static NJS_MESHSET dat_0c9665cc[3] = {
#include "b_chaos7/textures/dat_0c9665cc.inc"
};
static NJS_POINT3 dat_0c966614[81] = {
#include "b_chaos7/textures/dat_0c966614.inc"
};
static NJS_VECTOR dat_0c9669e0[81] = {
#include "b_chaos7/textures/dat_0c9669e0.inc"
};
static NJS_MODEL dat_0c966dac = {
#include "b_chaos7/textures/dat_0c966dac.inc"
};
NJS_OBJECT chaos7_sky_object = {
#include "b_chaos7/textures/chaos7_sky_object.inc"
};
