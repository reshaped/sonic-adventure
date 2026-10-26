/* Chaos 0's spin action and its motion; his textures and the stage's
 * effect, object and land textures, with their texlists; and the tuft of
 * grass O GRAS0 keeps, with its model */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos0/textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 7
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 4
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 7
#define TEXT_20_LENGTH 6
#define TEXT_21_LENGTH 6
#define TEXT_22_LENGTH 6
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 11
#define TEXT_26_LENGTH 16
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 4
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 4
#define TEXT_34_LENGTH 5
#define TEXT_35_LENGTH 7
#define TEXT_36_LENGTH 9
#define TEXT_37_LENGTH 7
#define TEXT_38_LENGTH 5
#define TEXT_39_LENGTH 6
#define TEXT_40_LENGTH 8
#define TEXT_41_LENGTH 13
#define TEXT_42_LENGTH 14
#define TEXT_43_LENGTH 8
#define TEXT_44_LENGTH 10
#define TEXT_45_LENGTH 11
#define TEXT_46_LENGTH 3
#define TEXT_47_LENGTH 5
#define TEXT_48_LENGTH 5
#define TEXT_49_LENGTH 6
#define TEXT_50_LENGTH 6
#define TEXT_51_LENGTH 6
#define TEXT_52_LENGTH 6
#define TEXT_53_LENGTH 7
#define TEXT_54_LENGTH 7
#define TEXT_55_LENGTH 10
#define TEXT_56_LENGTH 6
#define TEXT_57_LENGTH 6
#define TEXT_58_LENGTH 11
#define TEXT_59_LENGTH 6
#define TEXT_60_LENGTH 5
#define TEXT_61_LENGTH 10
#define TEXT_62_LENGTH 7
#define TEXT_63_LENGTH 12
#define TEXT_64_LENGTH 13
#define TEXT_65_LENGTH 6
#define TEXT_66_LENGTH 8
#define TEXT_67_LENGTH 6
#include "b_chaos0/textures/text.inc"

extern NJS_OBJECT chaos0_body_object;
extern NJS_MDATA2 dat_0c94cac0[];

static NJS_MOTION spin_motion = {
#include "b_chaos0/textures/spin_motion.inc"
};
NJS_ACTION chaos0_spin_action = {
#include "b_chaos0/textures/chaos0_spin_action.inc"
};
static NJS_TEXNAME chaos0_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST chaos0_texlist = {
#include "b_chaos0/textures/chaos0_texlist.inc"
};
static NJS_TEXNAME chaos0_effect_textures[2] = {
    {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},
};
NJS_TEXLIST chaos0_effect_texlist = {
#include "b_chaos0/textures/chaos0_effect_texlist.inc"
};
static NJS_TEXNAME chaos0_object_textures[33] = {
    {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},
    {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
    {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0},
    {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0}, {TEXT_21, 0, 0},
    {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0}, {TEXT_25, 0, 0},
    {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0},
    {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0}, {TEXT_33, 0, 0},
    {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0}, {TEXT_37, 0, 0},
    {TEXT_38, 0, 0},
};
NJS_TEXLIST chaos0_object_texlist = {
#include "b_chaos0/textures/chaos0_object_texlist.inc"
};
static NJS_TEXNAME chaos0_land_textures[34] = {
    {TEXT_39, 0, 0}, {TEXT_40, 0, 0}, {TEXT_41, 0, 0}, {TEXT_42, 0, 0},
    {TEXT_43, 0, 0}, {TEXT_44, 0, 0}, {TEXT_45, 0, 0}, {TEXT_17, 0, 0},
    {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0}, {TEXT_49, 0, 0},
    {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0}, {TEXT_53, 0, 0},
    {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0}, {TEXT_57, 0, 0},
    {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0}, {TEXT_61, 0, 0},
    {TEXT_62, 0, 0}, {TEXT_63, 0, 0}, {TEXT_64, 0, 0}, {TEXT_65, 0, 0},
    {TEXT_66, 0, 0}, {TEXT_15, 0, 0}, {TEXT_14, 0, 0}, {TEXT_13, 0, 0},
    {TEXT_16, 0, 0}, {TEXT_67, 0, 0},
};
NJS_TEXLIST chaos0_land_texlist = {
#include "b_chaos0/textures/chaos0_land_texlist.inc"
};
static NJS_MATERIAL dat_0c94d2d4[4] = {
#include "b_chaos0/textures/dat_0c94d2d4.inc"
};
static Sint16 dat_0c94d324[11] = {
#include "b_chaos0/textures/dat_0c94d324.inc"
};
static Sint16 dat_0c94d33a[14] = {
#include "b_chaos0/textures/dat_0c94d33a.inc"
};
static Sint16 dat_0c94d356[28] = {
#include "b_chaos0/textures/dat_0c94d356.inc"
};
static Sint16 dat_0c94d38e[319] = {
#include "b_chaos0/textures/dat_0c94d38e.inc"
};
static NJS_COLOR dat_0c94d60c[10] = {
#include "b_chaos0/textures/dat_0c94d60c.inc"
};
static NJS_COLOR dat_0c94d634[12] = {
#include "b_chaos0/textures/dat_0c94d634.inc"
};
static NJS_COLOR dat_0c94d664[24] = {
#include "b_chaos0/textures/dat_0c94d664.inc"
};
static NJS_COLOR dat_0c94d6c4[248] = {
#include "b_chaos0/textures/dat_0c94d6c4.inc"
};
static NJS_MESHSET dat_0c94daa4[4] = {
#include "b_chaos0/textures/dat_0c94daa4.inc"
};
static NJS_POINT3 dat_0c94db04[164] = {
#include "b_chaos0/textures/dat_0c94db04.inc"
};
static NJS_VECTOR dat_0c94e2b4[164] = {
#include "b_chaos0/textures/dat_0c94e2b4.inc"
};
static NJS_MODEL dat_0c94ea64 = {
#include "b_chaos0/textures/dat_0c94ea64.inc"
};
NJS_OBJECT chaos0_grass_0_object = {
#include "b_chaos0/textures/chaos0_grass_0_object.inc"
};
