/* 31 of the Tornado's texture names, and a texlist of them that nothing
 * reads. then a model tree whose root, dat_0cb87d78, nothing points at.
 * then ev003b_tornado_texlist and its texture names. the last datum is
 * dat_0cb87fdc, which the node just past it, at 0x0cb88010, names as its
 * sibling */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev003b/tornado_textures.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 12
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 12
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 10
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 9
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 9
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 12
#define TEXT_33_LENGTH 13
#define TEXT_34_LENGTH 10
#define TEXT_35_LENGTH 13
#define TEXT_36_LENGTH 13
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 11
#define TEXT_39_LENGTH 8
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 8
#define TEXT_42_LENGTH 12
#define TEXT_43_LENGTH 12
#define TEXT_44_LENGTH 12
#define TEXT_45_LENGTH 13
#define TEXT_46_LENGTH 13
#include "ev003b/tornado_textures/text.inc"

static NJS_TEXNAME dat_0cb831c8[31] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0},
};
static NJS_TEXLIST dat_0cb8333c = {
#include "ev003b/tornado_textures/dat_0cb8333c.inc"
};
static NJS_OBJECT dat_0cb83344 = {
#include "ev003b/tornado_textures/dat_0cb83344.inc"
};
static NJS_OBJECT dat_0cb83378 = {
#include "ev003b/tornado_textures/dat_0cb83378.inc"
};
static NJS_MATERIAL dat_0cb833ac[1] = {
#include "ev003b/tornado_textures/dat_0cb833ac.inc"
};
static Sint16 dat_0cb833c0[140] = {
#include "ev003b/tornado_textures/dat_0cb833c0.inc"
};
static NJS_COLOR dat_0cb834d8[122] = {
#include "ev003b/tornado_textures/dat_0cb834d8.inc"
};
static NJS_MESHSET dat_0cb836c0[1] = {
#include "ev003b/tornado_textures/dat_0cb836c0.inc"
};
static NJS_POINT3 dat_0cb836d8[48] = {
#include "ev003b/tornado_textures/dat_0cb836d8.inc"
};
static NJS_VECTOR dat_0cb83918[48] = {
#include "ev003b/tornado_textures/dat_0cb83918.inc"
};
static NJS_MODEL dat_0cb83b58 = {
#include "ev003b/tornado_textures/dat_0cb83b58.inc"
};
static NJS_OBJECT dat_0cb83b80 = {
#include "ev003b/tornado_textures/dat_0cb83b80.inc"
};
static NJS_MATERIAL dat_0cb83bb4[2] = {
#include "ev003b/tornado_textures/dat_0cb83bb4.inc"
};
static Sint16 dat_0cb83bdc[39] = {
#include "ev003b/tornado_textures/dat_0cb83bdc.inc"
};
static Sint16 dat_0cb83c2a[13] = {
#include "ev003b/tornado_textures/dat_0cb83c2a.inc"
};
static NJS_COLOR dat_0cb83c44[34] = {
#include "ev003b/tornado_textures/dat_0cb83c44.inc"
};
static NJS_COLOR dat_0cb83ccc[9] = {
#include "ev003b/tornado_textures/dat_0cb83ccc.inc"
};
static NJS_MESHSET dat_0cb83cf0[2] = {
#include "ev003b/tornado_textures/dat_0cb83cf0.inc"
};
static NJS_POINT3 dat_0cb83d20[24] = {
#include "ev003b/tornado_textures/dat_0cb83d20.inc"
};
static NJS_VECTOR dat_0cb83e40[24] = {
#include "ev003b/tornado_textures/dat_0cb83e40.inc"
};
static NJS_MODEL dat_0cb83f60 = {
#include "ev003b/tornado_textures/dat_0cb83f60.inc"
};
static NJS_OBJECT dat_0cb83f88 = {
#include "ev003b/tornado_textures/dat_0cb83f88.inc"
};
static NJS_MATERIAL dat_0cb83fbc[31] = {
#include "ev003b/tornado_textures/dat_0cb83fbc.inc"
};
static Sint16 dat_0cb84228[9] = {
#include "ev003b/tornado_textures/dat_0cb84228.inc"
};
static Sint16 dat_0cb8423a[163] = {
#include "ev003b/tornado_textures/dat_0cb8423a.inc"
};
static Sint16 dat_0cb84380[18] = {
#include "ev003b/tornado_textures/dat_0cb84380.inc"
};
static Sint16 dat_0cb843a4[21] = {
#include "ev003b/tornado_textures/dat_0cb843a4.inc"
};
static Sint16 dat_0cb843ce[6] = {
#include "ev003b/tornado_textures/dat_0cb843ce.inc"
};
static Sint16 dat_0cb843da[51] = {
#include "ev003b/tornado_textures/dat_0cb843da.inc"
};
static Sint16 dat_0cb84440[21] = {
#include "ev003b/tornado_textures/dat_0cb84440.inc"
};
static Sint16 dat_0cb8446a[16] = {
#include "ev003b/tornado_textures/dat_0cb8446a.inc"
};
static Sint16 dat_0cb8448a[12] = {
#include "ev003b/tornado_textures/dat_0cb8448a.inc"
};
static Sint16 dat_0cb844a2[29] = {
#include "ev003b/tornado_textures/dat_0cb844a2.inc"
};
static Sint16 dat_0cb844dc[19] = {
#include "ev003b/tornado_textures/dat_0cb844dc.inc"
};
static Sint16 dat_0cb84502[38] = {
#include "ev003b/tornado_textures/dat_0cb84502.inc"
};
static Sint16 dat_0cb8454e[11] = {
#include "ev003b/tornado_textures/dat_0cb8454e.inc"
};
static Sint16 dat_0cb84564[38] = {
#include "ev003b/tornado_textures/dat_0cb84564.inc"
};
static Sint16 dat_0cb845b0[49] = {
#include "ev003b/tornado_textures/dat_0cb845b0.inc"
};
static Sint16 dat_0cb84612[49] = {
#include "ev003b/tornado_textures/dat_0cb84612.inc"
};
static Sint16 dat_0cb84674[11] = {
#include "ev003b/tornado_textures/dat_0cb84674.inc"
};
static Sint16 dat_0cb8468a[20] = {
#include "ev003b/tornado_textures/dat_0cb8468a.inc"
};
static Sint16 dat_0cb846b2[5] = {
#include "ev003b/tornado_textures/dat_0cb846b2.inc"
};
static Sint16 dat_0cb846bc[48] = {
#include "ev003b/tornado_textures/dat_0cb846bc.inc"
};
static Sint16 dat_0cb8471c[48] = {
#include "ev003b/tornado_textures/dat_0cb8471c.inc"
};
static Sint16 dat_0cb8477c[10] = {
#include "ev003b/tornado_textures/dat_0cb8477c.inc"
};
static Sint16 dat_0cb84790[24] = {
#include "ev003b/tornado_textures/dat_0cb84790.inc"
};
static Sint16 dat_0cb847c0[51] = {
#include "ev003b/tornado_textures/dat_0cb847c0.inc"
};
static Sint16 dat_0cb84826[51] = {
#include "ev003b/tornado_textures/dat_0cb84826.inc"
};
static Sint16 dat_0cb8488c[36] = {
#include "ev003b/tornado_textures/dat_0cb8488c.inc"
};
static Sint16 dat_0cb848d4[28] = {
#include "ev003b/tornado_textures/dat_0cb848d4.inc"
};
static Sint16 dat_0cb8490c[6] = {
#include "ev003b/tornado_textures/dat_0cb8490c.inc"
};
static Sint16 dat_0cb84918[6] = {
#include "ev003b/tornado_textures/dat_0cb84918.inc"
};
static Sint16 dat_0cb84924[14] = {
#include "ev003b/tornado_textures/dat_0cb84924.inc"
};
static Sint16 dat_0cb84940[118] = {
#include "ev003b/tornado_textures/dat_0cb84940.inc"
};
static NJS_COLOR dat_0cb84a2c[8] = {
#include "ev003b/tornado_textures/dat_0cb84a2c.inc"
};
static NJS_COLOR dat_0cb84a4c[143] = {
#include "ev003b/tornado_textures/dat_0cb84a4c.inc"
};
static NJS_COLOR dat_0cb84c88[15] = {
#include "ev003b/tornado_textures/dat_0cb84c88.inc"
};
static NJS_COLOR dat_0cb84cc4[17] = {
#include "ev003b/tornado_textures/dat_0cb84cc4.inc"
};
static NJS_COLOR dat_0cb84d08[5] = {
#include "ev003b/tornado_textures/dat_0cb84d08.inc"
};
static NJS_COLOR dat_0cb84d1c[42] = {
#include "ev003b/tornado_textures/dat_0cb84d1c.inc"
};
static NJS_COLOR dat_0cb84dc4[18] = {
#include "ev003b/tornado_textures/dat_0cb84dc4.inc"
};
static NJS_COLOR dat_0cb84e0c[14] = {
#include "ev003b/tornado_textures/dat_0cb84e0c.inc"
};
static NJS_COLOR dat_0cb84e44[10] = {
#include "ev003b/tornado_textures/dat_0cb84e44.inc"
};
static NJS_COLOR dat_0cb84e6c[24] = {
#include "ev003b/tornado_textures/dat_0cb84e6c.inc"
};
static NJS_COLOR dat_0cb84ecc[15] = {
#include "ev003b/tornado_textures/dat_0cb84ecc.inc"
};
static NJS_COLOR dat_0cb84f08[32] = {
#include "ev003b/tornado_textures/dat_0cb84f08.inc"
};
static NJS_COLOR dat_0cb84f88[10] = {
#include "ev003b/tornado_textures/dat_0cb84f88.inc"
};
static NJS_COLOR dat_0cb84fb0[32] = {
#include "ev003b/tornado_textures/dat_0cb84fb0.inc"
};
static NJS_COLOR dat_0cb85030[42] = {
#include "ev003b/tornado_textures/dat_0cb85030.inc"
};
static NJS_COLOR dat_0cb850d8[42] = {
#include "ev003b/tornado_textures/dat_0cb850d8.inc"
};
static NJS_COLOR dat_0cb85180[9] = {
#include "ev003b/tornado_textures/dat_0cb85180.inc"
};
static NJS_COLOR dat_0cb851a4[16] = {
#include "ev003b/tornado_textures/dat_0cb851a4.inc"
};
static NJS_COLOR dat_0cb851e4[4] = {
#include "ev003b/tornado_textures/dat_0cb851e4.inc"
};
static NJS_COLOR dat_0cb851f4[40] = {
#include "ev003b/tornado_textures/dat_0cb851f4.inc"
};
static NJS_COLOR dat_0cb85294[40] = {
#include "ev003b/tornado_textures/dat_0cb85294.inc"
};
static NJS_COLOR dat_0cb85334[8] = {
#include "ev003b/tornado_textures/dat_0cb85334.inc"
};
static NJS_COLOR dat_0cb85354[20] = {
#include "ev003b/tornado_textures/dat_0cb85354.inc"
};
static NJS_COLOR dat_0cb853a4[42] = {
#include "ev003b/tornado_textures/dat_0cb853a4.inc"
};
static NJS_COLOR dat_0cb8544c[42] = {
#include "ev003b/tornado_textures/dat_0cb8544c.inc"
};
static NJS_COLOR dat_0cb854f4[32] = {
#include "ev003b/tornado_textures/dat_0cb854f4.inc"
};
static NJS_COLOR dat_0cb85574[24] = {
#include "ev003b/tornado_textures/dat_0cb85574.inc"
};
static NJS_COLOR dat_0cb855d4[5] = {
#include "ev003b/tornado_textures/dat_0cb855d4.inc"
};
static NJS_COLOR dat_0cb855e8[5] = {
#include "ev003b/tornado_textures/dat_0cb855e8.inc"
};
static NJS_COLOR dat_0cb855fc[12] = {
#include "ev003b/tornado_textures/dat_0cb855fc.inc"
};
static NJS_COLOR dat_0cb8562c[104] = {
#include "ev003b/tornado_textures/dat_0cb8562c.inc"
};
static NJS_MESHSET dat_0cb857cc[31] = {
#include "ev003b/tornado_textures/dat_0cb857cc.inc"
};
static NJS_POINT3 dat_0cb85ab4[367] = {
#include "ev003b/tornado_textures/dat_0cb85ab4.inc"
};
static NJS_VECTOR dat_0cb86be8[367] = {
#include "ev003b/tornado_textures/dat_0cb86be8.inc"
};
static NJS_MODEL dat_0cb87d1c = {
#include "ev003b/tornado_textures/dat_0cb87d1c.inc"
};
static NJS_OBJECT dat_0cb87d44 = {
#include "ev003b/tornado_textures/dat_0cb87d44.inc"
};
static NJS_OBJECT dat_0cb87d78 = {
#include "ev003b/tornado_textures/dat_0cb87d78.inc"
};
static NJS_TEXNAME ev003b_tornado_textures[46] = {
    {TEXT_32, 0, 0}, {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0},
    {TEXT_36, 0, 0}, {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0},
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_24, 0, 0}, {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0},
    {TEXT_28, 0, 0}, {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0},
};
NJS_TEXLIST ev003b_tornado_texlist = {
#include "ev003b/tornado_textures/ev003b_tornado_texlist.inc"
};
static NJS_OBJECT dat_0cb87fdc = {
#include "ev003b/tornado_textures/dat_0cb87fdc.inc"
};
