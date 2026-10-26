/* Big's texture names and texlist, his models, motions and actions, then
 * the fishing texture names and fishing_texlist. the last datum,
 * dat_8c56345c, is the material of a model in the generated data placed
 * after it. the notes beside this file's rows in splits.txt infer from
 * alignment that the fishing data and that material share Big's object */
#include <ninja.h>
#include <sg_xpt.h>

#include "player/big_models.h"
#include "player/textures.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 7
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 10
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 12
#define TEXT_27_LENGTH 12
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 10
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 10
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 8
#define TEXT_39_LENGTH 8
#define TEXT_40_LENGTH 8
#define TEXT_41_LENGTH 8
#define TEXT_42_LENGTH 8
#define TEXT_43_LENGTH 7
#define TEXT_44_LENGTH 7
#define TEXT_45_LENGTH 7
#define TEXT_46_LENGTH 7
#define TEXT_47_LENGTH 7
#define TEXT_48_LENGTH 7
#define TEXT_49_LENGTH 7
#define TEXT_50_LENGTH 7
#define TEXT_51_LENGTH 7
#define TEXT_52_LENGTH 7
#define TEXT_53_LENGTH 8
#define TEXT_54_LENGTH 8
#define TEXT_55_LENGTH 8
#define TEXT_56_LENGTH 8
#define TEXT_57_LENGTH 8
#define TEXT_58_LENGTH 8
#define TEXT_59_LENGTH 8
#define TEXT_60_LENGTH 8
#define TEXT_61_LENGTH 8
#define TEXT_62_LENGTH 8
#define TEXT_63_LENGTH 8
#define TEXT_64_LENGTH 9
#define TEXT_65_LENGTH 8
#define TEXT_66_LENGTH 10
#define TEXT_67_LENGTH 9
#define TEXT_68_LENGTH 8
#define TEXT_69_LENGTH 8
#define TEXT_70_LENGTH 8
#define TEXT_71_LENGTH 8
#define TEXT_72_LENGTH 8
#define TEXT_73_LENGTH 8
#define TEXT_74_LENGTH 8
#define TEXT_75_LENGTH 8
#define TEXT_76_LENGTH 8
#define TEXT_77_LENGTH 9
#define TEXT_78_LENGTH 9
#define TEXT_79_LENGTH 9
#define TEXT_80_LENGTH 9
#define TEXT_81_LENGTH 9
#define TEXT_82_LENGTH 9
#define TEXT_83_LENGTH 9
#define TEXT_84_LENGTH 13
#define TEXT_85_LENGTH 13
#define TEXT_86_LENGTH 8
#define TEXT_87_LENGTH 8
#define TEXT_88_LENGTH 8
#define TEXT_89_LENGTH 8
#define TEXT_90_LENGTH 8
#define TEXT_91_LENGTH 8
#define TEXT_92_LENGTH 8
#define TEXT_93_LENGTH 8
#define TEXT_94_LENGTH 10
#define TEXT_95_LENGTH 10
#define TEXT_96_LENGTH 10
#define TEXT_97_LENGTH 7
#define TEXT_98_LENGTH 9
#define TEXT_99_LENGTH 8
#define TEXT_100_LENGTH 4
#define TEXT_101_LENGTH 4
#include "player/big_textures/text.inc"

