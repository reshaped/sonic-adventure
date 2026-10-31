/* E-102's ev0142_e102_action_2, then E-101's texture list, model and action.
 * the data ends on the texture names of ev0142_memory_texlist, which holds
 * the two pictures script.c shows over the screen */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0142/e102_action_2.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 13
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 14
#define TEXT_23_LENGTH 13
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 12
#define TEXT_26_LENGTH 7
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 7
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 8
#define TEXT_32_LENGTH 8
#define TEXT_33_LENGTH 7
#define TEXT_34_LENGTH 7
#include "ev0142/e102_action_2/text.inc"

extern NJS_MDATA2 dat_0cb8a208[];
extern NJS_OBJECT e102_object;

static NJS_MOTION ev0142_e102_motion_2 = {
#include "ev0142/e102_action_2/ev0142_e102_motion_2.inc"
};
NJS_ACTION ev0142_e102_action_2 = {
#include "ev0142/e102_action_2/ev0142_e102_action_2.inc"
};
static NJS_TEXNAME ev0142_e101_textures[32] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
};
NJS_TEXLIST ev0142_e101_texlist = {
#include "ev0142/e102_action_2/ev0142_e101_texlist.inc"
};
static NJS_OBJECT dat_0cb8a874 = {
#include "ev0142/e102_action_2/dat_0cb8a874.inc"
};
static NJS_OBJECT dat_0cb8a8a8 = {
#include "ev0142/e102_action_2/dat_0cb8a8a8.inc"
};
static NJS_OBJECT dat_0cb8a8dc = {
#include "ev0142/e102_action_2/dat_0cb8a8dc.inc"
};
static NJS_OBJECT dat_0cb8a910 = {
#include "ev0142/e102_action_2/dat_0cb8a910.inc"
};
static NJS_OBJECT dat_0cb8a944 = {
#include "ev0142/e102_action_2/dat_0cb8a944.inc"
};
static NJS_OBJECT dat_0cb8a978 = {
#include "ev0142/e102_action_2/dat_0cb8a978.inc"
};
static NJS_OBJECT dat_0cb8a9ac = {
#include "ev0142/e102_action_2/dat_0cb8a9ac.inc"
};
static NJS_OBJECT dat_0cb8a9e0 = {
#include "ev0142/e102_action_2/dat_0cb8a9e0.inc"
};
static NJS_MATERIAL dat_0cb8aa14[1] = {
#include "ev0142/e102_action_2/dat_0cb8aa14.inc"
};
static Sint16 dat_0cb8aa28[12] = {
#include "ev0142/e102_action_2/dat_0cb8aa28.inc"
};
static NJS_COLOR dat_0cb8aa40[10] = {
#include "ev0142/e102_action_2/dat_0cb8aa40.inc"
};
static NJS_MESHSET dat_0cb8aa68[1] = {
#include "ev0142/e102_action_2/dat_0cb8aa68.inc"
};
static NJS_POINT3 dat_0cb8aa80[8] = {
#include "ev0142/e102_action_2/dat_0cb8aa80.inc"
};
static NJS_VECTOR dat_0cb8aae0[8] = {
#include "ev0142/e102_action_2/dat_0cb8aae0.inc"
};
static NJS_MODEL dat_0cb8ab40 = {
#include "ev0142/e102_action_2/dat_0cb8ab40.inc"
};
static NJS_OBJECT dat_0cb8ab68 = {
#include "ev0142/e102_action_2/dat_0cb8ab68.inc"
};
static NJS_MATERIAL dat_0cb8ab9c[1] = {
#include "ev0142/e102_action_2/dat_0cb8ab9c.inc"
};
static Sint16 dat_0cb8abb0[28] = {
#include "ev0142/e102_action_2/dat_0cb8abb0.inc"
};
static NJS_COLOR dat_0cb8abe8[24] = {
#include "ev0142/e102_action_2/dat_0cb8abe8.inc"
};
static NJS_MESHSET dat_0cb8ac48[1] = {
#include "ev0142/e102_action_2/dat_0cb8ac48.inc"
};
static NJS_POINT3 dat_0cb8ac60[14] = {
#include "ev0142/e102_action_2/dat_0cb8ac60.inc"
};
static NJS_VECTOR dat_0cb8ad08[14] = {
#include "ev0142/e102_action_2/dat_0cb8ad08.inc"
};
static NJS_MODEL dat_0cb8adb0 = {
#include "ev0142/e102_action_2/dat_0cb8adb0.inc"
};
static NJS_OBJECT dat_0cb8add8 = {
#include "ev0142/e102_action_2/dat_0cb8add8.inc"
};
static NJS_MATERIAL dat_0cb8ae0c[5] = {
#include "ev0142/e102_action_2/dat_0cb8ae0c.inc"
};
static Sint16 dat_0cb8ae70[79] = {
#include "ev0142/e102_action_2/dat_0cb8ae70.inc"
};
static Sint16 dat_0cb8af0e[45] = {
#include "ev0142/e102_action_2/dat_0cb8af0e.inc"
};
static Sint16 dat_0cb8af68[70] = {
#include "ev0142/e102_action_2/dat_0cb8af68.inc"
};
static Sint16 dat_0cb8aff4[14] = {
#include "ev0142/e102_action_2/dat_0cb8aff4.inc"
};
static Sint16 dat_0cb8b010[20] = {
#include "ev0142/e102_action_2/dat_0cb8b010.inc"
};
static NJS_COLOR dat_0cb8b038[68] = {
#include "ev0142/e102_action_2/dat_0cb8b038.inc"
};
static NJS_COLOR dat_0cb8b148[38] = {
#include "ev0142/e102_action_2/dat_0cb8b148.inc"
};
static NJS_COLOR dat_0cb8b1e0[60] = {
#include "ev0142/e102_action_2/dat_0cb8b1e0.inc"
};
static NJS_COLOR dat_0cb8b2d0[12] = {
#include "ev0142/e102_action_2/dat_0cb8b2d0.inc"
};
static NJS_COLOR dat_0cb8b300[16] = {
#include "ev0142/e102_action_2/dat_0cb8b300.inc"
};
static NJS_MESHSET dat_0cb8b340[5] = {
#include "ev0142/e102_action_2/dat_0cb8b340.inc"
};
static NJS_POINT3 dat_0cb8b3b8[66] = {
#include "ev0142/e102_action_2/dat_0cb8b3b8.inc"
};
static NJS_VECTOR dat_0cb8b6d0[66] = {
#include "ev0142/e102_action_2/dat_0cb8b6d0.inc"
};
static NJS_MODEL dat_0cb8b9e8 = {
#include "ev0142/e102_action_2/dat_0cb8b9e8.inc"
};
static NJS_OBJECT dat_0cb8ba10 = {
#include "ev0142/e102_action_2/dat_0cb8ba10.inc"
};
static NJS_MATERIAL dat_0cb8ba44[2] = {
#include "ev0142/e102_action_2/dat_0cb8ba44.inc"
};
static Sint16 dat_0cb8ba6c[38] = {
#include "ev0142/e102_action_2/dat_0cb8ba6c.inc"
};
static Sint16 dat_0cb8bab8[16] = {
#include "ev0142/e102_action_2/dat_0cb8bab8.inc"
};
static NJS_COLOR dat_0cb8bad8[36] = {
#include "ev0142/e102_action_2/dat_0cb8bad8.inc"
};
static NJS_COLOR dat_0cb8bb68[12] = {
#include "ev0142/e102_action_2/dat_0cb8bb68.inc"
};
static NJS_MESHSET dat_0cb8bb98[2] = {
#include "ev0142/e102_action_2/dat_0cb8bb98.inc"
};
static NJS_POINT3 dat_0cb8bbc8[24] = {
#include "ev0142/e102_action_2/dat_0cb8bbc8.inc"
};
static NJS_VECTOR dat_0cb8bce8[24] = {
#include "ev0142/e102_action_2/dat_0cb8bce8.inc"
};
static NJS_MODEL dat_0cb8be08 = {
#include "ev0142/e102_action_2/dat_0cb8be08.inc"
};
static NJS_OBJECT dat_0cb8be30 = {
#include "ev0142/e102_action_2/dat_0cb8be30.inc"
};
static NJS_MATERIAL dat_0cb8be64[2] = {
#include "ev0142/e102_action_2/dat_0cb8be64.inc"
};
static Sint16 dat_0cb8be8c[20] = {
#include "ev0142/e102_action_2/dat_0cb8be8c.inc"
};
static Sint16 dat_0cb8beb4[6] = {
#include "ev0142/e102_action_2/dat_0cb8beb4.inc"
};
static NJS_COLOR dat_0cb8bec0[16] = {
#include "ev0142/e102_action_2/dat_0cb8bec0.inc"
};
static NJS_COLOR dat_0cb8bf00[4] = {
#include "ev0142/e102_action_2/dat_0cb8bf00.inc"
};
static NJS_MESHSET dat_0cb8bf10[2] = {
#include "ev0142/e102_action_2/dat_0cb8bf10.inc"
};
static NJS_POINT3 dat_0cb8bf40[8] = {
#include "ev0142/e102_action_2/dat_0cb8bf40.inc"
};
static NJS_VECTOR dat_0cb8bfa0[8] = {
#include "ev0142/e102_action_2/dat_0cb8bfa0.inc"
};
static NJS_MODEL dat_0cb8c000 = {
#include "ev0142/e102_action_2/dat_0cb8c000.inc"
};
static NJS_OBJECT dat_0cb8c028 = {
#include "ev0142/e102_action_2/dat_0cb8c028.inc"
};
static NJS_MATERIAL dat_0cb8c05c[3] = {
#include "ev0142/e102_action_2/dat_0cb8c05c.inc"
};
static Sint16 dat_0cb8c098[10] = {
#include "ev0142/e102_action_2/dat_0cb8c098.inc"
};
static Sint16 dat_0cb8c0ac[11] = {
#include "ev0142/e102_action_2/dat_0cb8c0ac.inc"
};
static Sint16 dat_0cb8c0c2[5] = {
#include "ev0142/e102_action_2/dat_0cb8c0c2.inc"
};
static NJS_COLOR dat_0cb8c0cc[8] = {
#include "ev0142/e102_action_2/dat_0cb8c0cc.inc"
};
static NJS_COLOR dat_0cb8c0ec[10] = {
#include "ev0142/e102_action_2/dat_0cb8c0ec.inc"
};
static NJS_COLOR dat_0cb8c114[4] = {
#include "ev0142/e102_action_2/dat_0cb8c114.inc"
};
static NJS_MESHSET dat_0cb8c124[3] = {
#include "ev0142/e102_action_2/dat_0cb8c124.inc"
};
static NJS_POINT3 dat_0cb8c16c[9] = {
#include "ev0142/e102_action_2/dat_0cb8c16c.inc"
};
static NJS_VECTOR dat_0cb8c1d8[9] = {
#include "ev0142/e102_action_2/dat_0cb8c1d8.inc"
};
static NJS_MODEL dat_0cb8c244 = {
#include "ev0142/e102_action_2/dat_0cb8c244.inc"
};
static NJS_OBJECT dat_0cb8c26c = {
#include "ev0142/e102_action_2/dat_0cb8c26c.inc"
};
static NJS_OBJECT dat_0cb8c2a0 = {
#include "ev0142/e102_action_2/dat_0cb8c2a0.inc"
};
static NJS_OBJECT dat_0cb8c2d4 = {
#include "ev0142/e102_action_2/dat_0cb8c2d4.inc"
};
static NJS_OBJECT dat_0cb8c308 = {
#include "ev0142/e102_action_2/dat_0cb8c308.inc"
};
static NJS_OBJECT dat_0cb8c33c = {
#include "ev0142/e102_action_2/dat_0cb8c33c.inc"
};
static NJS_OBJECT dat_0cb8c370 = {
#include "ev0142/e102_action_2/dat_0cb8c370.inc"
};
static NJS_MATERIAL dat_0cb8c3a4[1] = {
#include "ev0142/e102_action_2/dat_0cb8c3a4.inc"
};
static Sint16 dat_0cb8c3b8[12] = {
#include "ev0142/e102_action_2/dat_0cb8c3b8.inc"
};
static NJS_COLOR dat_0cb8c3d0[10] = {
#include "ev0142/e102_action_2/dat_0cb8c3d0.inc"
};
static NJS_MESHSET dat_0cb8c3f8[1] = {
#include "ev0142/e102_action_2/dat_0cb8c3f8.inc"
};
static NJS_POINT3 dat_0cb8c410[8] = {
#include "ev0142/e102_action_2/dat_0cb8c410.inc"
};
static NJS_VECTOR dat_0cb8c470[8] = {
#include "ev0142/e102_action_2/dat_0cb8c470.inc"
};
static NJS_MODEL dat_0cb8c4d0 = {
#include "ev0142/e102_action_2/dat_0cb8c4d0.inc"
};
static NJS_OBJECT dat_0cb8c4f8 = {
#include "ev0142/e102_action_2/dat_0cb8c4f8.inc"
};
static NJS_MATERIAL dat_0cb8c52c[1] = {
#include "ev0142/e102_action_2/dat_0cb8c52c.inc"
};
static Sint16 dat_0cb8c540[28] = {
#include "ev0142/e102_action_2/dat_0cb8c540.inc"
};
static NJS_COLOR dat_0cb8c578[24] = {
#include "ev0142/e102_action_2/dat_0cb8c578.inc"
};
static NJS_MESHSET dat_0cb8c5d8[1] = {
#include "ev0142/e102_action_2/dat_0cb8c5d8.inc"
};
static NJS_POINT3 dat_0cb8c5f0[14] = {
#include "ev0142/e102_action_2/dat_0cb8c5f0.inc"
};
static NJS_VECTOR dat_0cb8c698[14] = {
#include "ev0142/e102_action_2/dat_0cb8c698.inc"
};
static NJS_MODEL dat_0cb8c740 = {
#include "ev0142/e102_action_2/dat_0cb8c740.inc"
};
static NJS_OBJECT dat_0cb8c768 = {
#include "ev0142/e102_action_2/dat_0cb8c768.inc"
};
static NJS_MATERIAL dat_0cb8c79c[5] = {
#include "ev0142/e102_action_2/dat_0cb8c79c.inc"
};
static Sint16 dat_0cb8c800[76] = {
#include "ev0142/e102_action_2/dat_0cb8c800.inc"
};
static Sint16 dat_0cb8c898[48] = {
#include "ev0142/e102_action_2/dat_0cb8c898.inc"
};
static Sint16 dat_0cb8c8f8[70] = {
#include "ev0142/e102_action_2/dat_0cb8c8f8.inc"
};
static Sint16 dat_0cb8c984[14] = {
#include "ev0142/e102_action_2/dat_0cb8c984.inc"
};
static Sint16 dat_0cb8c9a0[22] = {
#include "ev0142/e102_action_2/dat_0cb8c9a0.inc"
};
static NJS_COLOR dat_0cb8c9cc[66] = {
#include "ev0142/e102_action_2/dat_0cb8c9cc.inc"
};
static NJS_COLOR dat_0cb8cad4[40] = {
#include "ev0142/e102_action_2/dat_0cb8cad4.inc"
};
static NJS_COLOR dat_0cb8cb74[60] = {
#include "ev0142/e102_action_2/dat_0cb8cb74.inc"
};
static NJS_COLOR dat_0cb8cc64[12] = {
#include "ev0142/e102_action_2/dat_0cb8cc64.inc"
};
static NJS_COLOR dat_0cb8cc94[18] = {
#include "ev0142/e102_action_2/dat_0cb8cc94.inc"
};
static NJS_MESHSET dat_0cb8ccdc[5] = {
#include "ev0142/e102_action_2/dat_0cb8ccdc.inc"
};
static NJS_POINT3 dat_0cb8cd54[66] = {
#include "ev0142/e102_action_2/dat_0cb8cd54.inc"
};
static NJS_VECTOR dat_0cb8d06c[66] = {
#include "ev0142/e102_action_2/dat_0cb8d06c.inc"
};
static NJS_MODEL dat_0cb8d384 = {
#include "ev0142/e102_action_2/dat_0cb8d384.inc"
};
static NJS_OBJECT dat_0cb8d3ac = {
#include "ev0142/e102_action_2/dat_0cb8d3ac.inc"
};
static NJS_MATERIAL dat_0cb8d3e0[2] = {
#include "ev0142/e102_action_2/dat_0cb8d3e0.inc"
};
static Sint16 dat_0cb8d408[20] = {
#include "ev0142/e102_action_2/dat_0cb8d408.inc"
};
static Sint16 dat_0cb8d430[6] = {
#include "ev0142/e102_action_2/dat_0cb8d430.inc"
};
static NJS_COLOR dat_0cb8d43c[16] = {
#include "ev0142/e102_action_2/dat_0cb8d43c.inc"
};
static NJS_COLOR dat_0cb8d47c[4] = {
#include "ev0142/e102_action_2/dat_0cb8d47c.inc"
};
static NJS_MESHSET dat_0cb8d48c[2] = {
#include "ev0142/e102_action_2/dat_0cb8d48c.inc"
};
static NJS_POINT3 dat_0cb8d4bc[8] = {
#include "ev0142/e102_action_2/dat_0cb8d4bc.inc"
};
static NJS_VECTOR dat_0cb8d51c[8] = {
#include "ev0142/e102_action_2/dat_0cb8d51c.inc"
};
static NJS_MODEL dat_0cb8d57c = {
#include "ev0142/e102_action_2/dat_0cb8d57c.inc"
};
static NJS_OBJECT dat_0cb8d5a4 = {
#include "ev0142/e102_action_2/dat_0cb8d5a4.inc"
};
static NJS_MATERIAL dat_0cb8d5d8[2] = {
#include "ev0142/e102_action_2/dat_0cb8d5d8.inc"
};
static Sint16 dat_0cb8d600[38] = {
#include "ev0142/e102_action_2/dat_0cb8d600.inc"
};
static Sint16 dat_0cb8d64c[16] = {
#include "ev0142/e102_action_2/dat_0cb8d64c.inc"
};
static NJS_COLOR dat_0cb8d66c[36] = {
#include "ev0142/e102_action_2/dat_0cb8d66c.inc"
};
static NJS_COLOR dat_0cb8d6fc[12] = {
#include "ev0142/e102_action_2/dat_0cb8d6fc.inc"
};
static NJS_MESHSET dat_0cb8d72c[2] = {
#include "ev0142/e102_action_2/dat_0cb8d72c.inc"
};
static NJS_POINT3 dat_0cb8d75c[24] = {
#include "ev0142/e102_action_2/dat_0cb8d75c.inc"
};
static NJS_VECTOR dat_0cb8d87c[24] = {
#include "ev0142/e102_action_2/dat_0cb8d87c.inc"
};
static NJS_MODEL dat_0cb8d99c = {
#include "ev0142/e102_action_2/dat_0cb8d99c.inc"
};
static NJS_OBJECT dat_0cb8d9c4 = {
#include "ev0142/e102_action_2/dat_0cb8d9c4.inc"
};
static NJS_MATERIAL dat_0cb8d9f8[3] = {
#include "ev0142/e102_action_2/dat_0cb8d9f8.inc"
};
static Sint16 dat_0cb8da34[10] = {
#include "ev0142/e102_action_2/dat_0cb8da34.inc"
};
static Sint16 dat_0cb8da48[11] = {
#include "ev0142/e102_action_2/dat_0cb8da48.inc"
};
static Sint16 dat_0cb8da5e[5] = {
#include "ev0142/e102_action_2/dat_0cb8da5e.inc"
};
static NJS_COLOR dat_0cb8da68[8] = {
#include "ev0142/e102_action_2/dat_0cb8da68.inc"
};
static NJS_COLOR dat_0cb8da88[10] = {
#include "ev0142/e102_action_2/dat_0cb8da88.inc"
};
static NJS_COLOR dat_0cb8dab0[4] = {
#include "ev0142/e102_action_2/dat_0cb8dab0.inc"
};
static NJS_MESHSET dat_0cb8dac0[3] = {
#include "ev0142/e102_action_2/dat_0cb8dac0.inc"
};
static NJS_POINT3 dat_0cb8db08[9] = {
#include "ev0142/e102_action_2/dat_0cb8db08.inc"
};
static NJS_VECTOR dat_0cb8db74[9] = {
#include "ev0142/e102_action_2/dat_0cb8db74.inc"
};
static NJS_MODEL dat_0cb8dbe0 = {
#include "ev0142/e102_action_2/dat_0cb8dbe0.inc"
};
static NJS_OBJECT dat_0cb8dc08 = {
#include "ev0142/e102_action_2/dat_0cb8dc08.inc"
};
static NJS_OBJECT dat_0cb8dc3c = {
#include "ev0142/e102_action_2/dat_0cb8dc3c.inc"
};
static NJS_OBJECT dat_0cb8dc70 = {
#include "ev0142/e102_action_2/dat_0cb8dc70.inc"
};
static NJS_OBJECT dat_0cb8dca4 = {
#include "ev0142/e102_action_2/dat_0cb8dca4.inc"
};
static NJS_OBJECT dat_0cb8dcd8 = {
#include "ev0142/e102_action_2/dat_0cb8dcd8.inc"
};
static NJS_OBJECT dat_0cb8dd0c = {
#include "ev0142/e102_action_2/dat_0cb8dd0c.inc"
};
static NJS_MATERIAL dat_0cb8dd40[2] = {
#include "ev0142/e102_action_2/dat_0cb8dd40.inc"
};
static Sint16 dat_0cb8dd68[30] = {
#include "ev0142/e102_action_2/dat_0cb8dd68.inc"
};
static Sint16 dat_0cb8dda4[14] = {
#include "ev0142/e102_action_2/dat_0cb8dda4.inc"
};
static NJS_COLOR dat_0cb8ddc0[26] = {
#include "ev0142/e102_action_2/dat_0cb8ddc0.inc"
};
static NJS_COLOR dat_0cb8de28[12] = {
#include "ev0142/e102_action_2/dat_0cb8de28.inc"
};
static NJS_MESHSET dat_0cb8de58[2] = {
#include "ev0142/e102_action_2/dat_0cb8de58.inc"
};
static NJS_POINT3 dat_0cb8de88[16] = {
#include "ev0142/e102_action_2/dat_0cb8de88.inc"
};
static NJS_VECTOR dat_0cb8df48[16] = {
#include "ev0142/e102_action_2/dat_0cb8df48.inc"
};
static NJS_MODEL dat_0cb8e008 = {
#include "ev0142/e102_action_2/dat_0cb8e008.inc"
};
static NJS_OBJECT dat_0cb8e030 = {
#include "ev0142/e102_action_2/dat_0cb8e030.inc"
};
static NJS_MATERIAL dat_0cb8e064[2] = {
#include "ev0142/e102_action_2/dat_0cb8e064.inc"
};
static Sint16 dat_0cb8e08c[19] = {
#include "ev0142/e102_action_2/dat_0cb8e08c.inc"
};
static Sint16 dat_0cb8e0b2[13] = {
#include "ev0142/e102_action_2/dat_0cb8e0b2.inc"
};
static NJS_COLOR dat_0cb8e0cc[18] = {
#include "ev0142/e102_action_2/dat_0cb8e0cc.inc"
};
static NJS_COLOR dat_0cb8e114[10] = {
#include "ev0142/e102_action_2/dat_0cb8e114.inc"
};
static NJS_MESHSET dat_0cb8e13c[2] = {
#include "ev0142/e102_action_2/dat_0cb8e13c.inc"
};
static NJS_POINT3 dat_0cb8e16c[16] = {
#include "ev0142/e102_action_2/dat_0cb8e16c.inc"
};
static NJS_VECTOR dat_0cb8e22c[16] = {
#include "ev0142/e102_action_2/dat_0cb8e22c.inc"
};
static NJS_MODEL dat_0cb8e2ec = {
#include "ev0142/e102_action_2/dat_0cb8e2ec.inc"
};
static NJS_OBJECT dat_0cb8e314 = {
#include "ev0142/e102_action_2/dat_0cb8e314.inc"
};
static NJS_MATERIAL dat_0cb8e348[2] = {
#include "ev0142/e102_action_2/dat_0cb8e348.inc"
};
static Sint16 dat_0cb8e370[19] = {
#include "ev0142/e102_action_2/dat_0cb8e370.inc"
};
static Sint16 dat_0cb8e396[15] = {
#include "ev0142/e102_action_2/dat_0cb8e396.inc"
};
static NJS_COLOR dat_0cb8e3b4[18] = {
#include "ev0142/e102_action_2/dat_0cb8e3b4.inc"
};
static NJS_COLOR dat_0cb8e3fc[12] = {
#include "ev0142/e102_action_2/dat_0cb8e3fc.inc"
};
static NJS_MESHSET dat_0cb8e42c[2] = {
#include "ev0142/e102_action_2/dat_0cb8e42c.inc"
};
static NJS_POINT3 dat_0cb8e45c[16] = {
#include "ev0142/e102_action_2/dat_0cb8e45c.inc"
};
static NJS_VECTOR dat_0cb8e51c[16] = {
#include "ev0142/e102_action_2/dat_0cb8e51c.inc"
};
static NJS_MODEL dat_0cb8e5dc = {
#include "ev0142/e102_action_2/dat_0cb8e5dc.inc"
};
static NJS_OBJECT dat_0cb8e604 = {
#include "ev0142/e102_action_2/dat_0cb8e604.inc"
};
static NJS_MATERIAL dat_0cb8e638[2] = {
#include "ev0142/e102_action_2/dat_0cb8e638.inc"
};
static Sint16 dat_0cb8e660[12] = {
#include "ev0142/e102_action_2/dat_0cb8e660.inc"
};
static Sint16 dat_0cb8e678[14] = {
#include "ev0142/e102_action_2/dat_0cb8e678.inc"
};
static NJS_COLOR dat_0cb8e694[10] = {
#include "ev0142/e102_action_2/dat_0cb8e694.inc"
};
static NJS_COLOR dat_0cb8e6bc[12] = {
#include "ev0142/e102_action_2/dat_0cb8e6bc.inc"
};
static NJS_MESHSET dat_0cb8e6ec[2] = {
#include "ev0142/e102_action_2/dat_0cb8e6ec.inc"
};
static NJS_POINT3 dat_0cb8e71c[10] = {
#include "ev0142/e102_action_2/dat_0cb8e71c.inc"
};
static NJS_VECTOR dat_0cb8e794[10] = {
#include "ev0142/e102_action_2/dat_0cb8e794.inc"
};
static NJS_MODEL dat_0cb8e80c = {
#include "ev0142/e102_action_2/dat_0cb8e80c.inc"
};
static NJS_OBJECT dat_0cb8e834 = {
#include "ev0142/e102_action_2/dat_0cb8e834.inc"
};
static NJS_MATERIAL dat_0cb8e868[1] = {
#include "ev0142/e102_action_2/dat_0cb8e868.inc"
};
static Sint16 dat_0cb8e87c[154] = {
#include "ev0142/e102_action_2/dat_0cb8e87c.inc"
};
static NJS_COLOR dat_0cb8e9b0[134] = {
#include "ev0142/e102_action_2/dat_0cb8e9b0.inc"
};
static NJS_MESHSET dat_0cb8ebc8[1] = {
#include "ev0142/e102_action_2/dat_0cb8ebc8.inc"
};
static NJS_POINT3 dat_0cb8ebe0[49] = {
#include "ev0142/e102_action_2/dat_0cb8ebe0.inc"
};
static NJS_VECTOR dat_0cb8ee2c[49] = {
#include "ev0142/e102_action_2/dat_0cb8ee2c.inc"
};
static NJS_MODEL dat_0cb8f078 = {
#include "ev0142/e102_action_2/dat_0cb8f078.inc"
};
static NJS_OBJECT dat_0cb8f0a0 = {
#include "ev0142/e102_action_2/dat_0cb8f0a0.inc"
};
static NJS_MATERIAL dat_0cb8f0d4[1] = {
#include "ev0142/e102_action_2/dat_0cb8f0d4.inc"
};
static Sint16 dat_0cb8f0e8[22] = {
#include "ev0142/e102_action_2/dat_0cb8f0e8.inc"
};
static NJS_COLOR dat_0cb8f114[18] = {
#include "ev0142/e102_action_2/dat_0cb8f114.inc"
};
static NJS_MESHSET dat_0cb8f15c[1] = {
#include "ev0142/e102_action_2/dat_0cb8f15c.inc"
};
static NJS_POINT3 dat_0cb8f174[8] = {
#include "ev0142/e102_action_2/dat_0cb8f174.inc"
};
static NJS_VECTOR dat_0cb8f1d4[8] = {
#include "ev0142/e102_action_2/dat_0cb8f1d4.inc"
};
static NJS_MODEL dat_0cb8f234 = {
#include "ev0142/e102_action_2/dat_0cb8f234.inc"
};
static NJS_OBJECT dat_0cb8f25c = {
#include "ev0142/e102_action_2/dat_0cb8f25c.inc"
};
static NJS_MATERIAL dat_0cb8f290[2] = {
#include "ev0142/e102_action_2/dat_0cb8f290.inc"
};
static Sint16 dat_0cb8f2b8[11] = {
#include "ev0142/e102_action_2/dat_0cb8f2b8.inc"
};
static Sint16 dat_0cb8f2ce[5] = {
#include "ev0142/e102_action_2/dat_0cb8f2ce.inc"
};
static NJS_COLOR dat_0cb8f2d8[10] = {
#include "ev0142/e102_action_2/dat_0cb8f2d8.inc"
};
static NJS_COLOR dat_0cb8f300[4] = {
#include "ev0142/e102_action_2/dat_0cb8f300.inc"
};
static NJS_MESHSET dat_0cb8f310[2] = {
#include "ev0142/e102_action_2/dat_0cb8f310.inc"
};
static NJS_POINT3 dat_0cb8f340[8] = {
#include "ev0142/e102_action_2/dat_0cb8f340.inc"
};
static NJS_VECTOR dat_0cb8f3a0[8] = {
#include "ev0142/e102_action_2/dat_0cb8f3a0.inc"
};
static NJS_MODEL dat_0cb8f400 = {
#include "ev0142/e102_action_2/dat_0cb8f400.inc"
};
static NJS_OBJECT dat_0cb8f428 = {
#include "ev0142/e102_action_2/dat_0cb8f428.inc"
};
static NJS_MATERIAL dat_0cb8f45c[2] = {
#include "ev0142/e102_action_2/dat_0cb8f45c.inc"
};
static Sint16 dat_0cb8f484[10] = {
#include "ev0142/e102_action_2/dat_0cb8f484.inc"
};
static Sint16 dat_0cb8f498[14] = {
#include "ev0142/e102_action_2/dat_0cb8f498.inc"
};
static NJS_COLOR dat_0cb8f4b4[8] = {
#include "ev0142/e102_action_2/dat_0cb8f4b4.inc"
};
static NJS_COLOR dat_0cb8f4d4[12] = {
#include "ev0142/e102_action_2/dat_0cb8f4d4.inc"
};
static NJS_MESHSET dat_0cb8f504[2] = {
#include "ev0142/e102_action_2/dat_0cb8f504.inc"
};
static NJS_POINT3 dat_0cb8f534[8] = {
#include "ev0142/e102_action_2/dat_0cb8f534.inc"
};
static NJS_VECTOR dat_0cb8f594[8] = {
#include "ev0142/e102_action_2/dat_0cb8f594.inc"
};
static NJS_MODEL dat_0cb8f5f4 = {
#include "ev0142/e102_action_2/dat_0cb8f5f4.inc"
};
static NJS_OBJECT dat_0cb8f61c = {
#include "ev0142/e102_action_2/dat_0cb8f61c.inc"
};
static NJS_MATERIAL dat_0cb8f650[1] = {
#include "ev0142/e102_action_2/dat_0cb8f650.inc"
};
static Sint16 dat_0cb8f664[12] = {
#include "ev0142/e102_action_2/dat_0cb8f664.inc"
};
static NJS_COLOR dat_0cb8f67c[10] = {
#include "ev0142/e102_action_2/dat_0cb8f67c.inc"
};
static NJS_MESHSET dat_0cb8f6a4[1] = {
#include "ev0142/e102_action_2/dat_0cb8f6a4.inc"
};
static NJS_POINT3 dat_0cb8f6bc[8] = {
#include "ev0142/e102_action_2/dat_0cb8f6bc.inc"
};
static NJS_VECTOR dat_0cb8f71c[8] = {
#include "ev0142/e102_action_2/dat_0cb8f71c.inc"
};
static NJS_MODEL dat_0cb8f77c = {
#include "ev0142/e102_action_2/dat_0cb8f77c.inc"
};
static NJS_OBJECT dat_0cb8f7a4 = {
#include "ev0142/e102_action_2/dat_0cb8f7a4.inc"
};
static NJS_MATERIAL dat_0cb8f7d8[1] = {
#include "ev0142/e102_action_2/dat_0cb8f7d8.inc"
};
static Sint16 dat_0cb8f7ec[22] = {
#include "ev0142/e102_action_2/dat_0cb8f7ec.inc"
};
static NJS_COLOR dat_0cb8f818[18] = {
#include "ev0142/e102_action_2/dat_0cb8f818.inc"
};
static NJS_MESHSET dat_0cb8f860[1] = {
#include "ev0142/e102_action_2/dat_0cb8f860.inc"
};
static NJS_POINT3 dat_0cb8f878[8] = {
#include "ev0142/e102_action_2/dat_0cb8f878.inc"
};
static NJS_VECTOR dat_0cb8f8d8[8] = {
#include "ev0142/e102_action_2/dat_0cb8f8d8.inc"
};
static NJS_MODEL dat_0cb8f938 = {
#include "ev0142/e102_action_2/dat_0cb8f938.inc"
};
static NJS_OBJECT dat_0cb8f960 = {
#include "ev0142/e102_action_2/dat_0cb8f960.inc"
};
static NJS_MATERIAL dat_0cb8f994[2] = {
#include "ev0142/e102_action_2/dat_0cb8f994.inc"
};
static Sint16 dat_0cb8f9bc[14] = {
#include "ev0142/e102_action_2/dat_0cb8f9bc.inc"
};
static Sint16 dat_0cb8f9d8[6] = {
#include "ev0142/e102_action_2/dat_0cb8f9d8.inc"
};
static NJS_COLOR dat_0cb8f9e4[12] = {
#include "ev0142/e102_action_2/dat_0cb8f9e4.inc"
};
static NJS_COLOR dat_0cb8fa14[4] = {
#include "ev0142/e102_action_2/dat_0cb8fa14.inc"
};
static NJS_MESHSET dat_0cb8fa24[2] = {
#include "ev0142/e102_action_2/dat_0cb8fa24.inc"
};
static NJS_POINT3 dat_0cb8fa54[8] = {
#include "ev0142/e102_action_2/dat_0cb8fa54.inc"
};
static NJS_VECTOR dat_0cb8fab4[8] = {
#include "ev0142/e102_action_2/dat_0cb8fab4.inc"
};
static NJS_MODEL dat_0cb8fb14 = {
#include "ev0142/e102_action_2/dat_0cb8fb14.inc"
};
static NJS_OBJECT dat_0cb8fb3c = {
#include "ev0142/e102_action_2/dat_0cb8fb3c.inc"
};
static NJS_MATERIAL dat_0cb8fb70[2] = {
#include "ev0142/e102_action_2/dat_0cb8fb70.inc"
};
static Sint16 dat_0cb8fb98[10] = {
#include "ev0142/e102_action_2/dat_0cb8fb98.inc"
};
static Sint16 dat_0cb8fbac[14] = {
#include "ev0142/e102_action_2/dat_0cb8fbac.inc"
};
static NJS_COLOR dat_0cb8fbc8[8] = {
#include "ev0142/e102_action_2/dat_0cb8fbc8.inc"
};
static NJS_COLOR dat_0cb8fbe8[12] = {
#include "ev0142/e102_action_2/dat_0cb8fbe8.inc"
};
static NJS_MESHSET dat_0cb8fc18[2] = {
#include "ev0142/e102_action_2/dat_0cb8fc18.inc"
};
static NJS_POINT3 dat_0cb8fc48[8] = {
#include "ev0142/e102_action_2/dat_0cb8fc48.inc"
};
static NJS_VECTOR dat_0cb8fca8[8] = {
#include "ev0142/e102_action_2/dat_0cb8fca8.inc"
};
static NJS_MODEL dat_0cb8fd08 = {
#include "ev0142/e102_action_2/dat_0cb8fd08.inc"
};
static NJS_OBJECT dat_0cb8fd30 = {
#include "ev0142/e102_action_2/dat_0cb8fd30.inc"
};
static NJS_MATERIAL dat_0cb8fd64[1] = {
#include "ev0142/e102_action_2/dat_0cb8fd64.inc"
};
static Sint16 dat_0cb8fd78[12] = {
#include "ev0142/e102_action_2/dat_0cb8fd78.inc"
};
static NJS_COLOR dat_0cb8fd90[10] = {
#include "ev0142/e102_action_2/dat_0cb8fd90.inc"
};
static NJS_MESHSET dat_0cb8fdb8[1] = {
#include "ev0142/e102_action_2/dat_0cb8fdb8.inc"
};
static NJS_POINT3 dat_0cb8fdd0[8] = {
#include "ev0142/e102_action_2/dat_0cb8fdd0.inc"
};
static NJS_VECTOR dat_0cb8fe30[8] = {
#include "ev0142/e102_action_2/dat_0cb8fe30.inc"
};
static NJS_MODEL dat_0cb8fe90 = {
#include "ev0142/e102_action_2/dat_0cb8fe90.inc"
};
static NJS_OBJECT dat_0cb8feb8 = {
#include "ev0142/e102_action_2/dat_0cb8feb8.inc"
};
static NJS_MATERIAL dat_0cb8feec[2] = {
#include "ev0142/e102_action_2/dat_0cb8feec.inc"
};
static Sint16 dat_0cb8ff14[7] = {
#include "ev0142/e102_action_2/dat_0cb8ff14.inc"
};
static Sint16 dat_0cb8ff22[15] = {
#include "ev0142/e102_action_2/dat_0cb8ff22.inc"
};
static NJS_COLOR dat_0cb8ff40[6] = {
#include "ev0142/e102_action_2/dat_0cb8ff40.inc"
};
static NJS_COLOR dat_0cb8ff58[12] = {
#include "ev0142/e102_action_2/dat_0cb8ff58.inc"
};
static NJS_MESHSET dat_0cb8ff88[2] = {
#include "ev0142/e102_action_2/dat_0cb8ff88.inc"
};
static NJS_POINT3 dat_0cb8ffb8[8] = {
#include "ev0142/e102_action_2/dat_0cb8ffb8.inc"
};
static NJS_VECTOR dat_0cb90018[8] = {
#include "ev0142/e102_action_2/dat_0cb90018.inc"
};
static NJS_MODEL dat_0cb90078 = {
#include "ev0142/e102_action_2/dat_0cb90078.inc"
};
static NJS_OBJECT dat_0cb900a0 = {
#include "ev0142/e102_action_2/dat_0cb900a0.inc"
};
static NJS_MATERIAL dat_0cb900d4[1] = {
#include "ev0142/e102_action_2/dat_0cb900d4.inc"
};
static Sint16 dat_0cb900e8[86] = {
#include "ev0142/e102_action_2/dat_0cb900e8.inc"
};
static NJS_COLOR dat_0cb90194[76] = {
#include "ev0142/e102_action_2/dat_0cb90194.inc"
};
static NJS_MESHSET dat_0cb902c4[1] = {
#include "ev0142/e102_action_2/dat_0cb902c4.inc"
};
static NJS_POINT3 dat_0cb902dc[32] = {
#include "ev0142/e102_action_2/dat_0cb902dc.inc"
};
static NJS_VECTOR dat_0cb9045c[32] = {
#include "ev0142/e102_action_2/dat_0cb9045c.inc"
};
static NJS_MODEL dat_0cb905dc = {
#include "ev0142/e102_action_2/dat_0cb905dc.inc"
};
static NJS_OBJECT dat_0cb90604 = {
#include "ev0142/e102_action_2/dat_0cb90604.inc"
};
static NJS_MATERIAL dat_0cb90638[2] = {
#include "ev0142/e102_action_2/dat_0cb90638.inc"
};
static Sint16 dat_0cb90660[25] = {
#include "ev0142/e102_action_2/dat_0cb90660.inc"
};
static Sint16 dat_0cb90692[13] = {
#include "ev0142/e102_action_2/dat_0cb90692.inc"
};
static NJS_COLOR dat_0cb906ac[20] = {
#include "ev0142/e102_action_2/dat_0cb906ac.inc"
};
static NJS_COLOR dat_0cb906fc[10] = {
#include "ev0142/e102_action_2/dat_0cb906fc.inc"
};
static NJS_MESHSET dat_0cb90724[2] = {
#include "ev0142/e102_action_2/dat_0cb90724.inc"
};
static NJS_POINT3 dat_0cb90754[10] = {
#include "ev0142/e102_action_2/dat_0cb90754.inc"
};
static NJS_VECTOR dat_0cb907cc[10] = {
#include "ev0142/e102_action_2/dat_0cb907cc.inc"
};
static NJS_MODEL dat_0cb90844 = {
#include "ev0142/e102_action_2/dat_0cb90844.inc"
};
static NJS_OBJECT dat_0cb9086c = {
#include "ev0142/e102_action_2/dat_0cb9086c.inc"
};
static NJS_MATERIAL dat_0cb908a0[2] = {
#include "ev0142/e102_action_2/dat_0cb908a0.inc"
};
static Sint16 dat_0cb908c8[24] = {
#include "ev0142/e102_action_2/dat_0cb908c8.inc"
};
static Sint16 dat_0cb908f8[16] = {
#include "ev0142/e102_action_2/dat_0cb908f8.inc"
};
static NJS_COLOR dat_0cb90918[22] = {
#include "ev0142/e102_action_2/dat_0cb90918.inc"
};
static NJS_COLOR dat_0cb90970[13] = {
#include "ev0142/e102_action_2/dat_0cb90970.inc"
};
static NJS_MESHSET dat_0cb909a4[2] = {
#include "ev0142/e102_action_2/dat_0cb909a4.inc"
};
static NJS_POINT3 dat_0cb909d4[18] = {
#include "ev0142/e102_action_2/dat_0cb909d4.inc"
};
static NJS_VECTOR dat_0cb90aac[18] = {
#include "ev0142/e102_action_2/dat_0cb90aac.inc"
};
static NJS_MODEL dat_0cb90b84 = {
#include "ev0142/e102_action_2/dat_0cb90b84.inc"
};
static NJS_OBJECT dat_0cb90bac = {
#include "ev0142/e102_action_2/dat_0cb90bac.inc"
};
static NJS_OBJECT dat_0cb90be0 = {
#include "ev0142/e102_action_2/dat_0cb90be0.inc"
};
static NJS_MATERIAL dat_0cb90c14[1] = {
#include "ev0142/e102_action_2/dat_0cb90c14.inc"
};
static Sint16 dat_0cb90c28[8] = {
#include "ev0142/e102_action_2/dat_0cb90c28.inc"
};
static NJS_COLOR dat_0cb90c38[6] = {
#include "ev0142/e102_action_2/dat_0cb90c38.inc"
};
static NJS_MESHSET dat_0cb90c50[1] = {
#include "ev0142/e102_action_2/dat_0cb90c50.inc"
};
static NJS_POINT3 dat_0cb90c68[4] = {
#include "ev0142/e102_action_2/dat_0cb90c68.inc"
};
static NJS_VECTOR dat_0cb90c98[4] = {
#include "ev0142/e102_action_2/dat_0cb90c98.inc"
};
static NJS_MODEL dat_0cb90cc8 = {
#include "ev0142/e102_action_2/dat_0cb90cc8.inc"
};
static NJS_OBJECT dat_0cb90cf0 = {
#include "ev0142/e102_action_2/dat_0cb90cf0.inc"
};
static NJS_MATERIAL dat_0cb90d24[2] = {
#include "ev0142/e102_action_2/dat_0cb90d24.inc"
};
static Sint16 dat_0cb90d4c[25] = {
#include "ev0142/e102_action_2/dat_0cb90d4c.inc"
};
static Sint16 dat_0cb90d7e[11] = {
#include "ev0142/e102_action_2/dat_0cb90d7e.inc"
};
static NJS_COLOR dat_0cb90d94[22] = {
#include "ev0142/e102_action_2/dat_0cb90d94.inc"
};
static NJS_COLOR dat_0cb90dec[8] = {
#include "ev0142/e102_action_2/dat_0cb90dec.inc"
};
static NJS_MESHSET dat_0cb90e0c[2] = {
#include "ev0142/e102_action_2/dat_0cb90e0c.inc"
};
static NJS_POINT3 dat_0cb90e3c[12] = {
#include "ev0142/e102_action_2/dat_0cb90e3c.inc"
};
static NJS_VECTOR dat_0cb90ecc[12] = {
#include "ev0142/e102_action_2/dat_0cb90ecc.inc"
};
static NJS_MODEL dat_0cb90f5c = {
#include "ev0142/e102_action_2/dat_0cb90f5c.inc"
};
static NJS_OBJECT dat_0cb90f84 = {
#include "ev0142/e102_action_2/dat_0cb90f84.inc"
};
static NJS_MATERIAL dat_0cb90fb8[2] = {
#include "ev0142/e102_action_2/dat_0cb90fb8.inc"
};
static Sint16 dat_0cb90fe0[15] = {
#include "ev0142/e102_action_2/dat_0cb90fe0.inc"
};
static Sint16 dat_0cb90ffe[7] = {
#include "ev0142/e102_action_2/dat_0cb90ffe.inc"
};
static NJS_COLOR dat_0cb9100c[14] = {
#include "ev0142/e102_action_2/dat_0cb9100c.inc"
};
static NJS_COLOR dat_0cb91044[6] = {
#include "ev0142/e102_action_2/dat_0cb91044.inc"
};
static NJS_MESHSET dat_0cb9105c[2] = {
#include "ev0142/e102_action_2/dat_0cb9105c.inc"
};
static NJS_POINT3 dat_0cb9108c[12] = {
#include "ev0142/e102_action_2/dat_0cb9108c.inc"
};
static NJS_VECTOR dat_0cb9111c[12] = {
#include "ev0142/e102_action_2/dat_0cb9111c.inc"
};
static NJS_MODEL dat_0cb911ac = {
#include "ev0142/e102_action_2/dat_0cb911ac.inc"
};
static NJS_OBJECT dat_0cb911d4 = {
#include "ev0142/e102_action_2/dat_0cb911d4.inc"
};
static NJS_MATERIAL dat_0cb91208[2] = {
#include "ev0142/e102_action_2/dat_0cb91208.inc"
};
static Sint16 dat_0cb91230[15] = {
#include "ev0142/e102_action_2/dat_0cb91230.inc"
};
static Sint16 dat_0cb9124e[11] = {
#include "ev0142/e102_action_2/dat_0cb9124e.inc"
};
static NJS_COLOR dat_0cb91264[14] = {
#include "ev0142/e102_action_2/dat_0cb91264.inc"
};
static NJS_COLOR dat_0cb9129c[8] = {
#include "ev0142/e102_action_2/dat_0cb9129c.inc"
};
static NJS_MESHSET dat_0cb912bc[2] = {
#include "ev0142/e102_action_2/dat_0cb912bc.inc"
};
static NJS_POINT3 dat_0cb912ec[12] = {
#include "ev0142/e102_action_2/dat_0cb912ec.inc"
};
static NJS_VECTOR dat_0cb9137c[12] = {
#include "ev0142/e102_action_2/dat_0cb9137c.inc"
};
static NJS_MODEL dat_0cb9140c = {
#include "ev0142/e102_action_2/dat_0cb9140c.inc"
};
static NJS_OBJECT dat_0cb91434 = {
#include "ev0142/e102_action_2/dat_0cb91434.inc"
};
static NJS_MATERIAL dat_0cb91468[6] = {
#include "ev0142/e102_action_2/dat_0cb91468.inc"
};
static Sint16 dat_0cb914e0[81] = {
#include "ev0142/e102_action_2/dat_0cb914e0.inc"
};
static Sint16 dat_0cb91582[17] = {
#include "ev0142/e102_action_2/dat_0cb91582.inc"
};
static Sint16 dat_0cb915a4[16] = {
#include "ev0142/e102_action_2/dat_0cb915a4.inc"
};
static Sint16 dat_0cb915c4[40] = {
#include "ev0142/e102_action_2/dat_0cb915c4.inc"
};
static Sint16 dat_0cb91614[11] = {
#include "ev0142/e102_action_2/dat_0cb91614.inc"
};
static Sint16 dat_0cb9162a[11] = {
#include "ev0142/e102_action_2/dat_0cb9162a.inc"
};
static NJS_COLOR dat_0cb91640[70] = {
#include "ev0142/e102_action_2/dat_0cb91640.inc"
};
static NJS_COLOR dat_0cb91758[14] = {
#include "ev0142/e102_action_2/dat_0cb91758.inc"
};
static NJS_COLOR dat_0cb91790[14] = {
#include "ev0142/e102_action_2/dat_0cb91790.inc"
};
static NJS_COLOR dat_0cb917c8[35] = {
#include "ev0142/e102_action_2/dat_0cb917c8.inc"
};
static NJS_COLOR dat_0cb91854[9] = {
#include "ev0142/e102_action_2/dat_0cb91854.inc"
};
static NJS_COLOR dat_0cb91878[9] = {
#include "ev0142/e102_action_2/dat_0cb91878.inc"
};
static NJS_MESHSET dat_0cb9189c[6] = {
#include "ev0142/e102_action_2/dat_0cb9189c.inc"
};
static NJS_POINT3 dat_0cb9192c[60] = {
#include "ev0142/e102_action_2/dat_0cb9192c.inc"
};
static NJS_VECTOR dat_0cb91bfc[60] = {
#include "ev0142/e102_action_2/dat_0cb91bfc.inc"
};
static NJS_MODEL dat_0cb91ecc = {
#include "ev0142/e102_action_2/dat_0cb91ecc.inc"
};
static NJS_OBJECT dat_0cb91ef4 = {
#include "ev0142/e102_action_2/dat_0cb91ef4.inc"
};
static NJS_OBJECT dat_0cb91f28 = {
#include "ev0142/e102_action_2/dat_0cb91f28.inc"
};
static NJS_OBJECT dat_0cb91f5c = {
#include "ev0142/e102_action_2/dat_0cb91f5c.inc"
};
static NJS_OBJECT dat_0cb91f90 = {
#include "ev0142/e102_action_2/dat_0cb91f90.inc"
};
static NJS_MATERIAL dat_0cb91fc4[6] = {
#include "ev0142/e102_action_2/dat_0cb91fc4.inc"
};
static Sint16 dat_0cb9203c[35] = {
#include "ev0142/e102_action_2/dat_0cb9203c.inc"
};
static Sint16 dat_0cb92082[124] = {
#include "ev0142/e102_action_2/dat_0cb92082.inc"
};
static Sint16 dat_0cb9217a[25] = {
#include "ev0142/e102_action_2/dat_0cb9217a.inc"
};
static Sint16 dat_0cb921ac[10] = {
#include "ev0142/e102_action_2/dat_0cb921ac.inc"
};
static Sint16 dat_0cb921c0[18] = {
#include "ev0142/e102_action_2/dat_0cb921c0.inc"
};
static Sint16 dat_0cb921e4[14] = {
#include "ev0142/e102_action_2/dat_0cb921e4.inc"
};
static NJS_COLOR dat_0cb92200[30] = {
#include "ev0142/e102_action_2/dat_0cb92200.inc"
};
static NJS_COLOR dat_0cb92278[110] = {
#include "ev0142/e102_action_2/dat_0cb92278.inc"
};
static NJS_COLOR dat_0cb92430[20] = {
#include "ev0142/e102_action_2/dat_0cb92430.inc"
};
static NJS_COLOR dat_0cb92480[8] = {
#include "ev0142/e102_action_2/dat_0cb92480.inc"
};
static NJS_COLOR dat_0cb924a0[16] = {
#include "ev0142/e102_action_2/dat_0cb924a0.inc"
};
static NJS_COLOR dat_0cb924e0[12] = {
#include "ev0142/e102_action_2/dat_0cb924e0.inc"
};
static NJS_MESHSET dat_0cb92510[6] = {
#include "ev0142/e102_action_2/dat_0cb92510.inc"
};
static NJS_POINT3 dat_0cb925a0[72] = {
#include "ev0142/e102_action_2/dat_0cb925a0.inc"
};
static NJS_VECTOR dat_0cb92900[72] = {
#include "ev0142/e102_action_2/dat_0cb92900.inc"
};
static NJS_MODEL dat_0cb92c60 = {
#include "ev0142/e102_action_2/dat_0cb92c60.inc"
};
static NJS_OBJECT dat_0cb92c88 = {
#include "ev0142/e102_action_2/dat_0cb92c88.inc"
};
static NJS_OBJECT dat_0cb92cbc = {
#include "ev0142/e102_action_2/dat_0cb92cbc.inc"
};
static NJS_OBJECT dat_0cb92cf0 = {
#include "ev0142/e102_action_2/dat_0cb92cf0.inc"
};
static NJS_OBJECT dat_0cb92d24 = {
#include "ev0142/e102_action_2/dat_0cb92d24.inc"
};
static NJS_OBJECT dat_0cb92d58 = {
#include "ev0142/e102_action_2/dat_0cb92d58.inc"
};
static NJS_OBJECT dat_0cb92d8c = {
#include "ev0142/e102_action_2/dat_0cb92d8c.inc"
};
static NJS_OBJECT dat_0cb92dc0 = {
#include "ev0142/e102_action_2/dat_0cb92dc0.inc"
};
static NJS_OBJECT dat_0cb92df4 = {
#include "ev0142/e102_action_2/dat_0cb92df4.inc"
};
static NJS_OBJECT dat_0cb92e28 = {
#include "ev0142/e102_action_2/dat_0cb92e28.inc"
};
static NJS_MKEY_F dat_0cb92e5c[59] = {
#include "ev0142/e102_action_2/dat_0cb92e5c.inc"
};
static NJS_MKEY_A dat_0cb9320c[57] = {
#include "ev0142/e102_action_2/dat_0cb9320c.inc"
};
static NJS_MKEY_A dat_0cb9359c[58] = {
#include "ev0142/e102_action_2/dat_0cb9359c.inc"
};
static NJS_MKEY_A dat_0cb9393c[59] = {
#include "ev0142/e102_action_2/dat_0cb9393c.inc"
};
static NJS_MKEY_A dat_0cb93cec[59] = {
#include "ev0142/e102_action_2/dat_0cb93cec.inc"
};
static NJS_MKEY_A dat_0cb9409c[59] = {
#include "ev0142/e102_action_2/dat_0cb9409c.inc"
};
static NJS_MKEY_A dat_0cb9444c[59] = {
#include "ev0142/e102_action_2/dat_0cb9444c.inc"
};
static NJS_MKEY_A dat_0cb947fc[59] = {
#include "ev0142/e102_action_2/dat_0cb947fc.inc"
};
static NJS_MKEY_A dat_0cb94bac[56] = {
#include "ev0142/e102_action_2/dat_0cb94bac.inc"
};
static NJS_MKEY_A dat_0cb94f2c[58] = {
#include "ev0142/e102_action_2/dat_0cb94f2c.inc"
};
static NJS_MKEY_A dat_0cb952cc[58] = {
#include "ev0142/e102_action_2/dat_0cb952cc.inc"
};
static NJS_MKEY_A dat_0cb9566c[55] = {
#include "ev0142/e102_action_2/dat_0cb9566c.inc"
};
static NJS_MDATA2 dat_0cb959dc[65] = {
#include "ev0142/e102_action_2/dat_0cb959dc.inc"
};
static NJS_MOTION ev0142_e101_motion = {
#include "ev0142/e102_action_2/ev0142_e101_motion.inc"
};
NJS_ACTION ev0142_e101_action[1] = {
#include "ev0142/e102_action_2/ev0142_e101_action.inc"
};
NJS_TEXNAME ev0142_memory_textures[2] = {
    {TEXT_33, 0, 0},
    {TEXT_34, 0, 0},
};