static NJS_TEXNAME big_textures[36] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
};
NJS_TEXLIST big_texlist = {
#include "player/big_textures/big_texlist.inc"
};
static NJS_MATERIAL dat_8c53c8c0[1] = {
#include "player/big_textures/dat_8c53c8c0.inc"
};
static Sint16 dat_8c53c8d4[5] = {
#include "player/big_textures/dat_8c53c8d4.inc"
};
static NJS_COLOR dat_8c53c8e0[4] = {
#include "player/big_textures/dat_8c53c8e0.inc"
};
static NJS_MESHSET dat_8c53c8f0[1] = {
#include "player/big_textures/dat_8c53c8f0.inc"
};
static NJS_POINT3 dat_8c53c908[4] = {
#include "player/big_textures/dat_8c53c908.inc"
};
static NJS_VECTOR dat_8c53c938[4] = {
#include "player/big_textures/dat_8c53c938.inc"
};
static NJS_MODEL dat_8c53c968 = {
#include "player/big_textures/dat_8c53c968.inc"
};
static NJS_OBJECT dat_8c53c990 = {
#include "player/big_textures/dat_8c53c990.inc"
};
static NJS_MATERIAL dat_8c53c9c4[1] = {
#include "player/big_textures/dat_8c53c9c4.inc"
};
static Sint16 dat_8c53c9d8[5] = {
#include "player/big_textures/dat_8c53c9d8.inc"
};
static NJS_COLOR dat_8c53c9e4[4] = {
#include "player/big_textures/dat_8c53c9e4.inc"
};
static NJS_MESHSET dat_8c53c9f4[1] = {
#include "player/big_textures/dat_8c53c9f4.inc"
};
static NJS_POINT3 dat_8c53ca0c[4] = {
#include "player/big_textures/dat_8c53ca0c.inc"
};
static NJS_VECTOR dat_8c53ca3c[4] = {
#include "player/big_textures/dat_8c53ca3c.inc"
};
static NJS_MODEL dat_8c53ca6c = {
#include "player/big_textures/dat_8c53ca6c.inc"
};
static NJS_OBJECT dat_8c53ca94 = {
#include "player/big_textures/dat_8c53ca94.inc"
};
static NJS_MATERIAL dat_8c53cac8[5] = {
#include "player/big_textures/dat_8c53cac8.inc"
};
static Sint16 dat_8c53cb2c[111] = {
#include "player/big_textures/dat_8c53cb2c.inc"
};
static Sint16 dat_8c53cc0a[40] = {
#include "player/big_textures/dat_8c53cc0a.inc"
};
static Sint16 dat_8c53cc5a[31] = {
#include "player/big_textures/dat_8c53cc5a.inc"
};
static Sint16 dat_8c53cc98[63] = {
#include "player/big_textures/dat_8c53cc98.inc"
};
static Sint16 dat_8c53cd16[10] = {
#include "player/big_textures/dat_8c53cd16.inc"
};
static NJS_COLOR dat_8c53cd2c[90] = {
#include "player/big_textures/dat_8c53cd2c.inc"
};
static NJS_COLOR dat_8c53ce94[32] = {
#include "player/big_textures/dat_8c53ce94.inc"
};
static NJS_COLOR dat_8c53cf14[26] = {
#include "player/big_textures/dat_8c53cf14.inc"
};
static NJS_COLOR dat_8c53cf7c[56] = {
#include "player/big_textures/dat_8c53cf7c.inc"
};
static NJS_COLOR dat_8c53d05c[8] = {
#include "player/big_textures/dat_8c53d05c.inc"
};
static NJS_MESHSET dat_8c53d07c[5] = {
#include "player/big_textures/dat_8c53d07c.inc"
};
static NJS_POINT3 dat_8c53d0f4[69] = {
#include "player/big_textures/dat_8c53d0f4.inc"
};
static NJS_VECTOR dat_8c53d430[69] = {
#include "player/big_textures/dat_8c53d430.inc"
};
static NJS_MODEL dat_8c53d76c = {
#include "player/big_textures/dat_8c53d76c.inc"
};
NJS_OBJECT big_torso = {
#include "player/big_textures/big_torso.inc"
};
static NJS_OBJECT dat_8c53d7c8 = {
#include "player/big_textures/dat_8c53d7c8.inc"
};
static NJS_MATERIAL dat_8c53d7fc[1] = {
#include "player/big_textures/dat_8c53d7fc.inc"
};
static Sint16 dat_8c53d810[15] = {
#include "player/big_textures/dat_8c53d810.inc"
};
static NJS_COLOR dat_8c53d830[12] = {
#include "player/big_textures/dat_8c53d830.inc"
};
static NJS_MESHSET dat_8c53d860[1] = {
#include "player/big_textures/dat_8c53d860.inc"
};
static NJS_POINT3 dat_8c53d878[10] = {
#include "player/big_textures/dat_8c53d878.inc"
};
static NJS_VECTOR dat_8c53d8f0[10] = {
#include "player/big_textures/dat_8c53d8f0.inc"
};
static NJS_MODEL dat_8c53d968 = {
#include "player/big_textures/dat_8c53d968.inc"
};
static NJS_OBJECT dat_8c53d990 = {
#include "player/big_textures/dat_8c53d990.inc"
};
static NJS_MATERIAL dat_8c53d9c4[1] = {
#include "player/big_textures/dat_8c53d9c4.inc"
};
static Sint16 dat_8c53d9d8[15] = {
#include "player/big_textures/dat_8c53d9d8.inc"
};
static NJS_COLOR dat_8c53d9f8[12] = {
#include "player/big_textures/dat_8c53d9f8.inc"
};
static NJS_MESHSET dat_8c53da28[1] = {
#include "player/big_textures/dat_8c53da28.inc"
};
static NJS_POINT3 dat_8c53da40[10] = {
#include "player/big_textures/dat_8c53da40.inc"
};
static NJS_VECTOR dat_8c53dab8[10] = {
#include "player/big_textures/dat_8c53dab8.inc"
};
static NJS_MODEL dat_8c53db30 = {
#include "player/big_textures/dat_8c53db30.inc"
};
static NJS_OBJECT dat_8c53db58 = {
#include "player/big_textures/dat_8c53db58.inc"
};
static NJS_MATERIAL dat_8c53db8c[1] = {
#include "player/big_textures/dat_8c53db8c.inc"
};
static Sint16 dat_8c53dba0[10] = {
#include "player/big_textures/dat_8c53dba0.inc"
};
static NJS_COLOR dat_8c53dbb4[8] = {
#include "player/big_textures/dat_8c53dbb4.inc"
};
static NJS_MESHSET dat_8c53dbd4[1] = {
#include "player/big_textures/dat_8c53dbd4.inc"
};
static NJS_POINT3 dat_8c53dbec[6] = {
#include "player/big_textures/dat_8c53dbec.inc"
};
static NJS_VECTOR dat_8c53dc34[6] = {
#include "player/big_textures/dat_8c53dc34.inc"
};
static NJS_MODEL dat_8c53dc7c = {
#include "player/big_textures/dat_8c53dc7c.inc"
};
NJS_OBJECT big_pupil_plus_z = {
#include "player/big_textures/big_pupil_plus_z.inc"
};
static NJS_MATERIAL dat_8c53dcd8[1] = {
#include "player/big_textures/dat_8c53dcd8.inc"
};
static Sint16 dat_8c53dcec[10] = {
#include "player/big_textures/dat_8c53dcec.inc"
};
static NJS_COLOR dat_8c53dd00[8] = {
#include "player/big_textures/dat_8c53dd00.inc"
};
static NJS_MESHSET dat_8c53dd20[1] = {
#include "player/big_textures/dat_8c53dd20.inc"
};
static NJS_POINT3 dat_8c53dd38[6] = {
#include "player/big_textures/dat_8c53dd38.inc"
};
static NJS_VECTOR dat_8c53dd80[6] = {
#include "player/big_textures/dat_8c53dd80.inc"
};
static NJS_MODEL dat_8c53ddc8 = {
#include "player/big_textures/dat_8c53ddc8.inc"
};
NJS_OBJECT big_pupil_minus_z = {
#include "player/big_textures/big_pupil_minus_z.inc"
};
static NJS_MATERIAL dat_8c53de24[1] = {
#include "player/big_textures/dat_8c53de24.inc"
};
static Sint16 dat_8c53de38[17] = {
#include "player/big_textures/dat_8c53de38.inc"
};
static NJS_COLOR dat_8c53de5c[14] = {
#include "player/big_textures/dat_8c53de5c.inc"
};
static NJS_MESHSET dat_8c53de94[1] = {
#include "player/big_textures/dat_8c53de94.inc"
};
static NJS_POINT3 dat_8c53deac[10] = {
#include "player/big_textures/dat_8c53deac.inc"
};
static NJS_VECTOR dat_8c53df24[10] = {
#include "player/big_textures/dat_8c53df24.inc"
};
static NJS_MODEL dat_8c53df9c = {
#include "player/big_textures/dat_8c53df9c.inc"
};
static NJS_OBJECT dat_8c53dfc4 = {
#include "player/big_textures/dat_8c53dfc4.inc"
};
static NJS_MATERIAL dat_8c53dff8[1] = {
#include "player/big_textures/dat_8c53dff8.inc"
};
static Sint16 dat_8c53e00c[20] = {
#include "player/big_textures/dat_8c53e00c.inc"
};
static NJS_COLOR dat_8c53e034[16] = {
#include "player/big_textures/dat_8c53e034.inc"
};
static NJS_MESHSET dat_8c53e074[1] = {
#include "player/big_textures/dat_8c53e074.inc"
};
static NJS_POINT3 dat_8c53e08c[10] = {
#include "player/big_textures/dat_8c53e08c.inc"
};
static NJS_VECTOR dat_8c53e104[10] = {
#include "player/big_textures/dat_8c53e104.inc"
};
static NJS_MODEL dat_8c53e17c = {
#include "player/big_textures/dat_8c53e17c.inc"
};
static NJS_OBJECT dat_8c53e1a4 = {
#include "player/big_textures/dat_8c53e1a4.inc"
};
static NJS_MATERIAL dat_8c53e1d8[7] = {
#include "player/big_textures/dat_8c53e1d8.inc"
};
static Sint16 dat_8c53e264[199] = {
#include "player/big_textures/dat_8c53e264.inc"
};
static Sint16 dat_8c53e3f2[127] = {
#include "player/big_textures/dat_8c53e3f2.inc"
};
static Sint16 dat_8c53e4f0[49] = {
#include "player/big_textures/dat_8c53e4f0.inc"
};
static Sint16 dat_8c53e552[42] = {
#include "player/big_textures/dat_8c53e552.inc"
};
static Sint16 dat_8c53e5a6[32] = {
#include "player/big_textures/dat_8c53e5a6.inc"
};
static Sint16 dat_8c53e5e6[10] = {
#include "player/big_textures/dat_8c53e5e6.inc"
};
static Sint16 dat_8c53e5fa[24] = {
#include "player/big_textures/dat_8c53e5fa.inc"
};
static NJS_COLOR dat_8c53e62c[169] = {
#include "player/big_textures/dat_8c53e62c.inc"
};
static NJS_COLOR dat_8c53e8d0[112] = {
#include "player/big_textures/dat_8c53e8d0.inc"
};
static NJS_COLOR dat_8c53ea90[43] = {
#include "player/big_textures/dat_8c53ea90.inc"
};
static NJS_COLOR dat_8c53eb3c[36] = {
#include "player/big_textures/dat_8c53eb3c.inc"
};
static NJS_COLOR dat_8c53ebcc[28] = {
#include "player/big_textures/dat_8c53ebcc.inc"
};
static NJS_COLOR dat_8c53ec3c[8] = {
#include "player/big_textures/dat_8c53ec3c.inc"
};
static NJS_COLOR dat_8c53ec5c[20] = {
#include "player/big_textures/dat_8c53ec5c.inc"
};
static NJS_MESHSET dat_8c53ecac[7] = {
#include "player/big_textures/dat_8c53ecac.inc"
};
static NJS_POINT3 dat_8c53ed54[143] = {
#include "player/big_textures/dat_8c53ed54.inc"
};
static NJS_VECTOR dat_8c53f408[143] = {
#include "player/big_textures/dat_8c53f408.inc"
};
NJS_MODEL big_head_model = {
#include "player/big_textures/big_head_model.inc"
};
NJS_OBJECT big_head[1] = {
#include "player/big_textures/big_head.inc"
};
static NJS_MATERIAL dat_8c53fb18[2] = {
#include "player/big_textures/dat_8c53fb18.inc"
};
static Sint16 dat_8c53fb40[32] = {
#include "player/big_textures/dat_8c53fb40.inc"
};
static Sint16 dat_8c53fb80[8] = {
#include "player/big_textures/dat_8c53fb80.inc"
};
static NJS_COLOR dat_8c53fb90[28] = {
#include "player/big_textures/dat_8c53fb90.inc"
};
static NJS_COLOR dat_8c53fc00[7] = {
#include "player/big_textures/dat_8c53fc00.inc"
};
static NJS_MESHSET dat_8c53fc1c[2] = {
#include "player/big_textures/dat_8c53fc1c.inc"
};
static NJS_POINT3 dat_8c53fc4c[16] = {
#include "player/big_textures/dat_8c53fc4c.inc"
};
static NJS_VECTOR dat_8c53fd0c[16] = {
#include "player/big_textures/dat_8c53fd0c.inc"
};
static NJS_MODEL dat_8c53fdcc = {
#include "player/big_textures/dat_8c53fdcc.inc"
};
NJS_OBJECT big_mouth = {
#include "player/big_textures/big_mouth.inc"
};
NJS_OBJECT big_head_end = {
#include "player/big_textures/big_head_end.inc"
};
static NJS_OBJECT dat_8c53fe5c = {
#include "player/big_textures/dat_8c53fe5c.inc"
};
NJS_OBJECT big_neck[1] = {
#include "player/big_textures/big_neck.inc"
};
static NJS_OBJECT dat_8c53fec4 = {
#include "player/big_textures/dat_8c53fec4.inc"
};
static NJS_OBJECT dat_8c53fef8 = {
#include "player/big_textures/dat_8c53fef8.inc"
};
static NJS_OBJECT dat_8c53ff2c = {
#include "player/big_textures/dat_8c53ff2c.inc"
};
static NJS_OBJECT dat_8c53ff60 = {
#include "player/big_textures/dat_8c53ff60.inc"
};
static NJS_OBJECT dat_8c53ff94 = {
#include "player/big_textures/dat_8c53ff94.inc"
};
static NJS_OBJECT dat_8c53ffc8 = {
#include "player/big_textures/dat_8c53ffc8.inc"
};
static NJS_MATERIAL dat_8c53fffc[2] = {
#include "player/big_textures/dat_8c53fffc.inc"
};
static Sint16 dat_8c540024[12] = {
#include "player/big_textures/dat_8c540024.inc"
};
static Sint16 dat_8c54003c[38] = {
#include "player/big_textures/dat_8c54003c.inc"
};
static NJS_COLOR dat_8c540088[10] = {
#include "player/big_textures/dat_8c540088.inc"
};
static NJS_COLOR dat_8c5400b0[32] = {
#include "player/big_textures/dat_8c5400b0.inc"
};
static NJS_MESHSET dat_8c540130[2] = {
#include "player/big_textures/dat_8c540130.inc"
};
static NJS_POINT3 dat_8c540160[17] = {
#include "player/big_textures/dat_8c540160.inc"
};
static NJS_VECTOR dat_8c54022c[17] = {
#include "player/big_textures/dat_8c54022c.inc"
};
static NJS_MODEL dat_8c5402f8 = {
#include "player/big_textures/dat_8c5402f8.inc"
};
NJS_OBJECT big_upper_arm_plus_z = {
#include "player/big_textures/big_upper_arm_plus_z.inc"
};
static NJS_MATERIAL dat_8c540354[1] = {
#include "player/big_textures/dat_8c540354.inc"
};
static Sint16 dat_8c540368[45] = {
#include "player/big_textures/dat_8c540368.inc"
};
static NJS_COLOR dat_8c5403c4[38] = {
#include "player/big_textures/dat_8c5403c4.inc"
};
static NJS_MESHSET dat_8c54045c[1] = {
#include "player/big_textures/dat_8c54045c.inc"
};
static NJS_POINT3 dat_8c540474[18] = {
#include "player/big_textures/dat_8c540474.inc"
};
static NJS_VECTOR dat_8c54054c[18] = {
#include "player/big_textures/dat_8c54054c.inc"
};
static NJS_MODEL dat_8c540624 = {
#include "player/big_textures/dat_8c540624.inc"
};
NJS_OBJECT big_forearm_plus_z = {
#include "player/big_textures/big_forearm_plus_z.inc"
};
static NJS_MATERIAL dat_8c540680[2] = {
#include "player/big_textures/dat_8c540680.inc"
};
static Sint16 dat_8c5406a8[28] = {
#include "player/big_textures/dat_8c5406a8.inc"
};
static Sint16 dat_8c5406e0[38] = {
#include "player/big_textures/dat_8c5406e0.inc"
};
static NJS_COLOR dat_8c54072c[24] = {
#include "player/big_textures/dat_8c54072c.inc"
};
static NJS_COLOR dat_8c54078c[32] = {
#include "player/big_textures/dat_8c54078c.inc"
};
static NJS_MESHSET dat_8c54080c[2] = {
#include "player/big_textures/dat_8c54080c.inc"
};
static NJS_POINT3 dat_8c54083c[26] = {
#include "player/big_textures/dat_8c54083c.inc"
};
static NJS_VECTOR dat_8c540974[26] = {
#include "player/big_textures/dat_8c540974.inc"
};
static NJS_MODEL dat_8c540aac = {
#include "player/big_textures/dat_8c540aac.inc"
};
NJS_OBJECT big_hand_plus_z = {
#include "player/big_textures/big_hand_plus_z.inc"
};
static NJS_MATERIAL dat_8c540b08[2] = {
#include "player/big_textures/dat_8c540b08.inc"
};
static Sint16 dat_8c540b30[7] = {
#include "player/big_textures/dat_8c540b30.inc"
};
static Sint16 dat_8c540b3e[5] = {
#include "player/big_textures/dat_8c540b3e.inc"
};
static NJS_COLOR dat_8c540b48[6] = {
#include "player/big_textures/dat_8c540b48.inc"
};
static NJS_COLOR dat_8c540b60[4] = {
#include "player/big_textures/dat_8c540b60.inc"
};
static NJS_MESHSET dat_8c540b70[2] = {
#include "player/big_textures/dat_8c540b70.inc"
};
static NJS_POINT3 dat_8c540ba0[8] = {
#include "player/big_textures/dat_8c540ba0.inc"
};
static NJS_VECTOR dat_8c540c00[8] = {
#include "player/big_textures/dat_8c540c00.inc"
};
static NJS_MODEL dat_8c540c60 = {
#include "player/big_textures/dat_8c540c60.inc"
};
static NJS_OBJECT dat_8c540c88 = {
#include "player/big_textures/dat_8c540c88.inc"
};
static NJS_MATERIAL dat_8c540cbc[2] = {
#include "player/big_textures/dat_8c540cbc.inc"
};
static Sint16 dat_8c540ce4[12] = {
#include "player/big_textures/dat_8c540ce4.inc"
};
static Sint16 dat_8c540cfc[25] = {
#include "player/big_textures/dat_8c540cfc.inc"
};
static NJS_COLOR dat_8c540d30[10] = {
#include "player/big_textures/dat_8c540d30.inc"
};
static NJS_COLOR dat_8c540d58[20] = {
#include "player/big_textures/dat_8c540d58.inc"
};
static NJS_MESHSET dat_8c540da8[2] = {
#include "player/big_textures/dat_8c540da8.inc"
};
static NJS_POINT3 dat_8c540dd8[10] = {
#include "player/big_textures/dat_8c540dd8.inc"
};
static NJS_VECTOR dat_8c540e50[10] = {
#include "player/big_textures/dat_8c540e50.inc"
};
static NJS_MODEL dat_8c540ec8 = {
#include "player/big_textures/dat_8c540ec8.inc"
};
NJS_OBJECT big_rod_reel = {
#include "player/big_textures/big_rod_reel.inc"
};
static NJS_MATERIAL dat_8c540f24[3] = {
#include "player/big_textures/dat_8c540f24.inc"
};
static Sint16 dat_8c540f60[5] = {
#include "player/big_textures/dat_8c540f60.inc"
};
static Sint16 dat_8c540f6a[126] = {
#include "player/big_textures/dat_8c540f6a.inc"
};
static Sint16 dat_8c541066[16] = {
#include "player/big_textures/dat_8c541066.inc"
};
static NJS_COLOR dat_8c541088[101] = {
#include "player/big_textures/dat_8c541088.inc"
};
static NJS_COLOR dat_8c54121c[14] = {
#include "player/big_textures/dat_8c54121c.inc"
};
static NJS_MESHSET dat_8c541254[3] = {
#include "player/big_textures/dat_8c541254.inc"
};
static NJS_POINT3 dat_8c54129c[36] = {
#include "player/big_textures/dat_8c54129c.inc"
};
static NJS_VECTOR dat_8c54144c[36] = {
#include "player/big_textures/dat_8c54144c.inc"
};
NJS_MODEL big_rod_model = {
#include "player/big_textures/big_rod_model.inc"
};
NJS_OBJECT big_rod[1] = {
#include "player/big_textures/big_rod.inc"
};
static NJS_MATERIAL dat_8c541658[1] = {
#include "player/big_textures/dat_8c541658.inc"
};
static Sint16 dat_8c54166c[35] = {
#include "player/big_textures/dat_8c54166c.inc"
};
static NJS_COLOR dat_8c5416b4[30] = {
#include "player/big_textures/dat_8c5416b4.inc"
};
static NJS_MESHSET dat_8c54172c[1] = {
#include "player/big_textures/dat_8c54172c.inc"
};
static NJS_POINT3 dat_8c541744[12] = {
#include "player/big_textures/dat_8c541744.inc"
};
static NJS_VECTOR dat_8c5417d4[12] = {
#include "player/big_textures/dat_8c5417d4.inc"
};
static NJS_MODEL dat_8c541864 = {
#include "player/big_textures/dat_8c541864.inc"
};
static NJS_OBJECT dat_8c54188c = {
#include "player/big_textures/dat_8c54188c.inc"
};
static NJS_MATERIAL dat_8c5418c0[3] = {
#include "player/big_textures/dat_8c5418c0.inc"
};
static Sint16 dat_8c5418fc[20] = {
#include "player/big_textures/dat_8c5418fc.inc"
};
static Sint16 dat_8c541924[22] = {
#include "player/big_textures/dat_8c541924.inc"
};
static Sint16 dat_8c541950[7] = {
#include "player/big_textures/dat_8c541950.inc"
};
static NJS_COLOR dat_8c541960[16] = {
#include "player/big_textures/dat_8c541960.inc"
};
static NJS_COLOR dat_8c5419a0[18] = {
#include "player/big_textures/dat_8c5419a0.inc"
};
static NJS_COLOR dat_8c5419e8[6] = {
#include "player/big_textures/dat_8c5419e8.inc"
};
static NJS_MESHSET dat_8c541a00[3] = {
#include "player/big_textures/dat_8c541a00.inc"
};
static NJS_POINT3 dat_8c541a48[14] = {
#include "player/big_textures/dat_8c541a48.inc"
};
static NJS_VECTOR dat_8c541af0[14] = {
#include "player/big_textures/dat_8c541af0.inc"
};
static NJS_MODEL dat_8c541b98 = {
#include "player/big_textures/dat_8c541b98.inc"
};
NJS_OBJECT big_fingers_plus_z = {
#include "player/big_textures/big_fingers_plus_z.inc"
};
static NJS_MATERIAL dat_8c541bf4[2] = {
#include "player/big_textures/dat_8c541bf4.inc"
};
static Sint16 dat_8c541c1c[14] = {
#include "player/big_textures/dat_8c541c1c.inc"
};
static Sint16 dat_8c541c38[5] = {
#include "player/big_textures/dat_8c541c38.inc"
};
static NJS_COLOR dat_8c541c44[12] = {
#include "player/big_textures/dat_8c541c44.inc"
};
static NJS_COLOR dat_8c541c74[4] = {
#include "player/big_textures/dat_8c541c74.inc"
};
static NJS_MESHSET dat_8c541c84[2] = {
#include "player/big_textures/dat_8c541c84.inc"
};
static NJS_POINT3 dat_8c541cb4[8] = {
#include "player/big_textures/dat_8c541cb4.inc"
};
static NJS_VECTOR dat_8c541d14[8] = {
#include "player/big_textures/dat_8c541d14.inc"
};
static NJS_MODEL dat_8c541d74 = {
#include "player/big_textures/dat_8c541d74.inc"
};
static NJS_OBJECT dat_8c541d9c = {
#include "player/big_textures/dat_8c541d9c.inc"
};
static NJS_OBJECT dat_8c541dd0 = {
#include "player/big_textures/dat_8c541dd0.inc"
};
static NJS_OBJECT dat_8c541e04 = {
#include "player/big_textures/dat_8c541e04.inc"
};
static NJS_OBJECT dat_8c541e38 = {
#include "player/big_textures/dat_8c541e38.inc"
};
static NJS_OBJECT dat_8c541e6c = {
#include "player/big_textures/dat_8c541e6c.inc"
};
static NJS_OBJECT dat_8c541ea0 = {
#include "player/big_textures/dat_8c541ea0.inc"
};
static NJS_MATERIAL dat_8c541ed4[2] = {
#include "player/big_textures/dat_8c541ed4.inc"
};
static Sint16 dat_8c541efc[12] = {
#include "player/big_textures/dat_8c541efc.inc"
};
static Sint16 dat_8c541f14[38] = {
#include "player/big_textures/dat_8c541f14.inc"
};
static NJS_COLOR dat_8c541f60[10] = {
#include "player/big_textures/dat_8c541f60.inc"
};
static NJS_COLOR dat_8c541f88[32] = {
#include "player/big_textures/dat_8c541f88.inc"
};
static NJS_MESHSET dat_8c542008[2] = {
#include "player/big_textures/dat_8c542008.inc"
};
static NJS_POINT3 dat_8c542038[17] = {
#include "player/big_textures/dat_8c542038.inc"
};
static NJS_VECTOR dat_8c542104[17] = {
#include "player/big_textures/dat_8c542104.inc"
};
static NJS_MODEL dat_8c5421d0 = {
#include "player/big_textures/dat_8c5421d0.inc"
};
NJS_OBJECT big_upper_arm_minus_z = {
#include "player/big_textures/big_upper_arm_minus_z.inc"
};
static NJS_MATERIAL dat_8c54222c[1] = {
#include "player/big_textures/dat_8c54222c.inc"
};
static Sint16 dat_8c542240[51] = {
#include "player/big_textures/dat_8c542240.inc"
};
static NJS_COLOR dat_8c5422a8[42] = {
#include "player/big_textures/dat_8c5422a8.inc"
};
static NJS_MESHSET dat_8c542350[1] = {
#include "player/big_textures/dat_8c542350.inc"
};
static NJS_POINT3 dat_8c542368[18] = {
#include "player/big_textures/dat_8c542368.inc"
};
static NJS_VECTOR dat_8c542440[18] = {
#include "player/big_textures/dat_8c542440.inc"
};
static NJS_MODEL dat_8c542518 = {
#include "player/big_textures/dat_8c542518.inc"
};
NJS_OBJECT big_forearm_minus_z = {
#include "player/big_textures/big_forearm_minus_z.inc"
};
static NJS_MATERIAL dat_8c542574[1] = {
#include "player/big_textures/dat_8c542574.inc"
};
static Sint16 dat_8c542588[38] = {
#include "player/big_textures/dat_8c542588.inc"
};
static NJS_COLOR dat_8c5425d4[32] = {
#include "player/big_textures/dat_8c5425d4.inc"
};
static NJS_MESHSET dat_8c542654[1] = {
#include "player/big_textures/dat_8c542654.inc"
};
static NJS_POINT3 dat_8c54266c[12] = {
#include "player/big_textures/dat_8c54266c.inc"
};
static NJS_VECTOR dat_8c5426fc[12] = {
#include "player/big_textures/dat_8c5426fc.inc"
};
static NJS_MODEL dat_8c54278c = {
#include "player/big_textures/dat_8c54278c.inc"
};
static NJS_OBJECT dat_8c5427b4 = {
#include "player/big_textures/dat_8c5427b4.inc"
};
static NJS_MATERIAL dat_8c5427e8[3] = {
#include "player/big_textures/dat_8c5427e8.inc"
};
static Sint16 dat_8c542824[20] = {
#include "player/big_textures/dat_8c542824.inc"
};
static Sint16 dat_8c54284c[22] = {
#include "player/big_textures/dat_8c54284c.inc"
};
static Sint16 dat_8c542878[7] = {
#include "player/big_textures/dat_8c542878.inc"
};
static NJS_COLOR dat_8c542888[16] = {
#include "player/big_textures/dat_8c542888.inc"
};
static NJS_COLOR dat_8c5428c8[18] = {
#include "player/big_textures/dat_8c5428c8.inc"
};
static NJS_COLOR dat_8c542910[6] = {
#include "player/big_textures/dat_8c542910.inc"
};
static NJS_MESHSET dat_8c542928[3] = {
#include "player/big_textures/dat_8c542928.inc"
};
static NJS_POINT3 dat_8c542970[14] = {
#include "player/big_textures/dat_8c542970.inc"
};
static NJS_VECTOR dat_8c542a18[14] = {
#include "player/big_textures/dat_8c542a18.inc"
};
static NJS_MODEL dat_8c542ac0 = {
#include "player/big_textures/dat_8c542ac0.inc"
};
NJS_OBJECT big_fingers_minus_z = {
#include "player/big_textures/big_fingers_minus_z.inc"
};
static NJS_MATERIAL dat_8c542b1c[2] = {
#include "player/big_textures/dat_8c542b1c.inc"
};
static Sint16 dat_8c542b44[14] = {
#include "player/big_textures/dat_8c542b44.inc"
};
static Sint16 dat_8c542b60[5] = {
#include "player/big_textures/dat_8c542b60.inc"
};
static NJS_COLOR dat_8c542b6c[12] = {
#include "player/big_textures/dat_8c542b6c.inc"
};
static NJS_COLOR dat_8c542b9c[4] = {
#include "player/big_textures/dat_8c542b9c.inc"
};
static NJS_MESHSET dat_8c542bac[2] = {
#include "player/big_textures/dat_8c542bac.inc"
};
static NJS_POINT3 dat_8c542bdc[8] = {
#include "player/big_textures/dat_8c542bdc.inc"
};
static NJS_VECTOR dat_8c542c3c[8] = {
#include "player/big_textures/dat_8c542c3c.inc"
};
static NJS_MODEL dat_8c542c9c = {
#include "player/big_textures/dat_8c542c9c.inc"
};
static NJS_OBJECT dat_8c542cc4 = {
#include "player/big_textures/dat_8c542cc4.inc"
};
static NJS_MATERIAL dat_8c542cf8[2] = {
#include "player/big_textures/dat_8c542cf8.inc"
};
static Sint16 dat_8c542d20[28] = {
#include "player/big_textures/dat_8c542d20.inc"
};
static Sint16 dat_8c542d58[38] = {
#include "player/big_textures/dat_8c542d58.inc"
};
static NJS_COLOR dat_8c542da4[24] = {
#include "player/big_textures/dat_8c542da4.inc"
};
static NJS_COLOR dat_8c542e04[32] = {
#include "player/big_textures/dat_8c542e04.inc"
};
static NJS_MESHSET dat_8c542e84[2] = {
#include "player/big_textures/dat_8c542e84.inc"
};
static NJS_POINT3 dat_8c542eb4[26] = {
#include "player/big_textures/dat_8c542eb4.inc"
};
static NJS_VECTOR dat_8c542fec[26] = {
#include "player/big_textures/dat_8c542fec.inc"
};
static NJS_MODEL dat_8c543124 = {
#include "player/big_textures/dat_8c543124.inc"
};
NJS_OBJECT big_hand_minus_z = {
#include "player/big_textures/big_hand_minus_z.inc"
};
static NJS_OBJECT dat_8c543180 = {
#include "player/big_textures/dat_8c543180.inc"
};
static NJS_OBJECT dat_8c5431b4 = {
#include "player/big_textures/dat_8c5431b4.inc"
};
static NJS_OBJECT dat_8c5431e8 = {
#include "player/big_textures/dat_8c5431e8.inc"
};
static NJS_OBJECT dat_8c54321c = {
#include "player/big_textures/dat_8c54321c.inc"
};
static NJS_OBJECT dat_8c543250 = {
#include "player/big_textures/dat_8c543250.inc"
};
static NJS_OBJECT dat_8c543284 = {
#include "player/big_textures/dat_8c543284.inc"
};
static NJS_OBJECT dat_8c5432b8 = {
#include "player/big_textures/dat_8c5432b8.inc"
};
static NJS_OBJECT dat_8c5432ec = {
#include "player/big_textures/dat_8c5432ec.inc"
};
static NJS_MATERIAL dat_8c543320[3] = {
#include "player/big_textures/dat_8c543320.inc"
};
static Sint16 dat_8c54335c[64] = {
#include "player/big_textures/dat_8c54335c.inc"
};
static Sint16 dat_8c5433dc[14] = {
#include "player/big_textures/dat_8c5433dc.inc"
};
static Sint16 dat_8c5433f8[14] = {
#include "player/big_textures/dat_8c5433f8.inc"
};
static NJS_COLOR dat_8c543414[56] = {
#include "player/big_textures/dat_8c543414.inc"
};
static NJS_COLOR dat_8c5434f4[12] = {
#include "player/big_textures/dat_8c5434f4.inc"
};
static NJS_COLOR dat_8c543524[12] = {
#include "player/big_textures/dat_8c543524.inc"
};
static NJS_MESHSET dat_8c543554[3] = {
#include "player/big_textures/dat_8c543554.inc"
};
static NJS_POINT3 dat_8c54359c[33] = {
#include "player/big_textures/dat_8c54359c.inc"
};
static NJS_VECTOR dat_8c543728[33] = {
#include "player/big_textures/dat_8c543728.inc"
};
static NJS_MODEL dat_8c5438b4 = {
#include "player/big_textures/dat_8c5438b4.inc"
};
static NJS_OBJECT dat_8c5438dc = {
#include "player/big_textures/dat_8c5438dc.inc"
};
static NJS_MATERIAL dat_8c543910[1] = {
#include "player/big_textures/dat_8c543910.inc"
};
static Sint16 dat_8c543924[165] = {
#include "player/big_textures/dat_8c543924.inc"
};
static NJS_COLOR dat_8c543a70[130] = {
#include "player/big_textures/dat_8c543a70.inc"
};
static NJS_MESHSET dat_8c543c78[1] = {
#include "player/big_textures/dat_8c543c78.inc"
};
static NJS_POINT3 dat_8c543c90[32] = {
#include "player/big_textures/dat_8c543c90.inc"
};
static NJS_VECTOR dat_8c543e10[32] = {
#include "player/big_textures/dat_8c543e10.inc"
};
static NJS_MODEL dat_8c543f90 = {
#include "player/big_textures/dat_8c543f90.inc"
};
static NJS_OBJECT dat_8c543fb8 = {
#include "player/big_textures/dat_8c543fb8.inc"
};
static NJS_OBJECT dat_8c543fec = {
#include "player/big_textures/dat_8c543fec.inc"
};
static NJS_OBJECT dat_8c544020 = {
#include "player/big_textures/dat_8c544020.inc"
};
static NJS_OBJECT dat_8c544054 = {
#include "player/big_textures/dat_8c544054.inc"
};
static NJS_MATERIAL dat_8c544088[1] = {
#include "player/big_textures/dat_8c544088.inc"
};
static Sint16 dat_8c54409c[42] = {
#include "player/big_textures/dat_8c54409c.inc"
};
static NJS_COLOR dat_8c5440f0[36] = {
#include "player/big_textures/dat_8c5440f0.inc"
};
static NJS_MESHSET dat_8c544180[1] = {
#include "player/big_textures/dat_8c544180.inc"
};
static NJS_POINT3 dat_8c544198[18] = {
#include "player/big_textures/dat_8c544198.inc"
};
static NJS_VECTOR dat_8c544270[18] = {
#include "player/big_textures/dat_8c544270.inc"
};
static NJS_MODEL dat_8c544348 = {
#include "player/big_textures/dat_8c544348.inc"
};
NJS_OBJECT big_thigh_plus_z = {
#include "player/big_textures/big_thigh_plus_z.inc"
};
static NJS_MATERIAL dat_8c5443a4[3] = {
#include "player/big_textures/dat_8c5443a4.inc"
};
static Sint16 dat_8c5443e0[39] = {
#include "player/big_textures/dat_8c5443e0.inc"
};
static Sint16 dat_8c54442e[5] = {
#include "player/big_textures/dat_8c54442e.inc"
};
static Sint16 dat_8c544438[7] = {
#include "player/big_textures/dat_8c544438.inc"
};
static NJS_COLOR dat_8c544448[32] = {
#include "player/big_textures/dat_8c544448.inc"
};
static NJS_COLOR dat_8c5444c8[4] = {
#include "player/big_textures/dat_8c5444c8.inc"
};
static NJS_COLOR dat_8c5444d8[6] = {
#include "player/big_textures/dat_8c5444d8.inc"
};
static NJS_MESHSET dat_8c5444f0[3] = {
#include "player/big_textures/dat_8c5444f0.inc"
};
static NJS_POINT3 dat_8c544538[16] = {
#include "player/big_textures/dat_8c544538.inc"
};
static NJS_VECTOR dat_8c5445f8[16] = {
#include "player/big_textures/dat_8c5445f8.inc"
};
static NJS_MODEL dat_8c5446b8 = {
#include "player/big_textures/dat_8c5446b8.inc"
};
NJS_OBJECT big_toes_plus_z = {
#include "player/big_textures/big_toes_plus_z.inc"
};
static NJS_MATERIAL dat_8c544714[2] = {
#include "player/big_textures/dat_8c544714.inc"
};
static Sint16 dat_8c54473c[35] = {
#include "player/big_textures/dat_8c54473c.inc"
};
static Sint16 dat_8c544782[7] = {
#include "player/big_textures/dat_8c544782.inc"
};
static NJS_COLOR dat_8c544790[30] = {
#include "player/big_textures/dat_8c544790.inc"
};
static NJS_COLOR dat_8c544808[6] = {
#include "player/big_textures/dat_8c544808.inc"
};
static NJS_MESHSET dat_8c544820[2] = {
#include "player/big_textures/dat_8c544820.inc"
};
static NJS_POINT3 dat_8c544850[16] = {
#include "player/big_textures/dat_8c544850.inc"
};
static NJS_VECTOR dat_8c544910[16] = {
#include "player/big_textures/dat_8c544910.inc"
};
static NJS_MODEL dat_8c5449d0 = {
#include "player/big_textures/dat_8c5449d0.inc"
};
NJS_OBJECT big_foot_plus_z = {
#include "player/big_textures/big_foot_plus_z.inc"
};
static NJS_MATERIAL dat_8c544a2c[1] = {
#include "player/big_textures/dat_8c544a2c.inc"
};
static Sint16 dat_8c544a40[41] = {
#include "player/big_textures/dat_8c544a40.inc"
};
static NJS_COLOR dat_8c544a94[34] = {
#include "player/big_textures/dat_8c544a94.inc"
};
static NJS_MESHSET dat_8c544b1c[1] = {
#include "player/big_textures/dat_8c544b1c.inc"
};
static NJS_POINT3 dat_8c544b34[16] = {
#include "player/big_textures/dat_8c544b34.inc"
};
static NJS_VECTOR dat_8c544bf4[16] = {
#include "player/big_textures/dat_8c544bf4.inc"
};
static NJS_MODEL dat_8c544cb4 = {
#include "player/big_textures/dat_8c544cb4.inc"
};
NJS_OBJECT big_shin_plus_z = {
#include "player/big_textures/big_shin_plus_z.inc"
};
static NJS_OBJECT dat_8c544d10 = {
#include "player/big_textures/dat_8c544d10.inc"
};
static NJS_OBJECT dat_8c544d44 = {
#include "player/big_textures/dat_8c544d44.inc"
};
static NJS_OBJECT dat_8c544d78 = {
#include "player/big_textures/dat_8c544d78.inc"
};
static NJS_OBJECT dat_8c544dac = {
#include "player/big_textures/dat_8c544dac.inc"
};
static NJS_MATERIAL dat_8c544de0[1] = {
#include "player/big_textures/dat_8c544de0.inc"
};
static Sint16 dat_8c544df4[42] = {
#include "player/big_textures/dat_8c544df4.inc"
};
static NJS_COLOR dat_8c544e48[36] = {
#include "player/big_textures/dat_8c544e48.inc"
};
static NJS_MESHSET dat_8c544ed8[1] = {
#include "player/big_textures/dat_8c544ed8.inc"
};
static NJS_POINT3 dat_8c544ef0[18] = {
#include "player/big_textures/dat_8c544ef0.inc"
};
static NJS_VECTOR dat_8c544fc8[18] = {
#include "player/big_textures/dat_8c544fc8.inc"
};
static NJS_MODEL dat_8c5450a0 = {
#include "player/big_textures/dat_8c5450a0.inc"
};
NJS_OBJECT big_thigh_minus_z = {
#include "player/big_textures/big_thigh_minus_z.inc"
};
static NJS_MATERIAL dat_8c5450fc[1] = {
#include "player/big_textures/dat_8c5450fc.inc"
};
static Sint16 dat_8c545110[41] = {
#include "player/big_textures/dat_8c545110.inc"
};
static NJS_COLOR dat_8c545164[34] = {
#include "player/big_textures/dat_8c545164.inc"
};
static NJS_MESHSET dat_8c5451ec[1] = {
#include "player/big_textures/dat_8c5451ec.inc"
};
static NJS_POINT3 dat_8c545204[16] = {
#include "player/big_textures/dat_8c545204.inc"
};
static NJS_VECTOR dat_8c5452c4[16] = {
#include "player/big_textures/dat_8c5452c4.inc"
};
static NJS_MODEL dat_8c545384 = {
#include "player/big_textures/dat_8c545384.inc"
};
NJS_OBJECT big_shin_minus_z = {
#include "player/big_textures/big_shin_minus_z.inc"
};
static NJS_MATERIAL dat_8c5453e0[3] = {
#include "player/big_textures/dat_8c5453e0.inc"
};
static Sint16 dat_8c54541c[39] = {
#include "player/big_textures/dat_8c54541c.inc"
};
static Sint16 dat_8c54546a[5] = {
#include "player/big_textures/dat_8c54546a.inc"
};
static Sint16 dat_8c545474[7] = {
#include "player/big_textures/dat_8c545474.inc"
};
static NJS_COLOR dat_8c545484[32] = {
#include "player/big_textures/dat_8c545484.inc"
};
static NJS_COLOR dat_8c545504[4] = {
#include "player/big_textures/dat_8c545504.inc"
};
static NJS_COLOR dat_8c545514[6] = {
#include "player/big_textures/dat_8c545514.inc"
};
static NJS_MESHSET dat_8c54552c[3] = {
#include "player/big_textures/dat_8c54552c.inc"
};
static NJS_POINT3 dat_8c545574[16] = {
#include "player/big_textures/dat_8c545574.inc"
};
static NJS_VECTOR dat_8c545634[16] = {
#include "player/big_textures/dat_8c545634.inc"
};
static NJS_MODEL dat_8c5456f4 = {
#include "player/big_textures/dat_8c5456f4.inc"
};
NJS_OBJECT big_toes_minus_z = {
#include "player/big_textures/big_toes_minus_z.inc"
};
static NJS_MATERIAL dat_8c545750[2] = {
#include "player/big_textures/dat_8c545750.inc"
};
static Sint16 dat_8c545778[35] = {
#include "player/big_textures/dat_8c545778.inc"
};
static Sint16 dat_8c5457be[7] = {
#include "player/big_textures/dat_8c5457be.inc"
};
static NJS_COLOR dat_8c5457cc[30] = {
#include "player/big_textures/dat_8c5457cc.inc"
};
static NJS_COLOR dat_8c545844[6] = {
#include "player/big_textures/dat_8c545844.inc"
};
static NJS_MESHSET dat_8c54585c[2] = {
#include "player/big_textures/dat_8c54585c.inc"
};
static NJS_POINT3 dat_8c54588c[16] = {
#include "player/big_textures/dat_8c54588c.inc"
};
static NJS_VECTOR dat_8c54594c[16] = {
#include "player/big_textures/dat_8c54594c.inc"
};
static NJS_MODEL dat_8c545a0c = {
#include "player/big_textures/dat_8c545a0c.inc"
};
NJS_OBJECT big_foot_minus_z = {
#include "player/big_textures/big_foot_minus_z.inc"
};
static NJS_OBJECT dat_8c545a68 = {
#include "player/big_textures/dat_8c545a68.inc"
};
static NJS_OBJECT dat_8c545a9c = {
#include "player/big_textures/dat_8c545a9c.inc"
};
static NJS_OBJECT dat_8c545ad0 = {
#include "player/big_textures/dat_8c545ad0.inc"
};
static NJS_OBJECT dat_8c545b04 = {
#include "player/big_textures/dat_8c545b04.inc"
};
static NJS_OBJECT dat_8c545b38 = {
#include "player/big_textures/dat_8c545b38.inc"
};
static NJS_OBJECT dat_8c545b6c = {
#include "player/big_textures/dat_8c545b6c.inc"
};
NJS_OBJECT big_object = {
#include "player/big_textures/big_object.inc"
};
static NJS_MATERIAL dat_8c545bd4[7] = {
#include "player/big_textures/dat_8c545bd4.inc"
};
static Sint16 dat_8c545c60[199] = {
#include "player/big_textures/dat_8c545c60.inc"
};
static Sint16 dat_8c545dee[127] = {
#include "player/big_textures/dat_8c545dee.inc"
};
static Sint16 dat_8c545eec[49] = {
#include "player/big_textures/dat_8c545eec.inc"
};
static Sint16 dat_8c545f4e[42] = {
#include "player/big_textures/dat_8c545f4e.inc"
};
static Sint16 dat_8c545fa2[32] = {
#include "player/big_textures/dat_8c545fa2.inc"
};
static Sint16 dat_8c545fe2[10] = {
#include "player/big_textures/dat_8c545fe2.inc"
};
static Sint16 dat_8c545ff6[24] = {
#include "player/big_textures/dat_8c545ff6.inc"
};
static NJS_COLOR dat_8c546028[169] = {
#include "player/big_textures/dat_8c546028.inc"
};
static NJS_COLOR dat_8c5462cc[112] = {
#include "player/big_textures/dat_8c5462cc.inc"
};
static NJS_COLOR dat_8c54648c[43] = {
#include "player/big_textures/dat_8c54648c.inc"
};
static NJS_COLOR dat_8c546538[36] = {
#include "player/big_textures/dat_8c546538.inc"
};
static NJS_COLOR dat_8c5465c8[28] = {
#include "player/big_textures/dat_8c5465c8.inc"
};
static NJS_COLOR dat_8c546638[8] = {
#include "player/big_textures/dat_8c546638.inc"
};
static NJS_COLOR dat_8c546658[20] = {
#include "player/big_textures/dat_8c546658.inc"
};
static NJS_MESHSET dat_8c5466a8[7] = {
#include "player/big_textures/dat_8c5466a8.inc"
};
static NJS_POINT3 dat_8c546750[143] = {
#include "player/big_textures/dat_8c546750.inc"
};
static NJS_VECTOR dat_8c546e04[143] = {
#include "player/big_textures/dat_8c546e04.inc"
};
NJS_MODEL big_head_sway_model = {
#include "player/big_textures/big_head_sway_model.inc"
};
NJS_POINT3 big_belt_piece_flat[6] = {
#include "player/big_textures/big_belt_piece_flat.inc"
};
NJS_POINT3 big_belt_torso_flat[78] = {
#include "player/big_textures/big_belt_torso_flat.inc"
};
NJS_POINT3 big_belt_piece_full[6] = {
#include "player/big_textures/big_belt_piece_full.inc"
};
NJS_POINT3 big_belt_torso_full[78] = {
#include "player/big_textures/big_belt_torso_full.inc"
};
static NJS_OBJECT dat_8c547cc0 = {
#include "player/big_textures/dat_8c547cc0.inc"
};
static NJS_MATERIAL dat_8c547cf4[1] = {
#include "player/big_textures/dat_8c547cf4.inc"
};
static Sint16 dat_8c547d08[7] = {
#include "player/big_textures/dat_8c547d08.inc"
};
static NJS_COLOR dat_8c547d18[6] = {
#include "player/big_textures/dat_8c547d18.inc"
};
static NJS_MESHSET dat_8c547d30[1] = {
#include "player/big_textures/dat_8c547d30.inc"
};
NJS_POINT3 big_belt_piece_points[6] = {
#include "player/big_textures/big_belt_piece_points.inc"
};
static NJS_VECTOR dat_8c547d90[6] = {
#include "player/big_textures/dat_8c547d90.inc"
};
static NJS_MODEL dat_8c547dd8 = {
#include "player/big_textures/dat_8c547dd8.inc"
};
static NJS_OBJECT dat_8c547e00 = {
#include "player/big_textures/dat_8c547e00.inc"
};
static NJS_MATERIAL dat_8c547e34[4] = {
#include "player/big_textures/dat_8c547e34.inc"
};
static Sint16 dat_8c547e84[111] = {
#include "player/big_textures/dat_8c547e84.inc"
};
static Sint16 dat_8c547f62[64] = {
#include "player/big_textures/dat_8c547f62.inc"
};
static Sint16 dat_8c547fe2[31] = {
#include "player/big_textures/dat_8c547fe2.inc"
};
static Sint16 dat_8c548020[73] = {
#include "player/big_textures/dat_8c548020.inc"
};
static NJS_COLOR dat_8c5480b4[90] = {
#include "player/big_textures/dat_8c5480b4.inc"
};
static NJS_COLOR dat_8c54821c[54] = {
#include "player/big_textures/dat_8c54821c.inc"
};
static NJS_COLOR dat_8c5482f4[26] = {
#include "player/big_textures/dat_8c5482f4.inc"
};
static NJS_COLOR dat_8c54835c[64] = {
#include "player/big_textures/dat_8c54835c.inc"
};
static NJS_MESHSET dat_8c54845c[4] = {
#include "player/big_textures/dat_8c54845c.inc"
};
NJS_POINT3 big_belt_torso_points[78] = {
#include "player/big_textures/big_belt_torso_points.inc"
};
static NJS_VECTOR dat_8c548864[78] = {
#include "player/big_textures/dat_8c548864.inc"
};
static NJS_MODEL dat_8c548c0c = {
#include "player/big_textures/dat_8c548c0c.inc"
};
NJS_OBJECT big_belt_torso[1] = {
#include "player/big_textures/big_belt_torso.inc"
};
static NJS_MATERIAL dat_8c548c68[2] = {
#include "player/big_textures/dat_8c548c68.inc"
};
static Sint16 dat_8c548c90[7] = {
#include "player/big_textures/dat_8c548c90.inc"
};
static Sint16 dat_8c548c9e[5] = {
#include "player/big_textures/dat_8c548c9e.inc"
};
static NJS_COLOR dat_8c548ca8[6] = {
#include "player/big_textures/dat_8c548ca8.inc"
};
static NJS_COLOR dat_8c548cc0[4] = {
#include "player/big_textures/dat_8c548cc0.inc"
};
static NJS_MESHSET dat_8c548cd0[2] = {
#include "player/big_textures/dat_8c548cd0.inc"
};
static NJS_POINT3 dat_8c548d00[8] = {
#include "player/big_textures/dat_8c548d00.inc"
};
static NJS_VECTOR dat_8c548d60[8] = {
#include "player/big_textures/dat_8c548d60.inc"
};
static NJS_MODEL dat_8c548dc0 = {
#include "player/big_textures/dat_8c548dc0.inc"
};
static NJS_OBJECT dat_8c548de8 = {
#include "player/big_textures/dat_8c548de8.inc"
};
static NJS_MATERIAL dat_8c548e1c[3] = {
#include "player/big_textures/dat_8c548e1c.inc"
};
static Sint16 dat_8c548e58[20] = {
#include "player/big_textures/dat_8c548e58.inc"
};
static Sint16 dat_8c548e80[11] = {
#include "player/big_textures/dat_8c548e80.inc"
};
static Sint16 dat_8c548e96[30] = {
#include "player/big_textures/dat_8c548e96.inc"
};
static NJS_COLOR dat_8c548ed4[16] = {
#include "player/big_textures/dat_8c548ed4.inc"
};
static NJS_COLOR dat_8c548f14[9] = {
#include "player/big_textures/dat_8c548f14.inc"
};
static NJS_COLOR dat_8c548f38[25] = {
#include "player/big_textures/dat_8c548f38.inc"
};
static NJS_MESHSET dat_8c548f9c[3] = {
#include "player/big_textures/dat_8c548f9c.inc"
};
static NJS_POINT3 dat_8c548fe4[19] = {
#include "player/big_textures/dat_8c548fe4.inc"
};
static NJS_VECTOR dat_8c5490c8[19] = {
#include "player/big_textures/dat_8c5490c8.inc"
};
static NJS_MODEL dat_8c5491ac = {
#include "player/big_textures/dat_8c5491ac.inc"
};
NJS_OBJECT big_power_rod_reel = {
#include "player/big_textures/big_power_rod_reel.inc"
};
static NJS_MATERIAL dat_8c549208[2] = {
#include "player/big_textures/dat_8c549208.inc"
};
static Sint16 dat_8c549230[21] = {
#include "player/big_textures/dat_8c549230.inc"
};
static Sint16 dat_8c54925a[132] = {
#include "player/big_textures/dat_8c54925a.inc"
};
static NJS_COLOR dat_8c549364[18] = {
#include "player/big_textures/dat_8c549364.inc"
};
static NJS_COLOR dat_8c5493ac[105] = {
#include "player/big_textures/dat_8c5493ac.inc"
};
static NJS_MESHSET dat_8c549550[2] = {
#include "player/big_textures/dat_8c549550.inc"
};
static NJS_POINT3 dat_8c549580[36] = {
#include "player/big_textures/dat_8c549580.inc"
};
static NJS_VECTOR dat_8c549730[36] = {
#include "player/big_textures/dat_8c549730.inc"
};
NJS_MODEL big_power_rod_model = {
#include "player/big_textures/big_power_rod_model.inc"
};
NJS_OBJECT big_power_rod_object = {
#include "player/big_textures/big_power_rod_object.inc"
};
static NJS_MATERIAL dat_8c54993c[1] = {
#include "player/big_textures/dat_8c54993c.inc"
};
static Sint16 dat_8c549950[20] = {
#include "player/big_textures/dat_8c549950.inc"
};
static NJS_COLOR dat_8c549978[16] = {
#include "player/big_textures/dat_8c549978.inc"
};
static NJS_MESHSET dat_8c5499b8[1] = {
#include "player/big_textures/dat_8c5499b8.inc"
};
static NJS_POINT3 dat_8c5499d0[10] = {
#include "player/big_textures/dat_8c5499d0.inc"
};
static NJS_VECTOR dat_8c549a48[10] = {
#include "player/big_textures/dat_8c549a48.inc"
};
static NJS_MODEL dat_8c549ac0 = {
#include "player/big_textures/dat_8c549ac0.inc"
};
static NJS_OBJECT dat_8c549ae8 = {
#include "player/big_textures/dat_8c549ae8.inc"
};
static NJS_MATERIAL dat_8c549b1c[1] = {
#include "player/big_textures/dat_8c549b1c.inc"
};
static Sint16 dat_8c549b30[20] = {
#include "player/big_textures/dat_8c549b30.inc"
};
static NJS_COLOR dat_8c549b58[16] = {
#include "player/big_textures/dat_8c549b58.inc"
};
static NJS_MESHSET dat_8c549b98[1] = {
#include "player/big_textures/dat_8c549b98.inc"
};
static NJS_POINT3 dat_8c549bb0[10] = {
#include "player/big_textures/dat_8c549bb0.inc"
};
static NJS_VECTOR dat_8c549c28[10] = {
#include "player/big_textures/dat_8c549c28.inc"
};
static NJS_MODEL dat_8c549ca0 = {
#include "player/big_textures/dat_8c549ca0.inc"
};
static NJS_OBJECT dat_8c549cc8 = {
#include "player/big_textures/dat_8c549cc8.inc"
};
static NJS_MATERIAL dat_8c549cfc[1] = {
#include "player/big_textures/dat_8c549cfc.inc"
};
static Sint16 dat_8c549d10[15] = {
#include "player/big_textures/dat_8c549d10.inc"
};
static NJS_COLOR dat_8c549d30[12] = {
#include "player/big_textures/dat_8c549d30.inc"
};
static NJS_MESHSET dat_8c549d60[1] = {
#include "player/big_textures/dat_8c549d60.inc"
};
static NJS_POINT3 dat_8c549d78[10] = {
#include "player/big_textures/dat_8c549d78.inc"
};
static NJS_VECTOR dat_8c549df0[10] = {
#include "player/big_textures/dat_8c549df0.inc"
};
static NJS_MODEL dat_8c549e68 = {
#include "player/big_textures/dat_8c549e68.inc"
};
static NJS_OBJECT dat_8c549e90 = {
#include "player/big_textures/dat_8c549e90.inc"
};
static NJS_MATERIAL dat_8c549ec4[1] = {
#include "player/big_textures/dat_8c549ec4.inc"
};
static Sint16 dat_8c549ed8[10] = {
#include "player/big_textures/dat_8c549ed8.inc"
};
static NJS_COLOR dat_8c549eec[8] = {
#include "player/big_textures/dat_8c549eec.inc"
};
static NJS_MESHSET dat_8c549f0c[1] = {
#include "player/big_textures/dat_8c549f0c.inc"
};
static NJS_POINT3 dat_8c549f24[6] = {
#include "player/big_textures/dat_8c549f24.inc"
};
static NJS_VECTOR dat_8c549f6c[6] = {
#include "player/big_textures/dat_8c549f6c.inc"
};
static NJS_MODEL dat_8c549fb4 = {
#include "player/big_textures/dat_8c549fb4.inc"
};
static NJS_OBJECT dat_8c549fdc = {
#include "player/big_textures/dat_8c549fdc.inc"
};
static NJS_MATERIAL dat_8c54a010[1] = {
#include "player/big_textures/dat_8c54a010.inc"
};
static Sint16 dat_8c54a024[10] = {
#include "player/big_textures/dat_8c54a024.inc"
};
static NJS_COLOR dat_8c54a038[8] = {
#include "player/big_textures/dat_8c54a038.inc"
};
static NJS_MESHSET dat_8c54a058[1] = {
#include "player/big_textures/dat_8c54a058.inc"
};
static NJS_POINT3 dat_8c54a070[6] = {
#include "player/big_textures/dat_8c54a070.inc"
};
static NJS_VECTOR dat_8c54a0b8[6] = {
#include "player/big_textures/dat_8c54a0b8.inc"
};
static NJS_MODEL dat_8c54a100 = {
#include "player/big_textures/dat_8c54a100.inc"
};
static NJS_OBJECT dat_8c54a128 = {
#include "player/big_textures/dat_8c54a128.inc"
};
static NJS_MATERIAL dat_8c54a15c[1] = {
#include "player/big_textures/dat_8c54a15c.inc"
};
static Sint16 dat_8c54a170[15] = {
#include "player/big_textures/dat_8c54a170.inc"
};
static NJS_COLOR dat_8c54a190[12] = {
#include "player/big_textures/dat_8c54a190.inc"
};
static NJS_MESHSET dat_8c54a1c0[1] = {
#include "player/big_textures/dat_8c54a1c0.inc"
};
static NJS_POINT3 dat_8c54a1d8[10] = {
#include "player/big_textures/dat_8c54a1d8.inc"
};
static NJS_VECTOR dat_8c54a250[10] = {
#include "player/big_textures/dat_8c54a250.inc"
};
static NJS_MODEL dat_8c54a2c8 = {
#include "player/big_textures/dat_8c54a2c8.inc"
};
static NJS_OBJECT dat_8c54a2f0 = {
#include "player/big_textures/dat_8c54a2f0.inc"
};
static NJS_MATERIAL dat_8c54a324[11] = {
#include "player/big_textures/dat_8c54a324.inc"
};
static Sint16 dat_8c54a400[13] = {
#include "player/big_textures/dat_8c54a400.inc"
};
static Sint16 dat_8c54a41a[197] = {
#include "player/big_textures/dat_8c54a41a.inc"
};
static Sint16 dat_8c54a5a4[205] = {
#include "player/big_textures/dat_8c54a5a4.inc"
};
static Sint16 dat_8c54a73e[10] = {
#include "player/big_textures/dat_8c54a73e.inc"
};
static Sint16 dat_8c54a752[49] = {
#include "player/big_textures/dat_8c54a752.inc"
};
static Sint16 dat_8c54a7b4[134] = {
#include "player/big_textures/dat_8c54a7b4.inc"
};
static Sint16 dat_8c54a8c0[42] = {
#include "player/big_textures/dat_8c54a8c0.inc"
};
static Sint16 dat_8c54a914[24] = {
#include "player/big_textures/dat_8c54a914.inc"
};
static Sint16 dat_8c54a944[22] = {
#include "player/big_textures/dat_8c54a944.inc"
};
static Sint16 dat_8c54a970[14] = {
#include "player/big_textures/dat_8c54a970.inc"
};
static Sint16 dat_8c54a98c[40] = {
#include "player/big_textures/dat_8c54a98c.inc"
};
static NJS_COLOR dat_8c54a9dc[11] = {
#include "player/big_textures/dat_8c54a9dc.inc"
};
static NJS_COLOR dat_8c54aa08[172] = {
#include "player/big_textures/dat_8c54aa08.inc"
};
static NJS_COLOR dat_8c54acb8[173] = {
#include "player/big_textures/dat_8c54acb8.inc"
};
static NJS_COLOR dat_8c54af6c[8] = {
#include "player/big_textures/dat_8c54af6c.inc"
};
static NJS_COLOR dat_8c54af8c[43] = {
#include "player/big_textures/dat_8c54af8c.inc"
};
static NJS_COLOR dat_8c54b038[120] = {
#include "player/big_textures/dat_8c54b038.inc"
};
static NJS_COLOR dat_8c54b218[36] = {
#include "player/big_textures/dat_8c54b218.inc"
};
static NJS_COLOR dat_8c54b2a8[20] = {
#include "player/big_textures/dat_8c54b2a8.inc"
};
static NJS_COLOR dat_8c54b2f8[20] = {
#include "player/big_textures/dat_8c54b2f8.inc"
};
static NJS_COLOR dat_8c54b348[12] = {
#include "player/big_textures/dat_8c54b348.inc"
};
static NJS_COLOR dat_8c54b378[37] = {
#include "player/big_textures/dat_8c54b378.inc"
};
static NJS_MESHSET dat_8c54b40c[11] = {
#include "player/big_textures/dat_8c54b40c.inc"
};
static NJS_POINT3 dat_8c54b514[294] = {
#include "player/big_textures/dat_8c54b514.inc"
};
static NJS_VECTOR dat_8c54c2dc[294] = {
#include "player/big_textures/dat_8c54c2dc.inc"
};
static NJS_MODEL dat_8c54d0a4 = {
#include "player/big_textures/dat_8c54d0a4.inc"
};
NJS_OBJECT big_head_blend = {
#include "player/big_textures/big_head_blend.inc"
};
/* the mouth shapes, then their keys and big_mouth_shapes, the motion that
 * names them */
static NJS_POINT3 dat_8c54d100[294] = {
#include "player/big_textures/dat_8c54d100.inc"
};
static NJS_VECTOR dat_8c54dec8[1] = {
#include "player/big_textures/dat_8c54dec8.inc"
};
static NJS_POINT3 dat_8c54ded4[294] = {
#include "player/big_textures/dat_8c54ded4.inc"
};
static NJS_VECTOR dat_8c54ec9c[1] = {
#include "player/big_textures/dat_8c54ec9c.inc"
};
static NJS_POINT3 dat_8c54eca8[294] = {
#include "player/big_textures/dat_8c54eca8.inc"
};
static NJS_VECTOR dat_8c54fa70[1] = {
#include "player/big_textures/dat_8c54fa70.inc"
};
static NJS_POINT3 dat_8c54fa7c[294] = {
#include "player/big_textures/dat_8c54fa7c.inc"
};
static NJS_VECTOR dat_8c550844[1] = {
#include "player/big_textures/dat_8c550844.inc"
};
static NJS_POINT3 dat_8c550850[294] = {
#include "player/big_textures/dat_8c550850.inc"
};
static NJS_VECTOR dat_8c551618[1] = {
#include "player/big_textures/dat_8c551618.inc"
};
static NJS_POINT3 dat_8c551624[294] = {
#include "player/big_textures/dat_8c551624.inc"
};
static NJS_VECTOR dat_8c5523ec[1] = {
#include "player/big_textures/dat_8c5523ec.inc"
};
static NJS_MKEY_P dat_8c5523f8[8] = {
#include "player/big_textures/dat_8c5523f8.inc"
};
static NJS_MKEY_P dat_8c552438[8] = {
#include "player/big_textures/dat_8c552438.inc"
};
static NJS_POINT3 dat_8c552478[10] = {
#include "player/big_textures/dat_8c552478.inc"
};
static NJS_VECTOR dat_8c5524f0[1] = {
#include "player/big_textures/dat_8c5524f0.inc"
};
static NJS_POINT3 dat_8c5524fc[10] = {
#include "player/big_textures/dat_8c5524fc.inc"
};
static NJS_VECTOR dat_8c552574[1] = {
#include "player/big_textures/dat_8c552574.inc"
};
static NJS_POINT3 dat_8c552580[10] = {
#include "player/big_textures/dat_8c552580.inc"
};
static NJS_VECTOR dat_8c5525f8[1] = {
#include "player/big_textures/dat_8c5525f8.inc"
};
static NJS_POINT3 dat_8c552604[10] = {
#include "player/big_textures/dat_8c552604.inc"
};
static NJS_VECTOR dat_8c55267c[1] = {
#include "player/big_textures/dat_8c55267c.inc"
};
static NJS_POINT3 dat_8c552688[10] = {
#include "player/big_textures/dat_8c552688.inc"
};
static NJS_VECTOR dat_8c552700[1] = {
#include "player/big_textures/dat_8c552700.inc"
};
static NJS_POINT3 dat_8c55270c[10] = {
#include "player/big_textures/dat_8c55270c.inc"
};
static NJS_VECTOR dat_8c552784[1] = {
#include "player/big_textures/dat_8c552784.inc"
};
static NJS_MKEY_P dat_8c552790[8] = {
#include "player/big_textures/dat_8c552790.inc"
};
static NJS_MKEY_P dat_8c5527d0[8] = {
#include "player/big_textures/dat_8c5527d0.inc"
};
static NJS_POINT3 dat_8c552810[10] = {
#include "player/big_textures/dat_8c552810.inc"
};
static NJS_VECTOR dat_8c552888[1] = {
#include "player/big_textures/dat_8c552888.inc"
};
static NJS_POINT3 dat_8c552894[10] = {
#include "player/big_textures/dat_8c552894.inc"
};
static NJS_VECTOR dat_8c55290c[1] = {
#include "player/big_textures/dat_8c55290c.inc"
};
static NJS_POINT3 dat_8c552918[10] = {
#include "player/big_textures/dat_8c552918.inc"
};
static NJS_VECTOR dat_8c552990[1] = {
#include "player/big_textures/dat_8c552990.inc"
};
static NJS_POINT3 dat_8c55299c[10] = {
#include "player/big_textures/dat_8c55299c.inc"
};
static NJS_VECTOR dat_8c552a14[1] = {
#include "player/big_textures/dat_8c552a14.inc"
};
static NJS_POINT3 dat_8c552a20[10] = {
#include "player/big_textures/dat_8c552a20.inc"
};
static NJS_VECTOR dat_8c552a98[1] = {
#include "player/big_textures/dat_8c552a98.inc"
};
static NJS_POINT3 dat_8c552aa4[10] = {
#include "player/big_textures/dat_8c552aa4.inc"
};
static NJS_VECTOR dat_8c552b1c[1] = {
#include "player/big_textures/dat_8c552b1c.inc"
};
static NJS_MKEY_P dat_8c552b28[8] = {
#include "player/big_textures/dat_8c552b28.inc"
};
static NJS_MKEY_P dat_8c552b68[8] = {
#include "player/big_textures/dat_8c552b68.inc"
};
static NJS_MDATA2 dat_8c552ba8[7] = {
#include "player/big_textures/dat_8c552ba8.inc"
};
/* three nodes of big_head_blend take eight vertex keys and eight normal
 * keys each. a vertex key names a shape: an array with one point for each
 * point of its node's model. player_mouth_blend sets the node's points to a
 * blend of two shapes. a normal key names one zero vector, which nothing
 * reads */
NJS_MOTION big_mouth_shapes = {
#include "player/big_textures/big_mouth_shapes.inc"
};
static NJS_MKEY_A dat_8c552c24[16] = {
#include "player/big_textures/dat_8c552c24.inc"
};
static NJS_MKEY_A dat_8c552d24[2] = {
#include "player/big_textures/dat_8c552d24.inc"
};
static NJS_MKEY_A dat_8c552d44[16] = {
#include "player/big_textures/dat_8c552d44.inc"
};
static NJS_MKEY_A dat_8c552e44[16] = {
#include "player/big_textures/dat_8c552e44.inc"
};
static NJS_MKEY_A dat_8c552f44[16] = {
#include "player/big_textures/dat_8c552f44.inc"
};
static NJS_MKEY_A dat_8c553044[16] = {
#include "player/big_textures/dat_8c553044.inc"
};
static NJS_MKEY_A dat_8c553144[2] = {
#include "player/big_textures/dat_8c553144.inc"
};
static NJS_MKEY_A dat_8c553164[16] = {
#include "player/big_textures/dat_8c553164.inc"
};
static NJS_MKEY_A dat_8c553264[16] = {
#include "player/big_textures/dat_8c553264.inc"
};
static NJS_MKEY_A dat_8c553364[16] = {
#include "player/big_textures/dat_8c553364.inc"
};
static NJS_MKEY_A dat_8c553464[16] = {
#include "player/big_textures/dat_8c553464.inc"
};
static NJS_MKEY_A dat_8c553564[16] = {
#include "player/big_textures/dat_8c553564.inc"
};
static NJS_MKEY_A dat_8c553664[2] = {
#include "player/big_textures/dat_8c553664.inc"
};
static NJS_MKEY_A dat_8c553684[16] = {
#include "player/big_textures/dat_8c553684.inc"
};
static NJS_MKEY_A dat_8c553784[16] = {
#include "player/big_textures/dat_8c553784.inc"
};
static NJS_MKEY_A dat_8c553884[16] = {
#include "player/big_textures/dat_8c553884.inc"
};
static NJS_MKEY_A dat_8c553984[16] = {
#include "player/big_textures/dat_8c553984.inc"
};
static NJS_MKEY_A dat_8c553a84[16] = {
#include "player/big_textures/dat_8c553a84.inc"
};
static NJS_MKEY_A dat_8c553b84[16] = {
#include "player/big_textures/dat_8c553b84.inc"
};
static NJS_MKEY_A dat_8c553c84[16] = {
#include "player/big_textures/dat_8c553c84.inc"
};
static NJS_MKEY_A dat_8c553d84[16] = {
#include "player/big_textures/dat_8c553d84.inc"
};
static NJS_MKEY_A dat_8c553e84[16] = {
#include "player/big_textures/dat_8c553e84.inc"
};
static NJS_MKEY_A dat_8c553f84[16] = {
#include "player/big_textures/dat_8c553f84.inc"
};
static NJS_MKEY_A dat_8c554084[2] = {
#include "player/big_textures/dat_8c554084.inc"
};
static NJS_MKEY_A dat_8c5540a4[2] = {
#include "player/big_textures/dat_8c5540a4.inc"
};
static NJS_MDATA2 dat_8c5540c4[73] = {
#include "player/big_textures/dat_8c5540c4.inc"
};
static NJS_MOTION dat_8c554554 = {
#include "player/big_textures/dat_8c554554.inc"
};
NJS_ACTION big_jump_action = {
#include "player/big_textures/big_jump_action.inc"
};
static NJS_MKEY_A dat_8c554568[14] = {
#include "player/big_textures/dat_8c554568.inc"
};
static NJS_MKEY_A dat_8c554648[2] = {
#include "player/big_textures/dat_8c554648.inc"
};
static NJS_MKEY_A dat_8c554668[16] = {
#include "player/big_textures/dat_8c554668.inc"
};
static NJS_MKEY_A dat_8c554768[16] = {
#include "player/big_textures/dat_8c554768.inc"
};
static NJS_MKEY_A dat_8c554868[16] = {
#include "player/big_textures/dat_8c554868.inc"
};
static NJS_MKEY_A dat_8c554968[2] = {
#include "player/big_textures/dat_8c554968.inc"
};
static NJS_MKEY_A dat_8c554988[2] = {
#include "player/big_textures/dat_8c554988.inc"
};
static NJS_MKEY_A dat_8c5549a8[16] = {
#include "player/big_textures/dat_8c5549a8.inc"
};
static NJS_MKEY_A dat_8c554aa8[16] = {
#include "player/big_textures/dat_8c554aa8.inc"
};
static NJS_MKEY_A dat_8c554ba8[16] = {
#include "player/big_textures/dat_8c554ba8.inc"
};
static NJS_MKEY_A dat_8c554ca8[2] = {
#include "player/big_textures/dat_8c554ca8.inc"
};
static NJS_MKEY_A dat_8c554cc8[16] = {
#include "player/big_textures/dat_8c554cc8.inc"
};
static NJS_MKEY_A dat_8c554dc8[2] = {
#include "player/big_textures/dat_8c554dc8.inc"
};
static NJS_MKEY_A dat_8c554de8[16] = {
#include "player/big_textures/dat_8c554de8.inc"
};
static NJS_MKEY_A dat_8c554ee8[16] = {
#include "player/big_textures/dat_8c554ee8.inc"
};
static NJS_MKEY_A dat_8c554fe8[16] = {
#include "player/big_textures/dat_8c554fe8.inc"
};
static NJS_MKEY_A dat_8c5550e8[16] = {
#include "player/big_textures/dat_8c5550e8.inc"
};
static NJS_MKEY_A dat_8c5551e8[16] = {
#include "player/big_textures/dat_8c5551e8.inc"
};
static NJS_MKEY_A dat_8c5552e8[2] = {
#include "player/big_textures/dat_8c5552e8.inc"
};
static NJS_MKEY_F dat_8c555308[2] = {
#include "player/big_textures/dat_8c555308.inc"
};
static NJS_MKEY_A dat_8c555328[2] = {
#include "player/big_textures/dat_8c555328.inc"
};
static NJS_MKEY_A dat_8c555348[16] = {
#include "player/big_textures/dat_8c555348.inc"
};
static NJS_MKEY_A dat_8c555448[16] = {
#include "player/big_textures/dat_8c555448.inc"
};
static NJS_MKEY_A dat_8c555548[16] = {
#include "player/big_textures/dat_8c555548.inc"
};
static NJS_MKEY_A dat_8c555648[16] = {
#include "player/big_textures/dat_8c555648.inc"
};
static NJS_MKEY_F dat_8c555748[2] = {
#include "player/big_textures/dat_8c555748.inc"
};
static NJS_MKEY_F dat_8c555768[16] = {
#include "player/big_textures/dat_8c555768.inc"
};
static NJS_MKEY_F dat_8c555868[2] = {
#include "player/big_textures/dat_8c555868.inc"
};
static NJS_MKEY_F dat_8c555888[2] = {
#include "player/big_textures/dat_8c555888.inc"
};
static NJS_MKEY_F dat_8c5558a8[16] = {
#include "player/big_textures/dat_8c5558a8.inc"
};
static NJS_MKEY_F dat_8c5559a8[16] = {
#include "player/big_textures/dat_8c5559a8.inc"
};
static NJS_MKEY_F dat_8c555aa8[16] = {
#include "player/big_textures/dat_8c555aa8.inc"
};
static NJS_MKEY_A dat_8c555ba8[16] = {
#include "player/big_textures/dat_8c555ba8.inc"
};
static NJS_MKEY_F dat_8c555ca8[16] = {
#include "player/big_textures/dat_8c555ca8.inc"
};
static NJS_MKEY_F dat_8c555da8[16] = {
#include "player/big_textures/dat_8c555da8.inc"
};
static NJS_MKEY_F dat_8c555ea8[2] = {
#include "player/big_textures/dat_8c555ea8.inc"
};
static NJS_MKEY_F dat_8c555ec8[2] = {
#include "player/big_textures/dat_8c555ec8.inc"
};
static NJS_MKEY_F dat_8c555ee8[2] = {
#include "player/big_textures/dat_8c555ee8.inc"
};
static NJS_MKEY_A dat_8c555f08[2] = {
#include "player/big_textures/dat_8c555f08.inc"
};
static NJS_MKEY_F dat_8c555f28[2] = {
#include "player/big_textures/dat_8c555f28.inc"
};
static NJS_MKEY_A dat_8c555f48[2] = {
#include "player/big_textures/dat_8c555f48.inc"
};
static NJS_MKEY_F dat_8c555f68[2] = {
#include "player/big_textures/dat_8c555f68.inc"
};
static NJS_MDATA2 dat_8c555f88[73] = {
#include "player/big_textures/dat_8c555f88.inc"
};
static NJS_MOTION dat_8c556418 = {
#include "player/big_textures/dat_8c556418.inc"
};
NJS_ACTION big_fall_start_action = {
#include "player/big_textures/big_fall_start_action.inc"
};
static NJS_MKEY_F dat_8c55642c[32] = {
#include "player/big_textures/dat_8c55642c.inc"
};
static NJS_MKEY_A dat_8c55662c[2] = {
#include "player/big_textures/dat_8c55662c.inc"
};
static NJS_MKEY_A dat_8c55664c[32] = {
#include "player/big_textures/dat_8c55664c.inc"
};
static NJS_MKEY_A dat_8c55684c[31] = {
#include "player/big_textures/dat_8c55684c.inc"
};
static NJS_MKEY_A dat_8c556a3c[2] = {
#include "player/big_textures/dat_8c556a3c.inc"
};
static NJS_MKEY_A dat_8c556a5c[32] = {
#include "player/big_textures/dat_8c556a5c.inc"
};
static NJS_MKEY_A dat_8c556c5c[32] = {
#include "player/big_textures/dat_8c556c5c.inc"
};
static NJS_MKEY_A dat_8c556e5c[2] = {
#include "player/big_textures/dat_8c556e5c.inc"
};
static NJS_MKEY_A dat_8c556e7c[32] = {
#include "player/big_textures/dat_8c556e7c.inc"
};
static NJS_MKEY_A dat_8c55707c[32] = {
#include "player/big_textures/dat_8c55707c.inc"
};
static NJS_MKEY_A dat_8c55727c[2] = {
#include "player/big_textures/dat_8c55727c.inc"
};
static NJS_MKEY_A dat_8c55729c[32] = {
#include "player/big_textures/dat_8c55729c.inc"
};
static NJS_MKEY_A dat_8c55749c[32] = {
#include "player/big_textures/dat_8c55749c.inc"
};
static NJS_MKEY_A dat_8c55769c[32] = {
#include "player/big_textures/dat_8c55769c.inc"
};
static NJS_MKEY_A dat_8c55789c[2] = {
#include "player/big_textures/dat_8c55789c.inc"
};
static NJS_MKEY_A dat_8c5578bc[32] = {
#include "player/big_textures/dat_8c5578bc.inc"
};
static NJS_MKEY_A dat_8c557abc[32] = {
#include "player/big_textures/dat_8c557abc.inc"
};
static NJS_MKEY_A dat_8c557cbc[32] = {
#include "player/big_textures/dat_8c557cbc.inc"
};
static NJS_MKEY_A dat_8c557ebc[32] = {
#include "player/big_textures/dat_8c557ebc.inc"
};
static NJS_MKEY_A dat_8c5580bc[32] = {
#include "player/big_textures/dat_8c5580bc.inc"
};
static NJS_MKEY_A dat_8c5582bc[32] = {
#include "player/big_textures/dat_8c5582bc.inc"
};
static NJS_MKEY_A dat_8c5584bc[26] = {
#include "player/big_textures/dat_8c5584bc.inc"
};
static NJS_MKEY_A dat_8c55865c[31] = {
#include "player/big_textures/dat_8c55865c.inc"
};
static NJS_MKEY_A dat_8c55884c[32] = {
#include "player/big_textures/dat_8c55884c.inc"
};
static NJS_MKEY_A dat_8c558a4c[2] = {
#include "player/big_textures/dat_8c558a4c.inc"
};
static NJS_MKEY_A dat_8c558a6c[2] = {
#include "player/big_textures/dat_8c558a6c.inc"
};
static NJS_MKEY_A dat_8c558a8c[32] = {
#include "player/big_textures/dat_8c558a8c.inc"
};
static NJS_MKEY_A dat_8c558c8c[2] = {
#include "player/big_textures/dat_8c558c8c.inc"
};
static NJS_MKEY_A dat_8c558cac[2] = {
#include "player/big_textures/dat_8c558cac.inc"
};
static NJS_MKEY_A dat_8c558ccc[32] = {
#include "player/big_textures/dat_8c558ccc.inc"
};
static NJS_MDATA2 dat_8c558ecc[73] = {
#include "player/big_textures/dat_8c558ecc.inc"
};
static NJS_MOTION dat_8c55935c = {
#include "player/big_textures/dat_8c55935c.inc"
};
NJS_ACTION big_run_action = {
#include "player/big_textures/big_run_action.inc"
};
static NJS_MKEY_F dat_8c559370[32] = {
#include "player/big_textures/dat_8c559370.inc"
};
static NJS_MKEY_A dat_8c559570[2] = {
#include "player/big_textures/dat_8c559570.inc"
};
static NJS_MKEY_A dat_8c559590[32] = {
#include "player/big_textures/dat_8c559590.inc"
};
static NJS_MKEY_A dat_8c559790[32] = {
#include "player/big_textures/dat_8c559790.inc"
};
static NJS_MKEY_A dat_8c559990[32] = {
#include "player/big_textures/dat_8c559990.inc"
};
static NJS_MKEY_A dat_8c559b90[2] = {
#include "player/big_textures/dat_8c559b90.inc"
};
static NJS_MKEY_A dat_8c559bb0[32] = {
#include "player/big_textures/dat_8c559bb0.inc"
};
static NJS_MKEY_A dat_8c559db0[32] = {
#include "player/big_textures/dat_8c559db0.inc"
};
static NJS_MKEY_A dat_8c559fb0[32] = {
#include "player/big_textures/dat_8c559fb0.inc"
};
static NJS_MKEY_A dat_8c55a1b0[32] = {
#include "player/big_textures/dat_8c55a1b0.inc"
};
static NJS_MKEY_A dat_8c55a3b0[2] = {
#include "player/big_textures/dat_8c55a3b0.inc"
};
static NJS_MKEY_A dat_8c55a3d0[32] = {
#include "player/big_textures/dat_8c55a3d0.inc"
};
static NJS_MKEY_A dat_8c55a5d0[31] = {
#include "player/big_textures/dat_8c55a5d0.inc"
};
static NJS_MKEY_A dat_8c55a7c0[32] = {
#include "player/big_textures/dat_8c55a7c0.inc"
};
static NJS_MKEY_A dat_8c55a9c0[32] = {
#include "player/big_textures/dat_8c55a9c0.inc"
};
static NJS_MKEY_A dat_8c55abc0[32] = {
#include "player/big_textures/dat_8c55abc0.inc"
};
static NJS_MKEY_A dat_8c55adc0[32] = {
#include "player/big_textures/dat_8c55adc0.inc"
};
static NJS_MKEY_A dat_8c55afc0[31] = {
#include "player/big_textures/dat_8c55afc0.inc"
};
static NJS_MKEY_A dat_8c55b1b0[31] = {
#include "player/big_textures/dat_8c55b1b0.inc"
};
static NJS_MKEY_A dat_8c55b3a0[31] = {
#include "player/big_textures/dat_8c55b3a0.inc"
};
static NJS_MKEY_A dat_8c55b590[2] = {
#include "player/big_textures/dat_8c55b590.inc"
};
static NJS_MKEY_A dat_8c55b5b0[32] = {
#include "player/big_textures/dat_8c55b5b0.inc"
};
static NJS_MKEY_A dat_8c55b7b0[32] = {
#include "player/big_textures/dat_8c55b7b0.inc"
};
static NJS_MKEY_A dat_8c55b9b0[32] = {
#include "player/big_textures/dat_8c55b9b0.inc"
};
static NJS_MKEY_A dat_8c55bbb0[2] = {
#include "player/big_textures/dat_8c55bbb0.inc"
};
static NJS_MKEY_A dat_8c55bbd0[2] = {
#include "player/big_textures/dat_8c55bbd0.inc"
};
static NJS_MDATA2 dat_8c55bbf0[73] = {
#include "player/big_textures/dat_8c55bbf0.inc"
};
static NJS_MOTION dat_8c55c080 = {
#include "player/big_textures/dat_8c55c080.inc"
};
NJS_ACTION big_stand_action = {
#include "player/big_textures/big_stand_action.inc"
};
static NJS_MKEY_F dat_8c55c094[32] = {
#include "player/big_textures/dat_8c55c094.inc"
};
static NJS_MKEY_A dat_8c55c294[2] = {
#include "player/big_textures/dat_8c55c294.inc"
};
static NJS_MKEY_A dat_8c55c2b4[32] = {
#include "player/big_textures/dat_8c55c2b4.inc"
};
static NJS_MKEY_A dat_8c55c4b4[32] = {
#include "player/big_textures/dat_8c55c4b4.inc"
};
static NJS_MKEY_A dat_8c55c6b4[2] = {
#include "player/big_textures/dat_8c55c6b4.inc"
};
static NJS_MKEY_A dat_8c55c6d4[32] = {
#include "player/big_textures/dat_8c55c6d4.inc"
};
static NJS_MKEY_A dat_8c55c8d4[32] = {
#include "player/big_textures/dat_8c55c8d4.inc"
};
static NJS_MKEY_A dat_8c55cad4[2] = {
#include "player/big_textures/dat_8c55cad4.inc"
};
static NJS_MKEY_A dat_8c55caf4[32] = {
#include "player/big_textures/dat_8c55caf4.inc"
};
static NJS_MKEY_A dat_8c55ccf4[32] = {
#include "player/big_textures/dat_8c55ccf4.inc"
};
static NJS_MKEY_A dat_8c55cef4[2] = {
#include "player/big_textures/dat_8c55cef4.inc"
};
static NJS_MKEY_A dat_8c55cf14[32] = {
#include "player/big_textures/dat_8c55cf14.inc"
};
static NJS_MKEY_A dat_8c55d114[32] = {
#include "player/big_textures/dat_8c55d114.inc"
};
static NJS_MKEY_A dat_8c55d314[32] = {
#include "player/big_textures/dat_8c55d314.inc"
};
static NJS_MKEY_A dat_8c55d514[2] = {
#include "player/big_textures/dat_8c55d514.inc"
};
static NJS_MKEY_A dat_8c55d534[32] = {
#include "player/big_textures/dat_8c55d534.inc"
};
static NJS_MKEY_A dat_8c55d734[32] = {
#include "player/big_textures/dat_8c55d734.inc"
};
static NJS_MKEY_A dat_8c55d934[32] = {
#include "player/big_textures/dat_8c55d934.inc"
};
static NJS_MKEY_A dat_8c55db34[32] = {
#include "player/big_textures/dat_8c55db34.inc"
};
static NJS_MKEY_A dat_8c55dd34[32] = {
#include "player/big_textures/dat_8c55dd34.inc"
};
static NJS_MKEY_A dat_8c55df34[32] = {
#include "player/big_textures/dat_8c55df34.inc"
};
static NJS_MKEY_A dat_8c55e134[32] = {
#include "player/big_textures/dat_8c55e134.inc"
};
static NJS_MKEY_A dat_8c55e334[32] = {
#include "player/big_textures/dat_8c55e334.inc"
};
static NJS_MKEY_A dat_8c55e534[32] = {
#include "player/big_textures/dat_8c55e534.inc"
};
static NJS_MKEY_A dat_8c55e734[2] = {
#include "player/big_textures/dat_8c55e734.inc"
};
static NJS_MKEY_A dat_8c55e754[2] = {
#include "player/big_textures/dat_8c55e754.inc"
};
static NJS_MKEY_A dat_8c55e774[32] = {
#include "player/big_textures/dat_8c55e774.inc"
};
static NJS_MKEY_A dat_8c55e974[2] = {
#include "player/big_textures/dat_8c55e974.inc"
};
static NJS_MKEY_A dat_8c55e994[2] = {
#include "player/big_textures/dat_8c55e994.inc"
};
static NJS_MKEY_A dat_8c55e9b4[32] = {
#include "player/big_textures/dat_8c55e9b4.inc"
};
static NJS_MDATA2 dat_8c55ebb4[73] = {
#include "player/big_textures/dat_8c55ebb4.inc"
};
static NJS_MOTION dat_8c55f044 = {
#include "player/big_textures/dat_8c55f044.inc"
};
NJS_ACTION big_walk_action = {
#include "player/big_textures/big_walk_action.inc"
};
static NJS_MKEY_A dat_8c55f058[17] = {
#include "player/big_textures/dat_8c55f058.inc"
};
static NJS_MKEY_A dat_8c55f168[2] = {
#include "player/big_textures/dat_8c55f168.inc"
};
static NJS_MKEY_A dat_8c55f188[17] = {
#include "player/big_textures/dat_8c55f188.inc"
};
static NJS_MKEY_A dat_8c55f298[17] = {
#include "player/big_textures/dat_8c55f298.inc"
};
static NJS_MKEY_A dat_8c55f3a8[17] = {
#include "player/big_textures/dat_8c55f3a8.inc"
};
static NJS_MKEY_A dat_8c55f4b8[17] = {
#include "player/big_textures/dat_8c55f4b8.inc"
};
static NJS_MKEY_A dat_8c55f5c8[2] = {
#include "player/big_textures/dat_8c55f5c8.inc"
};
static NJS_MKEY_A dat_8c55f5e8[17] = {
#include "player/big_textures/dat_8c55f5e8.inc"
};
static NJS_MKEY_A dat_8c55f6f8[17] = {
#include "player/big_textures/dat_8c55f6f8.inc"
};
static NJS_MKEY_A dat_8c55f808[17] = {
#include "player/big_textures/dat_8c55f808.inc"
};
static NJS_MKEY_A dat_8c55f918[17] = {
#include "player/big_textures/dat_8c55f918.inc"
};
static NJS_MKEY_A dat_8c55fa28[17] = {
#include "player/big_textures/dat_8c55fa28.inc"
};
static NJS_MKEY_A dat_8c55fb38[2] = {
#include "player/big_textures/dat_8c55fb38.inc"
};
static NJS_MKEY_A dat_8c55fb58[2] = {
#include "player/big_textures/dat_8c55fb58.inc"
};
static NJS_MKEY_A dat_8c55fb78[2] = {
#include "player/big_textures/dat_8c55fb78.inc"
};
static NJS_MKEY_A dat_8c55fb98[17] = {
#include "player/big_textures/dat_8c55fb98.inc"
};
static NJS_MKEY_A dat_8c55fca8[17] = {
#include "player/big_textures/dat_8c55fca8.inc"
};
static NJS_MKEY_A dat_8c55fdb8[17] = {
#include "player/big_textures/dat_8c55fdb8.inc"
};
static NJS_MKEY_A dat_8c55fec8[17] = {
#include "player/big_textures/dat_8c55fec8.inc"
};
static NJS_MKEY_A dat_8c55ffd8[17] = {
#include "player/big_textures/dat_8c55ffd8.inc"
};
static NJS_MKEY_A dat_8c5600e8[2] = {
#include "player/big_textures/dat_8c5600e8.inc"
};
static NJS_MKEY_A dat_8c560108[17] = {
#include "player/big_textures/dat_8c560108.inc"
};
static NJS_MKEY_A dat_8c560218[17] = {
#include "player/big_textures/dat_8c560218.inc"
};
static NJS_MKEY_A dat_8c560328[17] = {
#include "player/big_textures/dat_8c560328.inc"
};
static NJS_MKEY_A dat_8c560438[17] = {
#include "player/big_textures/dat_8c560438.inc"
};
static NJS_MKEY_A dat_8c560548[17] = {
#include "player/big_textures/dat_8c560548.inc"
};
static NJS_MKEY_A dat_8c560658[2] = {
#include "player/big_textures/dat_8c560658.inc"
};
static NJS_MKEY_A dat_8c560678[2] = {
#include "player/big_textures/dat_8c560678.inc"
};
static NJS_MDATA2 dat_8c560698[73] = {
#include "player/big_textures/dat_8c560698.inc"
};
static NJS_MOTION dat_8c560b28 = {
#include "player/big_textures/dat_8c560b28.inc"
};
NJS_ACTION big_fall_action = {
#include "player/big_textures/big_fall_action.inc"
};
static NJS_MKEY_F dat_8c560b3c[16] = {
#include "player/big_textures/dat_8c560b3c.inc"
};
static NJS_MKEY_F dat_8c560c3c[16] = {
#include "player/big_textures/dat_8c560c3c.inc"
};
static NJS_MKEY_A dat_8c560d3c[16] = {
#include "player/big_textures/dat_8c560d3c.inc"
};
static NJS_MKEY_A dat_8c560e3c[2] = {
#include "player/big_textures/dat_8c560e3c.inc"
};
static NJS_MKEY_A dat_8c560e5c[16] = {
#include "player/big_textures/dat_8c560e5c.inc"
};
static NJS_MKEY_A dat_8c560f5c[16] = {
#include "player/big_textures/dat_8c560f5c.inc"
};
static NJS_MKEY_A dat_8c56105c[16] = {
#include "player/big_textures/dat_8c56105c.inc"
};
static NJS_MKEY_A dat_8c56115c[16] = {
#include "player/big_textures/dat_8c56115c.inc"
};
static NJS_MKEY_A dat_8c56125c[2] = {
#include "player/big_textures/dat_8c56125c.inc"
};
static NJS_MKEY_A dat_8c56127c[16] = {
#include "player/big_textures/dat_8c56127c.inc"
};
static NJS_MKEY_A dat_8c56137c[16] = {
#include "player/big_textures/dat_8c56137c.inc"
};
static NJS_MKEY_A dat_8c56147c[16] = {
#include "player/big_textures/dat_8c56147c.inc"
};
static NJS_MKEY_A dat_8c56157c[16] = {
#include "player/big_textures/dat_8c56157c.inc"
};
static NJS_MKEY_A dat_8c56167c[16] = {
#include "player/big_textures/dat_8c56167c.inc"
};
static NJS_MKEY_A dat_8c56177c[2] = {
#include "player/big_textures/dat_8c56177c.inc"
};
static NJS_MKEY_A dat_8c56179c[16] = {
#include "player/big_textures/dat_8c56179c.inc"
};
static NJS_MKEY_A dat_8c56189c[2] = {
#include "player/big_textures/dat_8c56189c.inc"
};
static NJS_MKEY_A dat_8c5618bc[16] = {
#include "player/big_textures/dat_8c5618bc.inc"
};
static NJS_MKEY_A dat_8c5619bc[16] = {
#include "player/big_textures/dat_8c5619bc.inc"
};
static NJS_MKEY_A dat_8c561abc[16] = {
#include "player/big_textures/dat_8c561abc.inc"
};
static NJS_MKEY_A dat_8c561bbc[16] = {
#include "player/big_textures/dat_8c561bbc.inc"
};
static NJS_MKEY_F dat_8c561cbc[2] = {
#include "player/big_textures/dat_8c561cbc.inc"
};
static NJS_MKEY_A dat_8c561cdc[2] = {
#include "player/big_textures/dat_8c561cdc.inc"
};
static NJS_MKEY_A dat_8c561cfc[2] = {
#include "player/big_textures/dat_8c561cfc.inc"
};
static NJS_MKEY_A dat_8c561d1c[16] = {
#include "player/big_textures/dat_8c561d1c.inc"
};
static NJS_MKEY_A dat_8c561e1c[16] = {
#include "player/big_textures/dat_8c561e1c.inc"
};
static NJS_MKEY_A dat_8c561f1c[16] = {
#include "player/big_textures/dat_8c561f1c.inc"
};
static NJS_MKEY_A dat_8c56201c[16] = {
#include "player/big_textures/dat_8c56201c.inc"
};
static NJS_MKEY_F dat_8c56211c[2] = {
#include "player/big_textures/dat_8c56211c.inc"
};
static NJS_MKEY_F dat_8c56213c[16] = {
#include "player/big_textures/dat_8c56213c.inc"
};
static NJS_MKEY_F dat_8c56223c[2] = {
#include "player/big_textures/dat_8c56223c.inc"
};
static NJS_MKEY_F dat_8c56225c[2] = {
#include "player/big_textures/dat_8c56225c.inc"
};
static NJS_MKEY_F dat_8c56227c[16] = {
#include "player/big_textures/dat_8c56227c.inc"
};
static NJS_MKEY_F dat_8c56237c[16] = {
#include "player/big_textures/dat_8c56237c.inc"
};
static NJS_MKEY_F dat_8c56247c[16] = {
#include "player/big_textures/dat_8c56247c.inc"
};
static NJS_MKEY_A dat_8c56257c[16] = {
#include "player/big_textures/dat_8c56257c.inc"
};
static NJS_MKEY_F dat_8c56267c[16] = {
#include "player/big_textures/dat_8c56267c.inc"
};
static NJS_MKEY_F dat_8c56277c[2] = {
#include "player/big_textures/dat_8c56277c.inc"
};
static NJS_MKEY_F dat_8c56279c[16] = {
#include "player/big_textures/dat_8c56279c.inc"
};
static NJS_MKEY_F dat_8c56289c[2] = {
#include "player/big_textures/dat_8c56289c.inc"
};
static NJS_MKEY_F dat_8c5628bc[2] = {
#include "player/big_textures/dat_8c5628bc.inc"
};
static NJS_MKEY_F dat_8c5628dc[2] = {
#include "player/big_textures/dat_8c5628dc.inc"
};
static NJS_MKEY_F dat_8c5628fc[2] = {
#include "player/big_textures/dat_8c5628fc.inc"
};
static NJS_MKEY_F dat_8c56291c[2] = {
#include "player/big_textures/dat_8c56291c.inc"
};
static NJS_MKEY_F dat_8c56293c[2] = {
#include "player/big_textures/dat_8c56293c.inc"
};
static NJS_MKEY_A dat_8c56295c[2] = {
#include "player/big_textures/dat_8c56295c.inc"
};
static NJS_MKEY_F dat_8c56297c[2] = {
#include "player/big_textures/dat_8c56297c.inc"
};
static NJS_MKEY_F dat_8c56299c[2] = {
#include "player/big_textures/dat_8c56299c.inc"
};
static NJS_MKEY_F dat_8c5629bc[2] = {
#include "player/big_textures/dat_8c5629bc.inc"
};
static NJS_MKEY_A dat_8c5629dc[2] = {
#include "player/big_textures/dat_8c5629dc.inc"
};
static NJS_MKEY_F dat_8c5629fc[2] = {
#include "player/big_textures/dat_8c5629fc.inc"
};
static NJS_MKEY_F dat_8c562a1c[2] = {
#include "player/big_textures/dat_8c562a1c.inc"
};
static NJS_MKEY_F dat_8c562a3c[2] = {
#include "player/big_textures/dat_8c562a3c.inc"
};
static NJS_MDATA3 dat_8c562a5c[73] = {
#include "player/big_textures/dat_8c562a5c.inc"
};
static NJS_MOTION dat_8c563134 = {
#include "player/big_textures/dat_8c563134.inc"
};
NJS_ACTION big_land_action = {
#include "player/big_textures/big_land_action.inc"
};
static NJS_TEXNAME fishing_textures[65] = {
    {TEXT_37, 0, 0},  {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0},  {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0},  {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0},  {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0},  {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
    {TEXT_57, 0, 0},  {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0},
    {TEXT_61, 0, 0},  {TEXT_62, 0, 0}, {TEXT_63, 0, 0}, {TEXT_64, 0, 0},
    {TEXT_65, 0, 0},  {TEXT_66, 0, 0}, {TEXT_67, 0, 0}, {TEXT_68, 0, 0},
    {TEXT_69, 0, 0},  {TEXT_70, 0, 0}, {TEXT_71, 0, 0}, {TEXT_72, 0, 0},
    {TEXT_73, 0, 0},  {TEXT_74, 0, 0}, {TEXT_75, 0, 0}, {TEXT_76, 0, 0},
    {TEXT_77, 0, 0},  {TEXT_78, 0, 0}, {TEXT_79, 0, 0}, {TEXT_80, 0, 0},
    {TEXT_81, 0, 0},  {TEXT_82, 0, 0}, {TEXT_83, 0, 0}, {TEXT_84, 0, 0},
    {TEXT_85, 0, 0},  {TEXT_86, 0, 0}, {TEXT_87, 0, 0}, {TEXT_88, 0, 0},
    {TEXT_89, 0, 0},  {TEXT_90, 0, 0}, {TEXT_91, 0, 0}, {TEXT_92, 0, 0},
    {TEXT_93, 0, 0},  {TEXT_94, 0, 0}, {TEXT_95, 0, 0}, {TEXT_96, 0, 0},
    {TEXT_97, 0, 0},  {TEXT_98, 0, 0}, {TEXT_99, 0, 0}, {TEXT_100, 0, 0},
    {TEXT_101, 0, 0},
};
NJS_TEXLIST fishing_texlist = {
#include "player/big_textures/fishing_texlist.inc"
};
static NJS_MATERIAL dat_8c56345c[1] = {
#include "player/big_textures/dat_8c56345c.inc"
};
