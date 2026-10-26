/* the fish's texture lists, models and actions, which #pragma section FISH
 * puts in DFISH. CFISH holds the texture names their texnames point at. Big's
 * four stages link both sections after their bss */
#include <ninja.h>
#include <sg_xpt.h>

#include "stg01/fish_models.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 6
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 7
#define TEXT_23_LENGTH 7
#define TEXT_24_LENGTH 7
#define TEXT_25_LENGTH 7
#define TEXT_26_LENGTH 7
#define TEXT_27_LENGTH 6
#define TEXT_28_LENGTH 6
#define TEXT_29_LENGTH 6
#define TEXT_30_LENGTH 6
#define TEXT_31_LENGTH 6
#define TEXT_32_LENGTH 8
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 8
#define TEXT_36_LENGTH 6
#define TEXT_37_LENGTH 8
#define TEXT_38_LENGTH 8
#define TEXT_39_LENGTH 8
#define TEXT_40_LENGTH 8
#define TEXT_41_LENGTH 8
#define TEXT_42_LENGTH 8
#define TEXT_43_LENGTH 5
#define TEXT_44_LENGTH 5
#define TEXT_45_LENGTH 5
#define TEXT_46_LENGTH 5
#define TEXT_47_LENGTH 5
#define TEXT_48_LENGTH 6
#define TEXT_49_LENGTH 5
#define TEXT_50_LENGTH 6
#define TEXT_51_LENGTH 6
#define TEXT_52_LENGTH 6
#define TEXT_53_LENGTH 11
#define TEXT_54_LENGTH 6
#define TEXT_55_LENGTH 6
#define TEXT_56_LENGTH 6
#define TEXT_57_LENGTH 6
#define TEXT_58_LENGTH 6
#define TEXT_59_LENGTH 6
#define TEXT_60_LENGTH 6
#define TEXT_61_LENGTH 6
#define TEXT_62_LENGTH 6
#define TEXT_63_LENGTH 6
#define TEXT_64_LENGTH 6
#define TEXT_65_LENGTH 6
#define TEXT_66_LENGTH 8
#define TEXT_67_LENGTH 8
#define TEXT_68_LENGTH 8
#define TEXT_69_LENGTH 8
#define TEXT_70_LENGTH 8
#define TEXT_71_LENGTH 8
#define TEXT_72_LENGTH 8
#define TEXT_73_LENGTH 8
#define TEXT_74_LENGTH 8
#define TEXT_75_LENGTH 8
#define TEXT_76_LENGTH 8
#define TEXT_77_LENGTH 7
#define TEXT_78_LENGTH 7
#define TEXT_79_LENGTH 7
#define TEXT_80_LENGTH 7
#define TEXT_81_LENGTH 7
#define TEXT_82_LENGTH 7
#define TEXT_83_LENGTH 7
#define TEXT_84_LENGTH 7
#define TEXT_85_LENGTH 7
#define TEXT_86_LENGTH 7
#define TEXT_87_LENGTH 7
#define TEXT_88_LENGTH 7
#define TEXT_89_LENGTH 7
#define TEXT_90_LENGTH 7
#define TEXT_91_LENGTH 7
#define TEXT_92_LENGTH 7
#define TEXT_93_LENGTH 7
#define TEXT_94_LENGTH 7
#define TEXT_95_LENGTH 7
#include "stg01/fish_models/text.inc"

#pragma section FISH

static NJS_TEXNAME fish_sb_texnames[7] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0},
};
NJS_TEXLIST fish_sb_texlist = {
#include "stg01/fish_models/fish_sb_texlist.inc"
};
static NJS_MATERIAL fish_sb_25_materials[5] = {
#include "stg01/fish_models/fish_sb_25_materials.inc"
};
static Sint16 fish_sb_25_meshes0[20] = {
#include "stg01/fish_models/fish_sb_25_meshes0.inc"
};
static Sint16 fish_sb_25_meshes1[63] = {
#include "stg01/fish_models/fish_sb_25_meshes1.inc"
};
static Sint16 fish_sb_25_meshes2[5] = {
#include "stg01/fish_models/fish_sb_25_meshes2.inc"
};
static Sint16 fish_sb_25_meshes3[18] = {
#include "stg01/fish_models/fish_sb_25_meshes3.inc"
};
static Sint16 fish_sb_25_meshes4[15] = {
#include "stg01/fish_models/fish_sb_25_meshes4.inc"
};
static NJS_COLOR fish_sb_25_uvs0[16] = {
#include "stg01/fish_models/fish_sb_25_uvs0.inc"
};
static NJS_COLOR fish_sb_25_uvs1[56] = {
#include "stg01/fish_models/fish_sb_25_uvs1.inc"
};
static NJS_COLOR fish_sb_25_uvs2[4] = {
#include "stg01/fish_models/fish_sb_25_uvs2.inc"
};
static NJS_COLOR fish_sb_25_uvs3[15] = {
#include "stg01/fish_models/fish_sb_25_uvs3.inc"
};
static NJS_COLOR fish_sb_25_uvs4[13] = {
#include "stg01/fish_models/fish_sb_25_uvs4.inc"
};
static NJS_MESHSET fish_sb_25_meshsets[5] = {
#include "stg01/fish_models/fish_sb_25_meshsets.inc"
};
static NJS_POINT3 fish_sb_25_points[68] = {
#include "stg01/fish_models/fish_sb_25_points.inc"
};
static NJS_VECTOR fish_sb_25_normals[68] = {
#include "stg01/fish_models/fish_sb_25_normals.inc"
};
static NJS_MODEL fish_sb_25_model = {
#include "stg01/fish_models/fish_sb_25_model.inc"
};
static NJS_OBJECT fish_sb_25 = {
#include "stg01/fish_models/fish_sb_25.inc"
};
static NJS_MATERIAL fish_sb_24_materials[1] = {
#include "stg01/fish_models/fish_sb_24_materials.inc"
};
static Sint16 fish_sb_24_meshes0[12] = {
#include "stg01/fish_models/fish_sb_24_meshes0.inc"
};
static NJS_COLOR fish_sb_24_uvs0[10] = {
#include "stg01/fish_models/fish_sb_24_uvs0.inc"
};
static NJS_MESHSET fish_sb_24_meshsets[1] = {
#include "stg01/fish_models/fish_sb_24_meshsets.inc"
};
static NJS_POINT3 fish_sb_24_points[8] = {
#include "stg01/fish_models/fish_sb_24_points.inc"
};
static NJS_VECTOR fish_sb_24_normals[8] = {
#include "stg01/fish_models/fish_sb_24_normals.inc"
};
static NJS_MODEL fish_sb_24_model = {
#include "stg01/fish_models/fish_sb_24_model.inc"
};
static NJS_OBJECT fish_sb_24 = {
#include "stg01/fish_models/fish_sb_24.inc"
};
static NJS_OBJECT fish_sb_23 = {
#include "stg01/fish_models/fish_sb_23.inc"
};
static NJS_OBJECT fish_sb_22 = {
#include "stg01/fish_models/fish_sb_22.inc"
};
static NJS_OBJECT fish_sb_21 = {
#include "stg01/fish_models/fish_sb_21.inc"
};
static NJS_MATERIAL fish_sb_20_materials[1] = {
#include "stg01/fish_models/fish_sb_20_materials.inc"
};
static Sint16 fish_sb_20_meshes0[12] = {
#include "stg01/fish_models/fish_sb_20_meshes0.inc"
};
static NJS_COLOR fish_sb_20_uvs0[10] = {
#include "stg01/fish_models/fish_sb_20_uvs0.inc"
};
static NJS_MESHSET fish_sb_20_meshsets[1] = {
#include "stg01/fish_models/fish_sb_20_meshsets.inc"
};
static NJS_POINT3 fish_sb_20_points[8] = {
#include "stg01/fish_models/fish_sb_20_points.inc"
};
static NJS_VECTOR fish_sb_20_normals[8] = {
#include "stg01/fish_models/fish_sb_20_normals.inc"
};
static NJS_MODEL fish_sb_20_model = {
#include "stg01/fish_models/fish_sb_20_model.inc"
};
static NJS_OBJECT fish_sb_20 = {
#include "stg01/fish_models/fish_sb_20.inc"
};
static NJS_OBJECT fish_sb_19 = {
#include "stg01/fish_models/fish_sb_19.inc"
};
static NJS_OBJECT fish_sb_18 = {
#include "stg01/fish_models/fish_sb_18.inc"
};
static NJS_OBJECT fish_sb_17 = {
#include "stg01/fish_models/fish_sb_17.inc"
};
static NJS_MATERIAL fish_sb_16_materials[2] = {
#include "stg01/fish_models/fish_sb_16_materials.inc"
};
static Sint16 fish_sb_16_meshes0[24] = {
#include "stg01/fish_models/fish_sb_16_meshes0.inc"
};
static Sint16 fish_sb_16_meshes1[58] = {
#include "stg01/fish_models/fish_sb_16_meshes1.inc"
};
static NJS_COLOR fish_sb_16_uvs0[20] = {
#include "stg01/fish_models/fish_sb_16_uvs0.inc"
};
static NJS_COLOR fish_sb_16_uvs1[52] = {
#include "stg01/fish_models/fish_sb_16_uvs1.inc"
};
static NJS_MESHSET fish_sb_16_meshsets[2] = {
#include "stg01/fish_models/fish_sb_16_meshsets.inc"
};
static NJS_POINT3 fish_sb_16_points[33] = {
#include "stg01/fish_models/fish_sb_16_points.inc"
};
static NJS_VECTOR fish_sb_16_normals[33] = {
#include "stg01/fish_models/fish_sb_16_normals.inc"
};
static NJS_MODEL fish_sb_16_model = {
#include "stg01/fish_models/fish_sb_16_model.inc"
};
static NJS_OBJECT fish_sb_16 = {
#include "stg01/fish_models/fish_sb_16.inc"
};
static NJS_OBJECT fish_sb_15 = {
#include "stg01/fish_models/fish_sb_15.inc"
};
static NJS_OBJECT fish_sb_14 = {
#include "stg01/fish_models/fish_sb_14.inc"
};
static NJS_OBJECT fish_sb_13 = {
#include "stg01/fish_models/fish_sb_13.inc"
};
static NJS_MATERIAL fish_sb_12_materials[3] = {
#include "stg01/fish_models/fish_sb_12_materials.inc"
};
static Sint16 fish_sb_12_meshes0[20] = {
#include "stg01/fish_models/fish_sb_12_meshes0.inc"
};
static Sint16 fish_sb_12_meshes1[55] = {
#include "stg01/fish_models/fish_sb_12_meshes1.inc"
};
static Sint16 fish_sb_12_meshes2[22] = {
#include "stg01/fish_models/fish_sb_12_meshes2.inc"
};
static NJS_COLOR fish_sb_12_uvs0[16] = {
#include "stg01/fish_models/fish_sb_12_uvs0.inc"
};
static NJS_COLOR fish_sb_12_uvs1[48] = {
#include "stg01/fish_models/fish_sb_12_uvs1.inc"
};
static NJS_COLOR fish_sb_12_uvs2[18] = {
#include "stg01/fish_models/fish_sb_12_uvs2.inc"
};
static NJS_MESHSET fish_sb_12_meshsets[3] = {
#include "stg01/fish_models/fish_sb_12_meshsets.inc"
};
static NJS_POINT3 fish_sb_12_points[31] = {
#include "stg01/fish_models/fish_sb_12_points.inc"
};
static NJS_VECTOR fish_sb_12_normals[31] = {
#include "stg01/fish_models/fish_sb_12_normals.inc"
};
static NJS_MODEL fish_sb_12_model = {
#include "stg01/fish_models/fish_sb_12_model.inc"
};
static NJS_OBJECT fish_sb_12 = {
#include "stg01/fish_models/fish_sb_12.inc"
};
static NJS_OBJECT fish_sb_11 = {
#include "stg01/fish_models/fish_sb_11.inc"
};
static NJS_OBJECT fish_sb_10 = {
#include "stg01/fish_models/fish_sb_10.inc"
};
static NJS_OBJECT fish_sb_9 = {
#include "stg01/fish_models/fish_sb_9.inc"
};
static NJS_MATERIAL fish_sb_8_materials[4] = {
#include "stg01/fish_models/fish_sb_8_materials.inc"
};
static Sint16 fish_sb_8_meshes0[31] = {
#include "stg01/fish_models/fish_sb_8_meshes0.inc"
};
static Sint16 fish_sb_8_meshes1[46] = {
#include "stg01/fish_models/fish_sb_8_meshes1.inc"
};
static Sint16 fish_sb_8_meshes2[7] = {
#include "stg01/fish_models/fish_sb_8_meshes2.inc"
};
static Sint16 fish_sb_8_meshes3[6] = {
#include "stg01/fish_models/fish_sb_8_meshes3.inc"
};
static NJS_COLOR fish_sb_8_uvs0[26] = {
#include "stg01/fish_models/fish_sb_8_uvs0.inc"
};
static NJS_COLOR fish_sb_8_uvs1[39] = {
#include "stg01/fish_models/fish_sb_8_uvs1.inc"
};
static NJS_COLOR fish_sb_8_uvs2[6] = {
#include "stg01/fish_models/fish_sb_8_uvs2.inc"
};
static NJS_COLOR fish_sb_8_uvs3[5] = {
#include "stg01/fish_models/fish_sb_8_uvs3.inc"
};
static NJS_MESHSET fish_sb_8_meshsets[4] = {
#include "stg01/fish_models/fish_sb_8_meshsets.inc"
};
static NJS_POINT3 fish_sb_8_points[57] = {
#include "stg01/fish_models/fish_sb_8_points.inc"
};
static NJS_VECTOR fish_sb_8_normals[57] = {
#include "stg01/fish_models/fish_sb_8_normals.inc"
};
static NJS_MODEL fish_sb_8_model = {
#include "stg01/fish_models/fish_sb_8_model.inc"
};
static NJS_OBJECT fish_sb_8 = {
#include "stg01/fish_models/fish_sb_8.inc"
};
static NJS_MATERIAL fish_sb_7_materials[1] = {
#include "stg01/fish_models/fish_sb_7_materials.inc"
};
static Sint16 fish_sb_7_meshes0[19] = {
#include "stg01/fish_models/fish_sb_7_meshes0.inc"
};
static NJS_COLOR fish_sb_7_uvs0[18] = {
#include "stg01/fish_models/fish_sb_7_uvs0.inc"
};
static NJS_MESHSET fish_sb_7_meshsets[1] = {
#include "stg01/fish_models/fish_sb_7_meshsets.inc"
};
static NJS_POINT3 fish_sb_7_points[16] = {
#include "stg01/fish_models/fish_sb_7_points.inc"
};
static NJS_VECTOR fish_sb_7_normals[16] = {
#include "stg01/fish_models/fish_sb_7_normals.inc"
};
static NJS_MODEL fish_sb_7_model = {
#include "stg01/fish_models/fish_sb_7_model.inc"
};
static NJS_OBJECT fish_sb_7 = {
#include "stg01/fish_models/fish_sb_7.inc"
};
static NJS_MATERIAL fish_sb_6_materials[2] = {
#include "stg01/fish_models/fish_sb_6_materials.inc"
};
static Sint16 fish_sb_6_meshes0[28] = {
#include "stg01/fish_models/fish_sb_6_meshes0.inc"
};
static Sint16 fish_sb_6_meshes1[12] = {
#include "stg01/fish_models/fish_sb_6_meshes1.inc"
};
static NJS_COLOR fish_sb_6_uvs0[24] = {
#include "stg01/fish_models/fish_sb_6_uvs0.inc"
};
static NJS_COLOR fish_sb_6_uvs1[10] = {
#include "stg01/fish_models/fish_sb_6_uvs1.inc"
};
static NJS_MESHSET fish_sb_6_meshsets[2] = {
#include "stg01/fish_models/fish_sb_6_meshsets.inc"
};
static NJS_POINT3 fish_sb_6_points[28] = {
#include "stg01/fish_models/fish_sb_6_points.inc"
};
static NJS_VECTOR fish_sb_6_normals[28] = {
#include "stg01/fish_models/fish_sb_6_normals.inc"
};
static NJS_MODEL fish_sb_6_model = {
#include "stg01/fish_models/fish_sb_6_model.inc"
};
static NJS_OBJECT fish_sb_6 = {
#include "stg01/fish_models/fish_sb_6.inc"
};
static NJS_OBJECT fish_sb_5 = {
#include "stg01/fish_models/fish_sb_5.inc"
};
static NJS_OBJECT fish_sb_4 = {
#include "stg01/fish_models/fish_sb_4.inc"
};
static NJS_OBJECT fish_sb_3 = {
#include "stg01/fish_models/fish_sb_3.inc"
};
static NJS_OBJECT fish_sb_2 = {
#include "stg01/fish_models/fish_sb_2.inc"
};
static NJS_OBJECT fish_sb_1 = {
#include "stg01/fish_models/fish_sb_1.inc"
};
NJS_OBJECT fish_sb_object = {
#include "stg01/fish_models/fish_sb_object.inc"
};

static NJS_TEXNAME fish_bb_texnames[7] = {
    {TEXT_8, 0, 0},  {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0},
    {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
};
NJS_TEXLIST fish_bb_texlist = {
#include "stg01/fish_models/fish_bb_texlist.inc"
};
static NJS_MATERIAL fish_bb_25_materials[1] = {
#include "stg01/fish_models/fish_bb_25_materials.inc"
};
static Sint16 fish_bb_25_meshes0[14] = {
#include "stg01/fish_models/fish_bb_25_meshes0.inc"
};
static NJS_COLOR fish_bb_25_uvs0[12] = {
#include "stg01/fish_models/fish_bb_25_uvs0.inc"
};
static NJS_MESHSET fish_bb_25_meshsets[1] = {
#include "stg01/fish_models/fish_bb_25_meshsets.inc"
};
static NJS_POINT3 fish_bb_25_points[8] = {
#include "stg01/fish_models/fish_bb_25_points.inc"
};
static NJS_VECTOR fish_bb_25_normals[8] = {
#include "stg01/fish_models/fish_bb_25_normals.inc"
};
static NJS_MODEL fish_bb_25_model = {
#include "stg01/fish_models/fish_bb_25_model.inc"
};
static NJS_OBJECT fish_bb_25 = {
#include "stg01/fish_models/fish_bb_25.inc"
};
static NJS_OBJECT fish_bb_24 = {
#include "stg01/fish_models/fish_bb_24.inc"
};
static NJS_OBJECT fish_bb_23 = {
#include "stg01/fish_models/fish_bb_23.inc"
};
static NJS_OBJECT fish_bb_22 = {
#include "stg01/fish_models/fish_bb_22.inc"
};
static NJS_MATERIAL fish_bb_21_materials[1] = {
#include "stg01/fish_models/fish_bb_21_materials.inc"
};
static Sint16 fish_bb_21_meshes0[17] = {
#include "stg01/fish_models/fish_bb_21_meshes0.inc"
};
static NJS_COLOR fish_bb_21_uvs0[14] = {
#include "stg01/fish_models/fish_bb_21_uvs0.inc"
};
static NJS_MESHSET fish_bb_21_meshsets[1] = {
#include "stg01/fish_models/fish_bb_21_meshsets.inc"
};
static NJS_POINT3 fish_bb_21_points[8] = {
#include "stg01/fish_models/fish_bb_21_points.inc"
};
static NJS_VECTOR fish_bb_21_normals[8] = {
#include "stg01/fish_models/fish_bb_21_normals.inc"
};
static NJS_MODEL fish_bb_21_model = {
#include "stg01/fish_models/fish_bb_21_model.inc"
};
static NJS_OBJECT fish_bb_21 = {
#include "stg01/fish_models/fish_bb_21.inc"
};
static NJS_OBJECT fish_bb_20 = {
#include "stg01/fish_models/fish_bb_20.inc"
};
static NJS_OBJECT fish_bb_19 = {
#include "stg01/fish_models/fish_bb_19.inc"
};
static NJS_OBJECT fish_bb_18 = {
#include "stg01/fish_models/fish_bb_18.inc"
};
static NJS_MATERIAL fish_bb_17_materials[3] = {
#include "stg01/fish_models/fish_bb_17_materials.inc"
};
static Sint16 fish_bb_17_meshes0[26] = {
#include "stg01/fish_models/fish_bb_17_meshes0.inc"
};
static Sint16 fish_bb_17_meshes1[54] = {
#include "stg01/fish_models/fish_bb_17_meshes1.inc"
};
static Sint16 fish_bb_17_meshes2[18] = {
#include "stg01/fish_models/fish_bb_17_meshes2.inc"
};
static NJS_COLOR fish_bb_17_uvs0[22] = {
#include "stg01/fish_models/fish_bb_17_uvs0.inc"
};
static NJS_COLOR fish_bb_17_uvs1[48] = {
#include "stg01/fish_models/fish_bb_17_uvs1.inc"
};
static NJS_COLOR fish_bb_17_uvs2[16] = {
#include "stg01/fish_models/fish_bb_17_uvs2.inc"
};
static NJS_MESHSET fish_bb_17_meshsets[3] = {
#include "stg01/fish_models/fish_bb_17_meshsets.inc"
};
static NJS_POINT3 fish_bb_17_points[52] = {
#include "stg01/fish_models/fish_bb_17_points.inc"
};
static NJS_VECTOR fish_bb_17_normals[52] = {
#include "stg01/fish_models/fish_bb_17_normals.inc"
};
static NJS_MODEL fish_bb_17_model = {
#include "stg01/fish_models/fish_bb_17_model.inc"
};
static NJS_OBJECT fish_bb_17 = {
#include "stg01/fish_models/fish_bb_17.inc"
};
static NJS_MATERIAL fish_bb_16_materials[3] = {
#include "stg01/fish_models/fish_bb_16_materials.inc"
};
static Sint16 fish_bb_16_meshes0[37] = {
#include "stg01/fish_models/fish_bb_16_meshes0.inc"
};
static Sint16 fish_bb_16_meshes1[35] = {
#include "stg01/fish_models/fish_bb_16_meshes1.inc"
};
static Sint16 fish_bb_16_meshes2[16] = {
#include "stg01/fish_models/fish_bb_16_meshes2.inc"
};
static NJS_COLOR fish_bb_16_uvs0[30] = {
#include "stg01/fish_models/fish_bb_16_uvs0.inc"
};
static NJS_COLOR fish_bb_16_uvs1[30] = {
#include "stg01/fish_models/fish_bb_16_uvs1.inc"
};
static NJS_COLOR fish_bb_16_uvs2[13] = {
#include "stg01/fish_models/fish_bb_16_uvs2.inc"
};
static NJS_MESHSET fish_bb_16_meshsets[3] = {
#include "stg01/fish_models/fish_bb_16_meshsets.inc"
};
static NJS_POINT3 fish_bb_16_points[35] = {
#include "stg01/fish_models/fish_bb_16_points.inc"
};
static NJS_VECTOR fish_bb_16_normals[35] = {
#include "stg01/fish_models/fish_bb_16_normals.inc"
};
static NJS_MODEL fish_bb_16_model = {
#include "stg01/fish_models/fish_bb_16_model.inc"
};
static NJS_OBJECT fish_bb_16 = {
#include "stg01/fish_models/fish_bb_16.inc"
};
static NJS_MATERIAL fish_bb_15_materials[2] = {
#include "stg01/fish_models/fish_bb_15_materials.inc"
};
static Sint16 fish_bb_15_meshes0[77] = {
#include "stg01/fish_models/fish_bb_15_meshes0.inc"
};
static Sint16 fish_bb_15_meshes1[19] = {
#include "stg01/fish_models/fish_bb_15_meshes1.inc"
};
static NJS_COLOR fish_bb_15_uvs0[66] = {
#include "stg01/fish_models/fish_bb_15_uvs0.inc"
};
static NJS_COLOR fish_bb_15_uvs1[15] = {
#include "stg01/fish_models/fish_bb_15_uvs1.inc"
};
static NJS_MESHSET fish_bb_15_meshsets[2] = {
#include "stg01/fish_models/fish_bb_15_meshsets.inc"
};
static NJS_POINT3 fish_bb_15_points[29] = {
#include "stg01/fish_models/fish_bb_15_points.inc"
};
static NJS_VECTOR fish_bb_15_normals[29] = {
#include "stg01/fish_models/fish_bb_15_normals.inc"
};
static NJS_MODEL fish_bb_15_model = {
#include "stg01/fish_models/fish_bb_15_model.inc"
};
static NJS_OBJECT fish_bb_15 = {
#include "stg01/fish_models/fish_bb_15.inc"
};
static NJS_OBJECT fish_bb_14 = {
#include "stg01/fish_models/fish_bb_14.inc"
};
static NJS_OBJECT fish_bb_13 = {
#include "stg01/fish_models/fish_bb_13.inc"
};
static NJS_OBJECT fish_bb_12 = {
#include "stg01/fish_models/fish_bb_12.inc"
};
static NJS_OBJECT fish_bb_11 = {
#include "stg01/fish_models/fish_bb_11.inc"
};
static NJS_OBJECT fish_bb_10 = {
#include "stg01/fish_models/fish_bb_10.inc"
};
static NJS_OBJECT fish_bb_9 = {
#include "stg01/fish_models/fish_bb_9.inc"
};
static NJS_MATERIAL fish_bb_8_materials[2] = {
#include "stg01/fish_models/fish_bb_8_materials.inc"
};
static Sint16 fish_bb_8_meshes0[30] = {
#include "stg01/fish_models/fish_bb_8_meshes0.inc"
};
static Sint16 fish_bb_8_meshes1[33] = {
#include "stg01/fish_models/fish_bb_8_meshes1.inc"
};
static NJS_COLOR fish_bb_8_uvs0[26] = {
#include "stg01/fish_models/fish_bb_8_uvs0.inc"
};
static NJS_COLOR fish_bb_8_uvs1[30] = {
#include "stg01/fish_models/fish_bb_8_uvs1.inc"
};
static NJS_MESHSET fish_bb_8_meshsets[2] = {
#include "stg01/fish_models/fish_bb_8_meshsets.inc"
};
static NJS_POINT3 fish_bb_8_points[40] = {
#include "stg01/fish_models/fish_bb_8_points.inc"
};
static NJS_VECTOR fish_bb_8_normals[40] = {
#include "stg01/fish_models/fish_bb_8_normals.inc"
};
static NJS_MODEL fish_bb_8_model = {
#include "stg01/fish_models/fish_bb_8_model.inc"
};
static NJS_OBJECT fish_bb_8 = {
#include "stg01/fish_models/fish_bb_8.inc"
};
static NJS_MATERIAL fish_bb_7_materials[1] = {
#include "stg01/fish_models/fish_bb_7_materials.inc"
};
static Sint16 fish_bb_7_meshes0[15] = {
#include "stg01/fish_models/fish_bb_7_meshes0.inc"
};
static NJS_COLOR fish_bb_7_uvs0[14] = {
#include "stg01/fish_models/fish_bb_7_uvs0.inc"
};
static NJS_MESHSET fish_bb_7_meshsets[1] = {
#include "stg01/fish_models/fish_bb_7_meshsets.inc"
};
static NJS_POINT3 fish_bb_7_points[12] = {
#include "stg01/fish_models/fish_bb_7_points.inc"
};
static NJS_VECTOR fish_bb_7_normals[12] = {
#include "stg01/fish_models/fish_bb_7_normals.inc"
};
static NJS_MODEL fish_bb_7_model = {
#include "stg01/fish_models/fish_bb_7_model.inc"
};
static NJS_OBJECT fish_bb_7 = {
#include "stg01/fish_models/fish_bb_7.inc"
};
static NJS_MATERIAL fish_bb_6_materials[2] = {
#include "stg01/fish_models/fish_bb_6_materials.inc"
};
static Sint16 fish_bb_6_meshes0[21] = {
#include "stg01/fish_models/fish_bb_6_meshes0.inc"
};
static Sint16 fish_bb_6_meshes1[12] = {
#include "stg01/fish_models/fish_bb_6_meshes1.inc"
};
static NJS_COLOR fish_bb_6_uvs0[18] = {
#include "stg01/fish_models/fish_bb_6_uvs0.inc"
};
static NJS_COLOR fish_bb_6_uvs1[10] = {
#include "stg01/fish_models/fish_bb_6_uvs1.inc"
};
static NJS_MESHSET fish_bb_6_meshsets[2] = {
#include "stg01/fish_models/fish_bb_6_meshsets.inc"
};
static NJS_POINT3 fish_bb_6_points[15] = {
#include "stg01/fish_models/fish_bb_6_points.inc"
};
static NJS_VECTOR fish_bb_6_normals[15] = {
#include "stg01/fish_models/fish_bb_6_normals.inc"
};
static NJS_MODEL fish_bb_6_model = {
#include "stg01/fish_models/fish_bb_6_model.inc"
};
static NJS_OBJECT fish_bb_6 = {
#include "stg01/fish_models/fish_bb_6.inc"
};
static NJS_OBJECT fish_bb_5 = {
#include "stg01/fish_models/fish_bb_5.inc"
};
static NJS_OBJECT fish_bb_4 = {
#include "stg01/fish_models/fish_bb_4.inc"
};
static NJS_OBJECT fish_bb_3 = {
#include "stg01/fish_models/fish_bb_3.inc"
};
static NJS_OBJECT fish_bb_2 = {
#include "stg01/fish_models/fish_bb_2.inc"
};
static NJS_OBJECT fish_bb_1 = {
#include "stg01/fish_models/fish_bb_1.inc"
};
NJS_OBJECT fish_bb_object = {
#include "stg01/fish_models/fish_bb_object.inc"
};

static NJS_TEXNAME fish_sira_texnames[8] = {
    {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
    {TEXT_11, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0}, {TEXT_21, 0, 0},
};
NJS_TEXLIST fish_sira_texlist = {
#include "stg01/fish_models/fish_sira_texlist.inc"
};
static NJS_MATERIAL fish_sira_25_materials[2] = {
#include "stg01/fish_models/fish_sira_25_materials.inc"
};
static Sint16 fish_sira_25_meshes0[13] = {
#include "stg01/fish_models/fish_sira_25_meshes0.inc"
};
static Sint16 fish_sira_25_meshes1[16] = {
#include "stg01/fish_models/fish_sira_25_meshes1.inc"
};
static NJS_COLOR fish_sira_25_uvs0[11] = {
#include "stg01/fish_models/fish_sira_25_uvs0.inc"
};
static NJS_COLOR fish_sira_25_uvs1[13] = {
#include "stg01/fish_models/fish_sira_25_uvs1.inc"
};
static NJS_MESHSET fish_sira_25_meshsets[2] = {
#include "stg01/fish_models/fish_sira_25_meshsets.inc"
};
static NJS_POINT3 fish_sira_25_points[15] = {
#include "stg01/fish_models/fish_sira_25_points.inc"
};
static NJS_VECTOR fish_sira_25_normals[15] = {
#include "stg01/fish_models/fish_sira_25_normals.inc"
};
static NJS_MODEL fish_sira_25_model = {
#include "stg01/fish_models/fish_sira_25_model.inc"
};
static NJS_OBJECT fish_sira_25 = {
#include "stg01/fish_models/fish_sira_25.inc"
};
static NJS_OBJECT fish_sira_24 = {
#include "stg01/fish_models/fish_sira_24.inc"
};
static NJS_OBJECT fish_sira_23 = {
#include "stg01/fish_models/fish_sira_23.inc"
};
static NJS_OBJECT fish_sira_22 = {
#include "stg01/fish_models/fish_sira_22.inc"
};
static NJS_MATERIAL fish_sira_21_materials[2] = {
#include "stg01/fish_models/fish_sira_21_materials.inc"
};
static Sint16 fish_sira_21_meshes0[13] = {
#include "stg01/fish_models/fish_sira_21_meshes0.inc"
};
static Sint16 fish_sira_21_meshes1[16] = {
#include "stg01/fish_models/fish_sira_21_meshes1.inc"
};
static NJS_COLOR fish_sira_21_uvs0[11] = {
#include "stg01/fish_models/fish_sira_21_uvs0.inc"
};
static NJS_COLOR fish_sira_21_uvs1[13] = {
#include "stg01/fish_models/fish_sira_21_uvs1.inc"
};
static NJS_MESHSET fish_sira_21_meshsets[2] = {
#include "stg01/fish_models/fish_sira_21_meshsets.inc"
};
static NJS_POINT3 fish_sira_21_points[15] = {
#include "stg01/fish_models/fish_sira_21_points.inc"
};
static NJS_VECTOR fish_sira_21_normals[15] = {
#include "stg01/fish_models/fish_sira_21_normals.inc"
};
static NJS_MODEL fish_sira_21_model = {
#include "stg01/fish_models/fish_sira_21_model.inc"
};
static NJS_OBJECT fish_sira_21 = {
#include "stg01/fish_models/fish_sira_21.inc"
};
static NJS_OBJECT fish_sira_20 = {
#include "stg01/fish_models/fish_sira_20.inc"
};
static NJS_OBJECT fish_sira_19 = {
#include "stg01/fish_models/fish_sira_19.inc"
};
static NJS_OBJECT fish_sira_18 = {
#include "stg01/fish_models/fish_sira_18.inc"
};
static NJS_MATERIAL fish_sira_17_materials[3] = {
#include "stg01/fish_models/fish_sira_17_materials.inc"
};
static Sint16 fish_sira_17_meshes0[81] = {
#include "stg01/fish_models/fish_sira_17_meshes0.inc"
};
static Sint16 fish_sira_17_meshes1[21] = {
#include "stg01/fish_models/fish_sira_17_meshes1.inc"
};
static Sint16 fish_sira_17_meshes2[21] = {
#include "stg01/fish_models/fish_sira_17_meshes2.inc"
};
static NJS_COLOR fish_sira_17_uvs0[70] = {
#include "stg01/fish_models/fish_sira_17_uvs0.inc"
};
static NJS_COLOR fish_sira_17_uvs1[18] = {
#include "stg01/fish_models/fish_sira_17_uvs1.inc"
};
static NJS_COLOR fish_sira_17_uvs2[18] = {
#include "stg01/fish_models/fish_sira_17_uvs2.inc"
};
static NJS_MESHSET fish_sira_17_meshsets[3] = {
#include "stg01/fish_models/fish_sira_17_meshsets.inc"
};
static NJS_POINT3 fish_sira_17_points[48] = {
#include "stg01/fish_models/fish_sira_17_points.inc"
};
static NJS_VECTOR fish_sira_17_normals[48] = {
#include "stg01/fish_models/fish_sira_17_normals.inc"
};
static NJS_MODEL fish_sira_17_model = {
#include "stg01/fish_models/fish_sira_17_model.inc"
};
static NJS_OBJECT fish_sira_17 = {
#include "stg01/fish_models/fish_sira_17.inc"
};
static NJS_MATERIAL fish_sira_16_materials[4] = {
#include "stg01/fish_models/fish_sira_16_materials.inc"
};
static Sint16 fish_sira_16_meshes0[67] = {
#include "stg01/fish_models/fish_sira_16_meshes0.inc"
};
static Sint16 fish_sira_16_meshes1[7] = {
#include "stg01/fish_models/fish_sira_16_meshes1.inc"
};
static Sint16 fish_sira_16_meshes2[12] = {
#include "stg01/fish_models/fish_sira_16_meshes2.inc"
};
static Sint16 fish_sira_16_meshes3[26] = {
#include "stg01/fish_models/fish_sira_16_meshes3.inc"
};
static NJS_COLOR fish_sira_16_uvs0[57] = {
#include "stg01/fish_models/fish_sira_16_uvs0.inc"
};
static NJS_COLOR fish_sira_16_uvs1[6] = {
#include "stg01/fish_models/fish_sira_16_uvs1.inc"
};
static NJS_COLOR fish_sira_16_uvs2[10] = {
#include "stg01/fish_models/fish_sira_16_uvs2.inc"
};
static NJS_COLOR fish_sira_16_uvs3[22] = {
#include "stg01/fish_models/fish_sira_16_uvs3.inc"
};
static NJS_MESHSET fish_sira_16_meshsets[4] = {
#include "stg01/fish_models/fish_sira_16_meshsets.inc"
};
static NJS_POINT3 fish_sira_16_points[36] = {
#include "stg01/fish_models/fish_sira_16_points.inc"
};
static NJS_VECTOR fish_sira_16_normals[36] = {
#include "stg01/fish_models/fish_sira_16_normals.inc"
};
static NJS_MODEL fish_sira_16_model = {
#include "stg01/fish_models/fish_sira_16_model.inc"
};
static NJS_OBJECT fish_sira_16 = {
#include "stg01/fish_models/fish_sira_16.inc"
};
static NJS_OBJECT fish_sira_15 = {
#include "stg01/fish_models/fish_sira_15.inc"
};
static NJS_OBJECT fish_sira_14 = {
#include "stg01/fish_models/fish_sira_14.inc"
};
static NJS_OBJECT fish_sira_13 = {
#include "stg01/fish_models/fish_sira_13.inc"
};
static NJS_MATERIAL fish_sira_12_materials[5] = {
#include "stg01/fish_models/fish_sira_12_materials.inc"
};
static Sint16 fish_sira_12_meshes0[91] = {
#include "stg01/fish_models/fish_sira_12_meshes0.inc"
};
static Sint16 fish_sira_12_meshes1[12] = {
#include "stg01/fish_models/fish_sira_12_meshes1.inc"
};
static Sint16 fish_sira_12_meshes2[20] = {
#include "stg01/fish_models/fish_sira_12_meshes2.inc"
};
static Sint16 fish_sira_12_meshes3[7] = {
#include "stg01/fish_models/fish_sira_12_meshes3.inc"
};
static Sint16 fish_sira_12_meshes4[12] = {
#include "stg01/fish_models/fish_sira_12_meshes4.inc"
};
static NJS_COLOR fish_sira_12_uvs0[75] = {
#include "stg01/fish_models/fish_sira_12_uvs0.inc"
};
static NJS_COLOR fish_sira_12_uvs1[10] = {
#include "stg01/fish_models/fish_sira_12_uvs1.inc"
};
static NJS_COLOR fish_sira_12_uvs2[16] = {
#include "stg01/fish_models/fish_sira_12_uvs2.inc"
};
static NJS_COLOR fish_sira_12_uvs3[6] = {
#include "stg01/fish_models/fish_sira_12_uvs3.inc"
};
static NJS_COLOR fish_sira_12_uvs4[10] = {
#include "stg01/fish_models/fish_sira_12_uvs4.inc"
};
static NJS_MESHSET fish_sira_12_meshsets[5] = {
#include "stg01/fish_models/fish_sira_12_meshsets.inc"
};
static NJS_POINT3 fish_sira_12_points[45] = {
#include "stg01/fish_models/fish_sira_12_points.inc"
};
static NJS_VECTOR fish_sira_12_normals[45] = {
#include "stg01/fish_models/fish_sira_12_normals.inc"
};
static NJS_MODEL fish_sira_12_model = {
#include "stg01/fish_models/fish_sira_12_model.inc"
};
static NJS_OBJECT fish_sira_12 = {
#include "stg01/fish_models/fish_sira_12.inc"
};
static NJS_OBJECT fish_sira_11 = {
#include "stg01/fish_models/fish_sira_11.inc"
};
static NJS_OBJECT fish_sira_10 = {
#include "stg01/fish_models/fish_sira_10.inc"
};
static NJS_OBJECT fish_sira_9 = {
#include "stg01/fish_models/fish_sira_9.inc"
};
static NJS_MATERIAL fish_sira_8_materials[3] = {
#include "stg01/fish_models/fish_sira_8_materials.inc"
};
static Sint16 fish_sira_8_meshes0[21] = {
#include "stg01/fish_models/fish_sira_8_meshes0.inc"
};
static Sint16 fish_sira_8_meshes1[78] = {
#include "stg01/fish_models/fish_sira_8_meshes1.inc"
};
static Sint16 fish_sira_8_meshes2[21] = {
#include "stg01/fish_models/fish_sira_8_meshes2.inc"
};
static NJS_COLOR fish_sira_8_uvs0[18] = {
#include "stg01/fish_models/fish_sira_8_uvs0.inc"
};
static NJS_COLOR fish_sira_8_uvs1[66] = {
#include "stg01/fish_models/fish_sira_8_uvs1.inc"
};
static NJS_COLOR fish_sira_8_uvs2[18] = {
#include "stg01/fish_models/fish_sira_8_uvs2.inc"
};
static NJS_MESHSET fish_sira_8_meshsets[3] = {
#include "stg01/fish_models/fish_sira_8_meshsets.inc"
};
static NJS_POINT3 fish_sira_8_points[43] = {
#include "stg01/fish_models/fish_sira_8_points.inc"
};
static NJS_VECTOR fish_sira_8_normals[43] = {
#include "stg01/fish_models/fish_sira_8_normals.inc"
};
static NJS_MODEL fish_sira_8_model = {
#include "stg01/fish_models/fish_sira_8_model.inc"
};
static NJS_OBJECT fish_sira_8 = {
#include "stg01/fish_models/fish_sira_8.inc"
};
static NJS_MATERIAL fish_sira_7_materials[1] = {
#include "stg01/fish_models/fish_sira_7_materials.inc"
};
static Sint16 fish_sira_7_meshes0[14] = {
#include "stg01/fish_models/fish_sira_7_meshes0.inc"
};
static NJS_COLOR fish_sira_7_uvs0[12] = {
#include "stg01/fish_models/fish_sira_7_uvs0.inc"
};
static NJS_MESHSET fish_sira_7_meshsets[1] = {
#include "stg01/fish_models/fish_sira_7_meshsets.inc"
};
static NJS_POINT3 fish_sira_7_points[8] = {
#include "stg01/fish_models/fish_sira_7_points.inc"
};
static NJS_VECTOR fish_sira_7_normals[8] = {
#include "stg01/fish_models/fish_sira_7_normals.inc"
};
static NJS_MODEL fish_sira_7_model = {
#include "stg01/fish_models/fish_sira_7_model.inc"
};
static NJS_OBJECT fish_sira_7 = {
#include "stg01/fish_models/fish_sira_7.inc"
};
static NJS_MATERIAL fish_sira_6_materials[2] = {
#include "stg01/fish_models/fish_sira_6_materials.inc"
};
static Sint16 fish_sira_6_meshes0[10] = {
#include "stg01/fish_models/fish_sira_6_meshes0.inc"
};
static Sint16 fish_sira_6_meshes1[31] = {
#include "stg01/fish_models/fish_sira_6_meshes1.inc"
};
static NJS_COLOR fish_sira_6_uvs0[8] = {
#include "stg01/fish_models/fish_sira_6_uvs0.inc"
};
static NJS_COLOR fish_sira_6_uvs1[26] = {
#include "stg01/fish_models/fish_sira_6_uvs1.inc"
};
static NJS_MESHSET fish_sira_6_meshsets[2] = {
#include "stg01/fish_models/fish_sira_6_meshsets.inc"
};
static NJS_POINT3 fish_sira_6_points[15] = {
#include "stg01/fish_models/fish_sira_6_points.inc"
};
static NJS_VECTOR fish_sira_6_normals[15] = {
#include "stg01/fish_models/fish_sira_6_normals.inc"
};
static NJS_MODEL fish_sira_6_model = {
#include "stg01/fish_models/fish_sira_6_model.inc"
};
static NJS_OBJECT fish_sira_6 = {
#include "stg01/fish_models/fish_sira_6.inc"
};
static NJS_OBJECT fish_sira_5 = {
#include "stg01/fish_models/fish_sira_5.inc"
};
static NJS_OBJECT fish_sira_4 = {
#include "stg01/fish_models/fish_sira_4.inc"
};
static NJS_OBJECT fish_sira_3 = {
#include "stg01/fish_models/fish_sira_3.inc"
};
static NJS_OBJECT fish_sira_2 = {
#include "stg01/fish_models/fish_sira_2.inc"
};
static NJS_OBJECT fish_sira_1 = {
#include "stg01/fish_models/fish_sira_1.inc"
};
NJS_OBJECT fish_sira_object = {
#include "stg01/fish_models/fish_sira_object.inc"
};

static NJS_TEXNAME fish_tai_texnames[6] = {
    {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_11, 0, 0}, {TEXT_26, 0, 0},
};
NJS_TEXLIST fish_tai_texlist = {
#include "stg01/fish_models/fish_tai_texlist.inc"
};
static NJS_MATERIAL fish_tai_25_materials[1] = {
#include "stg01/fish_models/fish_tai_25_materials.inc"
};
static Sint16 fish_tai_25_meshes0[14] = {
#include "stg01/fish_models/fish_tai_25_meshes0.inc"
};
static NJS_COLOR fish_tai_25_uvs0[12] = {
#include "stg01/fish_models/fish_tai_25_uvs0.inc"
};
static NJS_MESHSET fish_tai_25_meshsets[1] = {
#include "stg01/fish_models/fish_tai_25_meshsets.inc"
};
static NJS_POINT3 fish_tai_25_points[8] = {
#include "stg01/fish_models/fish_tai_25_points.inc"
};
static NJS_VECTOR fish_tai_25_normals[8] = {
#include "stg01/fish_models/fish_tai_25_normals.inc"
};
static NJS_MODEL fish_tai_25_model = {
#include "stg01/fish_models/fish_tai_25_model.inc"
};
static NJS_OBJECT fish_tai_25 = {
#include "stg01/fish_models/fish_tai_25.inc"
};
static NJS_OBJECT fish_tai_24 = {
#include "stg01/fish_models/fish_tai_24.inc"
};
static NJS_OBJECT fish_tai_23 = {
#include "stg01/fish_models/fish_tai_23.inc"
};
static NJS_OBJECT fish_tai_22 = {
#include "stg01/fish_models/fish_tai_22.inc"
};
static NJS_MATERIAL fish_tai_21_materials[1] = {
#include "stg01/fish_models/fish_tai_21_materials.inc"
};
static Sint16 fish_tai_21_meshes0[17] = {
#include "stg01/fish_models/fish_tai_21_meshes0.inc"
};
static NJS_COLOR fish_tai_21_uvs0[14] = {
#include "stg01/fish_models/fish_tai_21_uvs0.inc"
};
static NJS_MESHSET fish_tai_21_meshsets[1] = {
#include "stg01/fish_models/fish_tai_21_meshsets.inc"
};
static NJS_POINT3 fish_tai_21_points[8] = {
#include "stg01/fish_models/fish_tai_21_points.inc"
};
static NJS_VECTOR fish_tai_21_normals[8] = {
#include "stg01/fish_models/fish_tai_21_normals.inc"
};
static NJS_MODEL fish_tai_21_model = {
#include "stg01/fish_models/fish_tai_21_model.inc"
};
static NJS_OBJECT fish_tai_21 = {
#include "stg01/fish_models/fish_tai_21.inc"
};
static NJS_OBJECT fish_tai_20 = {
#include "stg01/fish_models/fish_tai_20.inc"
};
static NJS_OBJECT fish_tai_19 = {
#include "stg01/fish_models/fish_tai_19.inc"
};
static NJS_OBJECT fish_tai_18 = {
#include "stg01/fish_models/fish_tai_18.inc"
};
static NJS_MATERIAL fish_tai_17_materials[3] = {
#include "stg01/fish_models/fish_tai_17_materials.inc"
};
static Sint16 fish_tai_17_meshes0[26] = {
#include "stg01/fish_models/fish_tai_17_meshes0.inc"
};
static Sint16 fish_tai_17_meshes1[45] = {
#include "stg01/fish_models/fish_tai_17_meshes1.inc"
};
static Sint16 fish_tai_17_meshes2[16] = {
#include "stg01/fish_models/fish_tai_17_meshes2.inc"
};
static NJS_COLOR fish_tai_17_uvs0[22] = {
#include "stg01/fish_models/fish_tai_17_uvs0.inc"
};
static NJS_COLOR fish_tai_17_uvs1[42] = {
#include "stg01/fish_models/fish_tai_17_uvs1.inc"
};
static NJS_COLOR fish_tai_17_uvs2[14] = {
#include "stg01/fish_models/fish_tai_17_uvs2.inc"
};
static NJS_MESHSET fish_tai_17_meshsets[3] = {
#include "stg01/fish_models/fish_tai_17_meshsets.inc"
};
static NJS_POINT3 fish_tai_17_points[50] = {
#include "stg01/fish_models/fish_tai_17_points.inc"
};
static NJS_VECTOR fish_tai_17_normals[50] = {
#include "stg01/fish_models/fish_tai_17_normals.inc"
};
static NJS_MODEL fish_tai_17_model = {
#include "stg01/fish_models/fish_tai_17_model.inc"
};
static NJS_OBJECT fish_tai_17 = {
#include "stg01/fish_models/fish_tai_17.inc"
};
static NJS_MATERIAL fish_tai_16_materials[2] = {
#include "stg01/fish_models/fish_tai_16_materials.inc"
};
static Sint16 fish_tai_16_meshes0[34] = {
#include "stg01/fish_models/fish_tai_16_meshes0.inc"
};
static Sint16 fish_tai_16_meshes1[27] = {
#include "stg01/fish_models/fish_tai_16_meshes1.inc"
};
static NJS_COLOR fish_tai_16_uvs0[28] = {
#include "stg01/fish_models/fish_tai_16_uvs0.inc"
};
static NJS_COLOR fish_tai_16_uvs1[22] = {
#include "stg01/fish_models/fish_tai_16_uvs1.inc"
};
static NJS_MESHSET fish_tai_16_meshsets[2] = {
#include "stg01/fish_models/fish_tai_16_meshsets.inc"
};
static NJS_POINT3 fish_tai_16_points[20] = {
#include "stg01/fish_models/fish_tai_16_points.inc"
};
static NJS_VECTOR fish_tai_16_normals[20] = {
#include "stg01/fish_models/fish_tai_16_normals.inc"
};
static NJS_MODEL fish_tai_16_model = {
#include "stg01/fish_models/fish_tai_16_model.inc"
};
static NJS_OBJECT fish_tai_16 = {
#include "stg01/fish_models/fish_tai_16.inc"
};
static NJS_OBJECT fish_tai_15 = {
#include "stg01/fish_models/fish_tai_15.inc"
};
static NJS_OBJECT fish_tai_14 = {
#include "stg01/fish_models/fish_tai_14.inc"
};
static NJS_OBJECT fish_tai_13 = {
#include "stg01/fish_models/fish_tai_13.inc"
};
static NJS_MATERIAL fish_tai_12_materials[3] = {
#include "stg01/fish_models/fish_tai_12_materials.inc"
};
static Sint16 fish_tai_12_meshes0[67] = {
#include "stg01/fish_models/fish_tai_12_meshes0.inc"
};
static Sint16 fish_tai_12_meshes1[24] = {
#include "stg01/fish_models/fish_tai_12_meshes1.inc"
};
static Sint16 fish_tai_12_meshes2[24] = {
#include "stg01/fish_models/fish_tai_12_meshes2.inc"
};
static NJS_COLOR fish_tai_12_uvs0[56] = {
#include "stg01/fish_models/fish_tai_12_uvs0.inc"
};
static NJS_COLOR fish_tai_12_uvs1[20] = {
#include "stg01/fish_models/fish_tai_12_uvs1.inc"
};
static NJS_COLOR fish_tai_12_uvs2[20] = {
#include "stg01/fish_models/fish_tai_12_uvs2.inc"
};
static NJS_MESHSET fish_tai_12_meshsets[3] = {
#include "stg01/fish_models/fish_tai_12_meshsets.inc"
};
static NJS_POINT3 fish_tai_12_points[38] = {
#include "stg01/fish_models/fish_tai_12_points.inc"
};
static NJS_VECTOR fish_tai_12_normals[38] = {
#include "stg01/fish_models/fish_tai_12_normals.inc"
};
static NJS_MODEL fish_tai_12_model = {
#include "stg01/fish_models/fish_tai_12_model.inc"
};
static NJS_OBJECT fish_tai_12 = {
#include "stg01/fish_models/fish_tai_12.inc"
};
static NJS_OBJECT fish_tai_11 = {
#include "stg01/fish_models/fish_tai_11.inc"
};
static NJS_OBJECT fish_tai_10 = {
#include "stg01/fish_models/fish_tai_10.inc"
};
static NJS_OBJECT fish_tai_9 = {
#include "stg01/fish_models/fish_tai_9.inc"
};
static NJS_MATERIAL fish_tai_8_materials[2] = {
#include "stg01/fish_models/fish_tai_8_materials.inc"
};
static Sint16 fish_tai_8_meshes0[30] = {
#include "stg01/fish_models/fish_tai_8_meshes0.inc"
};
static Sint16 fish_tai_8_meshes1[26] = {
#include "stg01/fish_models/fish_tai_8_meshes1.inc"
};
static NJS_COLOR fish_tai_8_uvs0[28] = {
#include "stg01/fish_models/fish_tai_8_uvs0.inc"
};
static NJS_COLOR fish_tai_8_uvs1[22] = {
#include "stg01/fish_models/fish_tai_8_uvs1.inc"
};
static NJS_MESHSET fish_tai_8_meshsets[2] = {
#include "stg01/fish_models/fish_tai_8_meshsets.inc"
};
static NJS_POINT3 fish_tai_8_points[33] = {
#include "stg01/fish_models/fish_tai_8_points.inc"
};
static NJS_VECTOR fish_tai_8_normals[33] = {
#include "stg01/fish_models/fish_tai_8_normals.inc"
};
static NJS_MODEL fish_tai_8_model = {
#include "stg01/fish_models/fish_tai_8_model.inc"
};
static NJS_OBJECT fish_tai_8 = {
#include "stg01/fish_models/fish_tai_8.inc"
};
static NJS_MATERIAL fish_tai_7_materials[1] = {
#include "stg01/fish_models/fish_tai_7_materials.inc"
};
static Sint16 fish_tai_7_meshes0[15] = {
#include "stg01/fish_models/fish_tai_7_meshes0.inc"
};
static NJS_COLOR fish_tai_7_uvs0[14] = {
#include "stg01/fish_models/fish_tai_7_uvs0.inc"
};
static NJS_MESHSET fish_tai_7_meshsets[1] = {
#include "stg01/fish_models/fish_tai_7_meshsets.inc"
};
static NJS_POINT3 fish_tai_7_points[12] = {
#include "stg01/fish_models/fish_tai_7_points.inc"
};
static NJS_VECTOR fish_tai_7_normals[12] = {
#include "stg01/fish_models/fish_tai_7_normals.inc"
};
static NJS_MODEL fish_tai_7_model = {
#include "stg01/fish_models/fish_tai_7_model.inc"
};
static NJS_OBJECT fish_tai_7 = {
#include "stg01/fish_models/fish_tai_7.inc"
};
static NJS_MATERIAL fish_tai_6_materials[2] = {
#include "stg01/fish_models/fish_tai_6_materials.inc"
};
static Sint16 fish_tai_6_meshes0[12] = {
#include "stg01/fish_models/fish_tai_6_meshes0.inc"
};
static Sint16 fish_tai_6_meshes1[19] = {
#include "stg01/fish_models/fish_tai_6_meshes1.inc"
};
static NJS_COLOR fish_tai_6_uvs0[10] = {
#include "stg01/fish_models/fish_tai_6_uvs0.inc"
};
static NJS_COLOR fish_tai_6_uvs1[16] = {
#include "stg01/fish_models/fish_tai_6_uvs1.inc"
};
static NJS_MESHSET fish_tai_6_meshsets[2] = {
#include "stg01/fish_models/fish_tai_6_meshsets.inc"
};
static NJS_POINT3 fish_tai_6_points[14] = {
#include "stg01/fish_models/fish_tai_6_points.inc"
};
static NJS_VECTOR fish_tai_6_normals[14] = {
#include "stg01/fish_models/fish_tai_6_normals.inc"
};
static NJS_MODEL fish_tai_6_model = {
#include "stg01/fish_models/fish_tai_6_model.inc"
};
static NJS_OBJECT fish_tai_6 = {
#include "stg01/fish_models/fish_tai_6.inc"
};
static NJS_OBJECT fish_tai_5 = {
#include "stg01/fish_models/fish_tai_5.inc"
};
static NJS_OBJECT fish_tai_4 = {
#include "stg01/fish_models/fish_tai_4.inc"
};
static NJS_OBJECT fish_tai_3 = {
#include "stg01/fish_models/fish_tai_3.inc"
};
static NJS_OBJECT fish_tai_2 = {
#include "stg01/fish_models/fish_tai_2.inc"
};
static NJS_OBJECT fish_tai_1 = {
#include "stg01/fish_models/fish_tai_1.inc"
};
NJS_OBJECT fish_tai_object = {
#include "stg01/fish_models/fish_tai_object.inc"
};

static NJS_TEXNAME fish_id_texnames[6] = {
    {TEXT_27, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0},
    {TEXT_11, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0},
};
NJS_TEXLIST fish_id_texlist = {
#include "stg01/fish_models/fish_id_texlist.inc"
};
static NJS_MATERIAL fish_id_25_materials[1] = {
#include "stg01/fish_models/fish_id_25_materials.inc"
};
static Sint16 fish_id_25_meshes0[14] = {
#include "stg01/fish_models/fish_id_25_meshes0.inc"
};
static NJS_COLOR fish_id_25_uvs0[12] = {
#include "stg01/fish_models/fish_id_25_uvs0.inc"
};
static NJS_MESHSET fish_id_25_meshsets[1] = {
#include "stg01/fish_models/fish_id_25_meshsets.inc"
};
static NJS_POINT3 fish_id_25_points[8] = {
#include "stg01/fish_models/fish_id_25_points.inc"
};
static NJS_VECTOR fish_id_25_normals[8] = {
#include "stg01/fish_models/fish_id_25_normals.inc"
};
static NJS_MODEL fish_id_25_model = {
#include "stg01/fish_models/fish_id_25_model.inc"
};
static NJS_OBJECT fish_id_25 = {
#include "stg01/fish_models/fish_id_25.inc"
};
static NJS_OBJECT fish_id_24 = {
#include "stg01/fish_models/fish_id_24.inc"
};
static NJS_OBJECT fish_id_23 = {
#include "stg01/fish_models/fish_id_23.inc"
};
static NJS_OBJECT fish_id_22 = {
#include "stg01/fish_models/fish_id_22.inc"
};
static NJS_MATERIAL fish_id_21_materials[1] = {
#include "stg01/fish_models/fish_id_21_materials.inc"
};
static Sint16 fish_id_21_meshes0[14] = {
#include "stg01/fish_models/fish_id_21_meshes0.inc"
};
static NJS_COLOR fish_id_21_uvs0[12] = {
#include "stg01/fish_models/fish_id_21_uvs0.inc"
};
static NJS_MESHSET fish_id_21_meshsets[1] = {
#include "stg01/fish_models/fish_id_21_meshsets.inc"
};
static NJS_POINT3 fish_id_21_points[8] = {
#include "stg01/fish_models/fish_id_21_points.inc"
};
static NJS_VECTOR fish_id_21_normals[8] = {
#include "stg01/fish_models/fish_id_21_normals.inc"
};
static NJS_MODEL fish_id_21_model = {
#include "stg01/fish_models/fish_id_21_model.inc"
};
static NJS_OBJECT fish_id_21 = {
#include "stg01/fish_models/fish_id_21.inc"
};
static NJS_OBJECT fish_id_20 = {
#include "stg01/fish_models/fish_id_20.inc"
};
static NJS_OBJECT fish_id_19 = {
#include "stg01/fish_models/fish_id_19.inc"
};
static NJS_OBJECT fish_id_18 = {
#include "stg01/fish_models/fish_id_18.inc"
};
static NJS_MATERIAL fish_id_17_materials[2] = {
#include "stg01/fish_models/fish_id_17_materials.inc"
};
static Sint16 fish_id_17_meshes0[36] = {
#include "stg01/fish_models/fish_id_17_meshes0.inc"
};
static Sint16 fish_id_17_meshes1[57] = {
#include "stg01/fish_models/fish_id_17_meshes1.inc"
};
static NJS_COLOR fish_id_17_uvs0[30] = {
#include "stg01/fish_models/fish_id_17_uvs0.inc"
};
static NJS_COLOR fish_id_17_uvs1[50] = {
#include "stg01/fish_models/fish_id_17_uvs1.inc"
};
static NJS_MESHSET fish_id_17_meshsets[2] = {
#include "stg01/fish_models/fish_id_17_meshsets.inc"
};
static NJS_POINT3 fish_id_17_points[40] = {
#include "stg01/fish_models/fish_id_17_points.inc"
};
static NJS_VECTOR fish_id_17_normals[40] = {
#include "stg01/fish_models/fish_id_17_normals.inc"
};
static NJS_MODEL fish_id_17_model = {
#include "stg01/fish_models/fish_id_17_model.inc"
};
static NJS_OBJECT fish_id_17 = {
#include "stg01/fish_models/fish_id_17.inc"
};
static NJS_MATERIAL fish_id_16_materials[3] = {
#include "stg01/fish_models/fish_id_16_materials.inc"
};
static Sint16 fish_id_16_meshes0[10] = {
#include "stg01/fish_models/fish_id_16_meshes0.inc"
};
static Sint16 fish_id_16_meshes1[43] = {
#include "stg01/fish_models/fish_id_16_meshes1.inc"
};
static Sint16 fish_id_16_meshes2[24] = {
#include "stg01/fish_models/fish_id_16_meshes2.inc"
};
static NJS_COLOR fish_id_16_uvs0[8] = {
#include "stg01/fish_models/fish_id_16_uvs0.inc"
};
static NJS_COLOR fish_id_16_uvs1[36] = {
#include "stg01/fish_models/fish_id_16_uvs1.inc"
};
static NJS_COLOR fish_id_16_uvs2[20] = {
#include "stg01/fish_models/fish_id_16_uvs2.inc"
};
static NJS_MESHSET fish_id_16_meshsets[3] = {
#include "stg01/fish_models/fish_id_16_meshsets.inc"
};
static NJS_POINT3 fish_id_16_points[30] = {
#include "stg01/fish_models/fish_id_16_points.inc"
};
static NJS_VECTOR fish_id_16_normals[30] = {
#include "stg01/fish_models/fish_id_16_normals.inc"
};
static NJS_MODEL fish_id_16_model = {
#include "stg01/fish_models/fish_id_16_model.inc"
};
static NJS_OBJECT fish_id_16 = {
#include "stg01/fish_models/fish_id_16.inc"
};
static NJS_MATERIAL fish_id_15_materials[3] = {
#include "stg01/fish_models/fish_id_15_materials.inc"
};
static Sint16 fish_id_15_meshes0[34] = {
#include "stg01/fish_models/fish_id_15_meshes0.inc"
};
static Sint16 fish_id_15_meshes1[17] = {
#include "stg01/fish_models/fish_id_15_meshes1.inc"
};
static Sint16 fish_id_15_meshes2[10] = {
#include "stg01/fish_models/fish_id_15_meshes2.inc"
};
static NJS_COLOR fish_id_15_uvs0[28] = {
#include "stg01/fish_models/fish_id_15_uvs0.inc"
};
static NJS_COLOR fish_id_15_uvs1[14] = {
#include "stg01/fish_models/fish_id_15_uvs1.inc"
};
static NJS_COLOR fish_id_15_uvs2[8] = {
#include "stg01/fish_models/fish_id_15_uvs2.inc"
};
static NJS_MESHSET fish_id_15_meshsets[3] = {
#include "stg01/fish_models/fish_id_15_meshsets.inc"
};
static NJS_POINT3 fish_id_15_points[20] = {
#include "stg01/fish_models/fish_id_15_points.inc"
};
static NJS_VECTOR fish_id_15_normals[20] = {
#include "stg01/fish_models/fish_id_15_normals.inc"
};
static NJS_MODEL fish_id_15_model = {
#include "stg01/fish_models/fish_id_15_model.inc"
};
static NJS_OBJECT fish_id_15 = {
#include "stg01/fish_models/fish_id_15.inc"
};
static NJS_OBJECT fish_id_14 = {
#include "stg01/fish_models/fish_id_14.inc"
};
static NJS_OBJECT fish_id_13 = {
#include "stg01/fish_models/fish_id_13.inc"
};
static NJS_OBJECT fish_id_12 = {
#include "stg01/fish_models/fish_id_12.inc"
};
static NJS_OBJECT fish_id_11 = {
#include "stg01/fish_models/fish_id_11.inc"
};
static NJS_OBJECT fish_id_10 = {
#include "stg01/fish_models/fish_id_10.inc"
};
static NJS_OBJECT fish_id_9 = {
#include "stg01/fish_models/fish_id_9.inc"
};
static NJS_MATERIAL fish_id_8_materials[2] = {
#include "stg01/fish_models/fish_id_8_materials.inc"
};
static Sint16 fish_id_8_meshes0[30] = {
#include "stg01/fish_models/fish_id_8_meshes0.inc"
};
static Sint16 fish_id_8_meshes1[36] = {
#include "stg01/fish_models/fish_id_8_meshes1.inc"
};
static NJS_COLOR fish_id_8_uvs0[28] = {
#include "stg01/fish_models/fish_id_8_uvs0.inc"
};
static NJS_COLOR fish_id_8_uvs1[30] = {
#include "stg01/fish_models/fish_id_8_uvs1.inc"
};
static NJS_MESHSET fish_id_8_meshsets[2] = {
#include "stg01/fish_models/fish_id_8_meshsets.inc"
};
static NJS_POINT3 fish_id_8_points[35] = {
#include "stg01/fish_models/fish_id_8_points.inc"
};
static NJS_VECTOR fish_id_8_normals[35] = {
#include "stg01/fish_models/fish_id_8_normals.inc"
};
static NJS_MODEL fish_id_8_model = {
#include "stg01/fish_models/fish_id_8_model.inc"
};
static NJS_OBJECT fish_id_8 = {
#include "stg01/fish_models/fish_id_8.inc"
};
static NJS_MATERIAL fish_id_7_materials[1] = {
#include "stg01/fish_models/fish_id_7_materials.inc"
};
static Sint16 fish_id_7_meshes0[18] = {
#include "stg01/fish_models/fish_id_7_meshes0.inc"
};
static NJS_COLOR fish_id_7_uvs0[16] = {
#include "stg01/fish_models/fish_id_7_uvs0.inc"
};
static NJS_MESHSET fish_id_7_meshsets[1] = {
#include "stg01/fish_models/fish_id_7_meshsets.inc"
};
static NJS_POINT3 fish_id_7_points[12] = {
#include "stg01/fish_models/fish_id_7_points.inc"
};
static NJS_VECTOR fish_id_7_normals[12] = {
#include "stg01/fish_models/fish_id_7_normals.inc"
};
static NJS_MODEL fish_id_7_model = {
#include "stg01/fish_models/fish_id_7_model.inc"
};
static NJS_OBJECT fish_id_7 = {
#include "stg01/fish_models/fish_id_7.inc"
};
static NJS_MATERIAL fish_id_6_materials[2] = {
#include "stg01/fish_models/fish_id_6_materials.inc"
};
static Sint16 fish_id_6_meshes0[12] = {
#include "stg01/fish_models/fish_id_6_meshes0.inc"
};
static Sint16 fish_id_6_meshes1[22] = {
#include "stg01/fish_models/fish_id_6_meshes1.inc"
};
static NJS_COLOR fish_id_6_uvs0[10] = {
#include "stg01/fish_models/fish_id_6_uvs0.inc"
};
static NJS_COLOR fish_id_6_uvs1[18] = {
#include "stg01/fish_models/fish_id_6_uvs1.inc"
};
static NJS_MESHSET fish_id_6_meshsets[2] = {
#include "stg01/fish_models/fish_id_6_meshsets.inc"
};
static NJS_POINT3 fish_id_6_points[14] = {
#include "stg01/fish_models/fish_id_6_points.inc"
};
static NJS_VECTOR fish_id_6_normals[14] = {
#include "stg01/fish_models/fish_id_6_normals.inc"
};
static NJS_MODEL fish_id_6_model = {
#include "stg01/fish_models/fish_id_6_model.inc"
};
static NJS_OBJECT fish_id_6 = {
#include "stg01/fish_models/fish_id_6.inc"
};
static NJS_OBJECT fish_id_5 = {
#include "stg01/fish_models/fish_id_5.inc"
};
static NJS_OBJECT fish_id_4 = {
#include "stg01/fish_models/fish_id_4.inc"
};
static NJS_OBJECT fish_id_3 = {
#include "stg01/fish_models/fish_id_3.inc"
};
static NJS_OBJECT fish_id_2 = {
#include "stg01/fish_models/fish_id_2.inc"
};
static NJS_OBJECT fish_id_1 = {
#include "stg01/fish_models/fish_id_1.inc"
};
NJS_OBJECT fish_id_object = {
#include "stg01/fish_models/fish_id_object.inc"
};

static NJS_TEXNAME fish_sake_texnames[8] = {
    {TEXT_32, 0, 0}, {TEXT_2, 0, 0},  {TEXT_11, 0, 0}, {TEXT_33, 0, 0},
    {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0}, {TEXT_37, 0, 0},
};
NJS_TEXLIST fish_sake_texlist = {
#include "stg01/fish_models/fish_sake_texlist.inc"
};
static NJS_MATERIAL fish_sake_25_materials[4] = {
#include "stg01/fish_models/fish_sake_25_materials.inc"
};
static Sint16 fish_sake_25_meshes0[24] = {
#include "stg01/fish_models/fish_sake_25_meshes0.inc"
};
static Sint16 fish_sake_25_meshes1[58] = {
#include "stg01/fish_models/fish_sake_25_meshes1.inc"
};
static Sint16 fish_sake_25_meshes2[18] = {
#include "stg01/fish_models/fish_sake_25_meshes2.inc"
};
static Sint16 fish_sake_25_meshes3[15] = {
#include "stg01/fish_models/fish_sake_25_meshes3.inc"
};
static NJS_COLOR fish_sake_25_uvs0[20] = {
#include "stg01/fish_models/fish_sake_25_uvs0.inc"
};
static NJS_COLOR fish_sake_25_uvs1[52] = {
#include "stg01/fish_models/fish_sake_25_uvs1.inc"
};
static NJS_COLOR fish_sake_25_uvs2[15] = {
#include "stg01/fish_models/fish_sake_25_uvs2.inc"
};
static NJS_COLOR fish_sake_25_uvs3[13] = {
#include "stg01/fish_models/fish_sake_25_uvs3.inc"
};
static NJS_MESHSET fish_sake_25_meshsets[4] = {
#include "stg01/fish_models/fish_sake_25_meshsets.inc"
};
static NJS_POINT3 fish_sake_25_points[67] = {
#include "stg01/fish_models/fish_sake_25_points.inc"
};
static NJS_VECTOR fish_sake_25_normals[67] = {
#include "stg01/fish_models/fish_sake_25_normals.inc"
};
static NJS_MODEL fish_sake_25_model = {
#include "stg01/fish_models/fish_sake_25_model.inc"
};
static NJS_OBJECT fish_sake_25 = {
#include "stg01/fish_models/fish_sake_25.inc"
};
static NJS_MATERIAL fish_sake_24_materials[1] = {
#include "stg01/fish_models/fish_sake_24_materials.inc"
};
static Sint16 fish_sake_24_meshes0[11] = {
#include "stg01/fish_models/fish_sake_24_meshes0.inc"
};
static NJS_COLOR fish_sake_24_uvs0[9] = {
#include "stg01/fish_models/fish_sake_24_uvs0.inc"
};
static NJS_MESHSET fish_sake_24_meshsets[1] = {
#include "stg01/fish_models/fish_sake_24_meshsets.inc"
};
static NJS_POINT3 fish_sake_24_points[7] = {
#include "stg01/fish_models/fish_sake_24_points.inc"
};
static NJS_VECTOR fish_sake_24_normals[7] = {
#include "stg01/fish_models/fish_sake_24_normals.inc"
};
static NJS_MODEL fish_sake_24_model = {
#include "stg01/fish_models/fish_sake_24_model.inc"
};
static NJS_OBJECT fish_sake_24 = {
#include "stg01/fish_models/fish_sake_24.inc"
};
static NJS_OBJECT fish_sake_23 = {
#include "stg01/fish_models/fish_sake_23.inc"
};
static NJS_OBJECT fish_sake_22 = {
#include "stg01/fish_models/fish_sake_22.inc"
};
static NJS_OBJECT fish_sake_21 = {
#include "stg01/fish_models/fish_sake_21.inc"
};
static NJS_MATERIAL fish_sake_20_materials[1] = {
#include "stg01/fish_models/fish_sake_20_materials.inc"
};
static Sint16 fish_sake_20_meshes0[8] = {
#include "stg01/fish_models/fish_sake_20_meshes0.inc"
};
static NJS_COLOR fish_sake_20_uvs0[7] = {
#include "stg01/fish_models/fish_sake_20_uvs0.inc"
};
static NJS_MESHSET fish_sake_20_meshsets[1] = {
#include "stg01/fish_models/fish_sake_20_meshsets.inc"
};
static NJS_POINT3 fish_sake_20_points[7] = {
#include "stg01/fish_models/fish_sake_20_points.inc"
};
static NJS_VECTOR fish_sake_20_normals[7] = {
#include "stg01/fish_models/fish_sake_20_normals.inc"
};
static NJS_MODEL fish_sake_20_model = {
#include "stg01/fish_models/fish_sake_20_model.inc"
};
static NJS_OBJECT fish_sake_20 = {
#include "stg01/fish_models/fish_sake_20.inc"
};
static NJS_OBJECT fish_sake_19 = {
#include "stg01/fish_models/fish_sake_19.inc"
};
static NJS_OBJECT fish_sake_18 = {
#include "stg01/fish_models/fish_sake_18.inc"
};
static NJS_OBJECT fish_sake_17 = {
#include "stg01/fish_models/fish_sake_17.inc"
};
static NJS_MATERIAL fish_sake_16_materials[4] = {
#include "stg01/fish_models/fish_sake_16_materials.inc"
};
static Sint16 fish_sake_16_meshes0[42] = {
#include "stg01/fish_models/fish_sake_16_meshes0.inc"
};
static Sint16 fish_sake_16_meshes1[12] = {
#include "stg01/fish_models/fish_sake_16_meshes1.inc"
};
static Sint16 fish_sake_16_meshes2[10] = {
#include "stg01/fish_models/fish_sake_16_meshes2.inc"
};
static Sint16 fish_sake_16_meshes3[24] = {
#include "stg01/fish_models/fish_sake_16_meshes3.inc"
};
static NJS_COLOR fish_sake_16_uvs0[36] = {
#include "stg01/fish_models/fish_sake_16_uvs0.inc"
};
static NJS_COLOR fish_sake_16_uvs1[10] = {
#include "stg01/fish_models/fish_sake_16_uvs1.inc"
};
static NJS_COLOR fish_sake_16_uvs2[8] = {
#include "stg01/fish_models/fish_sake_16_uvs2.inc"
};
static NJS_COLOR fish_sake_16_uvs3[20] = {
#include "stg01/fish_models/fish_sake_16_uvs3.inc"
};
static NJS_MESHSET fish_sake_16_meshsets[4] = {
#include "stg01/fish_models/fish_sake_16_meshsets.inc"
};
static NJS_POINT3 fish_sake_16_points[39] = {
#include "stg01/fish_models/fish_sake_16_points.inc"
};
static NJS_VECTOR fish_sake_16_normals[39] = {
#include "stg01/fish_models/fish_sake_16_normals.inc"
};
static NJS_MODEL fish_sake_16_model = {
#include "stg01/fish_models/fish_sake_16_model.inc"
};
static NJS_OBJECT fish_sake_16 = {
#include "stg01/fish_models/fish_sake_16.inc"
};
static NJS_MATERIAL fish_sake_15_materials[3] = {
#include "stg01/fish_models/fish_sake_15_materials.inc"
};
static Sint16 fish_sake_15_meshes0[24] = {
#include "stg01/fish_models/fish_sake_15_meshes0.inc"
};
static Sint16 fish_sake_15_meshes1[27] = {
#include "stg01/fish_models/fish_sake_15_meshes1.inc"
};
static Sint16 fish_sake_15_meshes2[31] = {
#include "stg01/fish_models/fish_sake_15_meshes2.inc"
};
static NJS_COLOR fish_sake_15_uvs0[20] = {
#include "stg01/fish_models/fish_sake_15_uvs0.inc"
};
static NJS_COLOR fish_sake_15_uvs1[24] = {
#include "stg01/fish_models/fish_sake_15_uvs1.inc"
};
static NJS_COLOR fish_sake_15_uvs2[28] = {
#include "stg01/fish_models/fish_sake_15_uvs2.inc"
};
static NJS_MESHSET fish_sake_15_meshsets[3] = {
#include "stg01/fish_models/fish_sake_15_meshsets.inc"
};
static NJS_POINT3 fish_sake_15_points[33] = {
#include "stg01/fish_models/fish_sake_15_points.inc"
};
static NJS_VECTOR fish_sake_15_normals[33] = {
#include "stg01/fish_models/fish_sake_15_normals.inc"
};
static NJS_MODEL fish_sake_15_model = {
#include "stg01/fish_models/fish_sake_15_model.inc"
};
static NJS_OBJECT fish_sake_15 = {
#include "stg01/fish_models/fish_sake_15.inc"
};
static NJS_OBJECT fish_sake_14 = {
#include "stg01/fish_models/fish_sake_14.inc"
};
static NJS_OBJECT fish_sake_13 = {
#include "stg01/fish_models/fish_sake_13.inc"
};
static NJS_OBJECT fish_sake_12 = {
#include "stg01/fish_models/fish_sake_12.inc"
};
static NJS_OBJECT fish_sake_11 = {
#include "stg01/fish_models/fish_sake_11.inc"
};
static NJS_OBJECT fish_sake_10 = {
#include "stg01/fish_models/fish_sake_10.inc"
};
static NJS_OBJECT fish_sake_9 = {
#include "stg01/fish_models/fish_sake_9.inc"
};
static NJS_MATERIAL fish_sake_8_materials[2] = {
#include "stg01/fish_models/fish_sake_8_materials.inc"
};
static Sint16 fish_sake_8_meshes0[25] = {
#include "stg01/fish_models/fish_sake_8_meshes0.inc"
};
static Sint16 fish_sake_8_meshes1[9] = {
#include "stg01/fish_models/fish_sake_8_meshes1.inc"
};
static NJS_COLOR fish_sake_8_uvs0[22] = {
#include "stg01/fish_models/fish_sake_8_uvs0.inc"
};
static NJS_COLOR fish_sake_8_uvs1[8] = {
#include "stg01/fish_models/fish_sake_8_uvs1.inc"
};
static NJS_MESHSET fish_sake_8_meshsets[2] = {
#include "stg01/fish_models/fish_sake_8_meshsets.inc"
};
static NJS_POINT3 fish_sake_8_points[24] = {
#include "stg01/fish_models/fish_sake_8_points.inc"
};
static NJS_VECTOR fish_sake_8_normals[24] = {
#include "stg01/fish_models/fish_sake_8_normals.inc"
};
static NJS_MODEL fish_sake_8_model = {
#include "stg01/fish_models/fish_sake_8_model.inc"
};
static NJS_OBJECT fish_sake_8 = {
#include "stg01/fish_models/fish_sake_8.inc"
};
static NJS_MATERIAL fish_sake_7_materials[1] = {
#include "stg01/fish_models/fish_sake_7_materials.inc"
};
static Sint16 fish_sake_7_meshes0[19] = {
#include "stg01/fish_models/fish_sake_7_meshes0.inc"
};
static NJS_COLOR fish_sake_7_uvs0[18] = {
#include "stg01/fish_models/fish_sake_7_uvs0.inc"
};
static NJS_MESHSET fish_sake_7_meshsets[1] = {
#include "stg01/fish_models/fish_sake_7_meshsets.inc"
};
static NJS_POINT3 fish_sake_7_points[16] = {
#include "stg01/fish_models/fish_sake_7_points.inc"
};
static NJS_VECTOR fish_sake_7_normals[16] = {
#include "stg01/fish_models/fish_sake_7_normals.inc"
};
static NJS_MODEL fish_sake_7_model = {
#include "stg01/fish_models/fish_sake_7_model.inc"
};
static NJS_OBJECT fish_sake_7 = {
#include "stg01/fish_models/fish_sake_7.inc"
};
static NJS_MATERIAL fish_sake_6_materials[2] = {
#include "stg01/fish_models/fish_sake_6_materials.inc"
};
static Sint16 fish_sake_6_meshes0[19] = {
#include "stg01/fish_models/fish_sake_6_meshes0.inc"
};
static Sint16 fish_sake_6_meshes1[14] = {
#include "stg01/fish_models/fish_sake_6_meshes1.inc"
};
static NJS_COLOR fish_sake_6_uvs0[18] = {
#include "stg01/fish_models/fish_sake_6_uvs0.inc"
};
static NJS_COLOR fish_sake_6_uvs1[12] = {
#include "stg01/fish_models/fish_sake_6_uvs1.inc"
};
static NJS_MESHSET fish_sake_6_meshsets[2] = {
#include "stg01/fish_models/fish_sake_6_meshsets.inc"
};
static NJS_POINT3 fish_sake_6_points[26] = {
#include "stg01/fish_models/fish_sake_6_points.inc"
};
static NJS_VECTOR fish_sake_6_normals[26] = {
#include "stg01/fish_models/fish_sake_6_normals.inc"
};
static NJS_MODEL fish_sake_6_model = {
#include "stg01/fish_models/fish_sake_6_model.inc"
};
static NJS_OBJECT fish_sake_6 = {
#include "stg01/fish_models/fish_sake_6.inc"
};
static NJS_OBJECT fish_sake_5 = {
#include "stg01/fish_models/fish_sake_5.inc"
};
static NJS_OBJECT fish_sake_4 = {
#include "stg01/fish_models/fish_sake_4.inc"
};
static NJS_OBJECT fish_sake_3 = {
#include "stg01/fish_models/fish_sake_3.inc"
};
static NJS_OBJECT fish_sake_2 = {
#include "stg01/fish_models/fish_sake_2.inc"
};
static NJS_OBJECT fish_sake_1 = {
#include "stg01/fish_models/fish_sake_1.inc"
};
NJS_OBJECT fish_sake_object = {
#include "stg01/fish_models/fish_sake_object.inc"
};

static NJS_TEXNAME fish_kaji_texnames[8] = {
    {TEXT_38, 0, 0}, {TEXT_2, 0, 0},  {TEXT_39, 0, 0}, {TEXT_37, 0, 0},
    {TEXT_40, 0, 0}, {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_11, 0, 0},
};
NJS_TEXLIST fish_kaji_texlist = {
#include "stg01/fish_models/fish_kaji_texlist.inc"
};
static NJS_MATERIAL fish_kaji_25_materials[1] = {
#include "stg01/fish_models/fish_kaji_25_materials.inc"
};
static Sint16 fish_kaji_25_meshes0[11] = {
#include "stg01/fish_models/fish_kaji_25_meshes0.inc"
};
static NJS_COLOR fish_kaji_25_uvs0[9] = {
#include "stg01/fish_models/fish_kaji_25_uvs0.inc"
};
static NJS_MESHSET fish_kaji_25_meshsets[1] = {
#include "stg01/fish_models/fish_kaji_25_meshsets.inc"
};
static NJS_POINT3 fish_kaji_25_points[6] = {
#include "stg01/fish_models/fish_kaji_25_points.inc"
};
static NJS_VECTOR fish_kaji_25_normals[6] = {
#include "stg01/fish_models/fish_kaji_25_normals.inc"
};
static NJS_MODEL fish_kaji_25_model = {
#include "stg01/fish_models/fish_kaji_25_model.inc"
};
static NJS_OBJECT fish_kaji_25 = {
#include "stg01/fish_models/fish_kaji_25.inc"
};
static NJS_OBJECT fish_kaji_24 = {
#include "stg01/fish_models/fish_kaji_24.inc"
};
static NJS_OBJECT fish_kaji_23 = {
#include "stg01/fish_models/fish_kaji_23.inc"
};
static NJS_OBJECT fish_kaji_22 = {
#include "stg01/fish_models/fish_kaji_22.inc"
};
static NJS_MATERIAL fish_kaji_21_materials[1] = {
#include "stg01/fish_models/fish_kaji_21_materials.inc"
};
static Sint16 fish_kaji_21_meshes0[11] = {
#include "stg01/fish_models/fish_kaji_21_meshes0.inc"
};
static NJS_COLOR fish_kaji_21_uvs0[9] = {
#include "stg01/fish_models/fish_kaji_21_uvs0.inc"
};
static NJS_MESHSET fish_kaji_21_meshsets[1] = {
#include "stg01/fish_models/fish_kaji_21_meshsets.inc"
};
static NJS_POINT3 fish_kaji_21_points[6] = {
#include "stg01/fish_models/fish_kaji_21_points.inc"
};
static NJS_VECTOR fish_kaji_21_normals[6] = {
#include "stg01/fish_models/fish_kaji_21_normals.inc"
};
static NJS_MODEL fish_kaji_21_model = {
#include "stg01/fish_models/fish_kaji_21_model.inc"
};
static NJS_OBJECT fish_kaji_21 = {
#include "stg01/fish_models/fish_kaji_21.inc"
};
static NJS_OBJECT fish_kaji_20 = {
#include "stg01/fish_models/fish_kaji_20.inc"
};
static NJS_OBJECT fish_kaji_19 = {
#include "stg01/fish_models/fish_kaji_19.inc"
};
static NJS_OBJECT fish_kaji_18 = {
#include "stg01/fish_models/fish_kaji_18.inc"
};
static NJS_MATERIAL fish_kaji_17_materials[3] = {
#include "stg01/fish_models/fish_kaji_17_materials.inc"
};
static Sint16 fish_kaji_17_meshes0[45] = {
#include "stg01/fish_models/fish_kaji_17_meshes0.inc"
};
static Sint16 fish_kaji_17_meshes1[16] = {
#include "stg01/fish_models/fish_kaji_17_meshes1.inc"
};
static Sint16 fish_kaji_17_meshes2[34] = {
#include "stg01/fish_models/fish_kaji_17_meshes2.inc"
};
static NJS_COLOR fish_kaji_17_uvs0[42] = {
#include "stg01/fish_models/fish_kaji_17_uvs0.inc"
};
static NJS_COLOR fish_kaji_17_uvs1[14] = {
#include "stg01/fish_models/fish_kaji_17_uvs1.inc"
};
static NJS_COLOR fish_kaji_17_uvs2[28] = {
#include "stg01/fish_models/fish_kaji_17_uvs2.inc"
};
static NJS_MESHSET fish_kaji_17_meshsets[3] = {
#include "stg01/fish_models/fish_kaji_17_meshsets.inc"
};
static NJS_POINT3 fish_kaji_17_points[46] = {
#include "stg01/fish_models/fish_kaji_17_points.inc"
};
static NJS_VECTOR fish_kaji_17_normals[46] = {
#include "stg01/fish_models/fish_kaji_17_normals.inc"
};
static NJS_MODEL fish_kaji_17_model = {
#include "stg01/fish_models/fish_kaji_17_model.inc"
};
static NJS_OBJECT fish_kaji_17 = {
#include "stg01/fish_models/fish_kaji_17.inc"
};
static NJS_MATERIAL fish_kaji_16_materials[3] = {
#include "stg01/fish_models/fish_kaji_16_materials.inc"
};
static Sint16 fish_kaji_16_meshes0[31] = {
#include "stg01/fish_models/fish_kaji_16_meshes0.inc"
};
static Sint16 fish_kaji_16_meshes1[17] = {
#include "stg01/fish_models/fish_kaji_16_meshes1.inc"
};
static Sint16 fish_kaji_16_meshes2[10] = {
#include "stg01/fish_models/fish_kaji_16_meshes2.inc"
};
static NJS_COLOR fish_kaji_16_uvs0[26] = {
#include "stg01/fish_models/fish_kaji_16_uvs0.inc"
};
static NJS_COLOR fish_kaji_16_uvs1[14] = {
#include "stg01/fish_models/fish_kaji_16_uvs1.inc"
};
static NJS_COLOR fish_kaji_16_uvs2[8] = {
#include "stg01/fish_models/fish_kaji_16_uvs2.inc"
};
static NJS_MESHSET fish_kaji_16_meshsets[3] = {
#include "stg01/fish_models/fish_kaji_16_meshsets.inc"
};
static NJS_POINT3 fish_kaji_16_points[20] = {
#include "stg01/fish_models/fish_kaji_16_points.inc"
};
static NJS_VECTOR fish_kaji_16_normals[20] = {
#include "stg01/fish_models/fish_kaji_16_normals.inc"
};
static NJS_MODEL fish_kaji_16_model = {
#include "stg01/fish_models/fish_kaji_16_model.inc"
};
static NJS_OBJECT fish_kaji_16 = {
#include "stg01/fish_models/fish_kaji_16.inc"
};
static NJS_OBJECT fish_kaji_15 = {
#include "stg01/fish_models/fish_kaji_15.inc"
};
static NJS_OBJECT fish_kaji_14 = {
#include "stg01/fish_models/fish_kaji_14.inc"
};
static NJS_OBJECT fish_kaji_13 = {
#include "stg01/fish_models/fish_kaji_13.inc"
};
static NJS_MATERIAL fish_kaji_12_materials[4] = {
#include "stg01/fish_models/fish_kaji_12_materials.inc"
};
static Sint16 fish_kaji_12_meshes0[10] = {
#include "stg01/fish_models/fish_kaji_12_meshes0.inc"
};
static Sint16 fish_kaji_12_meshes1[44] = {
#include "stg01/fish_models/fish_kaji_12_meshes1.inc"
};
static Sint16 fish_kaji_12_meshes2[12] = {
#include "stg01/fish_models/fish_kaji_12_meshes2.inc"
};
static Sint16 fish_kaji_12_meshes3[10] = {
#include "stg01/fish_models/fish_kaji_12_meshes3.inc"
};
static NJS_COLOR fish_kaji_12_uvs0[8] = {
#include "stg01/fish_models/fish_kaji_12_uvs0.inc"
};
static NJS_COLOR fish_kaji_12_uvs1[37] = {
#include "stg01/fish_models/fish_kaji_12_uvs1.inc"
};
static NJS_COLOR fish_kaji_12_uvs2[10] = {
#include "stg01/fish_models/fish_kaji_12_uvs2.inc"
};
static NJS_COLOR fish_kaji_12_uvs3[8] = {
#include "stg01/fish_models/fish_kaji_12_uvs3.inc"
};
static NJS_MESHSET fish_kaji_12_meshsets[4] = {
#include "stg01/fish_models/fish_kaji_12_meshsets.inc"
};
static NJS_POINT3 fish_kaji_12_points[29] = {
#include "stg01/fish_models/fish_kaji_12_points.inc"
};
static NJS_VECTOR fish_kaji_12_normals[29] = {
#include "stg01/fish_models/fish_kaji_12_normals.inc"
};
static NJS_MODEL fish_kaji_12_model = {
#include "stg01/fish_models/fish_kaji_12_model.inc"
};
static NJS_OBJECT fish_kaji_12 = {
#include "stg01/fish_models/fish_kaji_12.inc"
};
static NJS_OBJECT fish_kaji_11 = {
#include "stg01/fish_models/fish_kaji_11.inc"
};
static NJS_OBJECT fish_kaji_10 = {
#include "stg01/fish_models/fish_kaji_10.inc"
};
static NJS_OBJECT fish_kaji_9 = {
#include "stg01/fish_models/fish_kaji_9.inc"
};
static NJS_MATERIAL fish_kaji_8_materials[4] = {
#include "stg01/fish_models/fish_kaji_8_materials.inc"
};
static Sint16 fish_kaji_8_meshes0[17] = {
#include "stg01/fish_models/fish_kaji_8_meshes0.inc"
};
static Sint16 fish_kaji_8_meshes1[27] = {
#include "stg01/fish_models/fish_kaji_8_meshes1.inc"
};
static Sint16 fish_kaji_8_meshes2[12] = {
#include "stg01/fish_models/fish_kaji_8_meshes2.inc"
};
static Sint16 fish_kaji_8_meshes3[7] = {
#include "stg01/fish_models/fish_kaji_8_meshes3.inc"
};
static NJS_COLOR fish_kaji_8_uvs0[14] = {
#include "stg01/fish_models/fish_kaji_8_uvs0.inc"
};
static NJS_COLOR fish_kaji_8_uvs1[26] = {
#include "stg01/fish_models/fish_kaji_8_uvs1.inc"
};
static NJS_COLOR fish_kaji_8_uvs2[10] = {
#include "stg01/fish_models/fish_kaji_8_uvs2.inc"
};
static NJS_COLOR fish_kaji_8_uvs3[6] = {
#include "stg01/fish_models/fish_kaji_8_uvs3.inc"
};
static NJS_MESHSET fish_kaji_8_meshsets[4] = {
#include "stg01/fish_models/fish_kaji_8_meshsets.inc"
};
static NJS_POINT3 fish_kaji_8_points[35] = {
#include "stg01/fish_models/fish_kaji_8_points.inc"
};
static NJS_VECTOR fish_kaji_8_normals[35] = {
#include "stg01/fish_models/fish_kaji_8_normals.inc"
};
static NJS_MODEL fish_kaji_8_model = {
#include "stg01/fish_models/fish_kaji_8_model.inc"
};
static NJS_OBJECT fish_kaji_8 = {
#include "stg01/fish_models/fish_kaji_8.inc"
};
static NJS_MATERIAL fish_kaji_7_materials[2] = {
#include "stg01/fish_models/fish_kaji_7_materials.inc"
};
static Sint16 fish_kaji_7_meshes0[15] = {
#include "stg01/fish_models/fish_kaji_7_meshes0.inc"
};
static Sint16 fish_kaji_7_meshes1[17] = {
#include "stg01/fish_models/fish_kaji_7_meshes1.inc"
};
static NJS_COLOR fish_kaji_7_uvs0[14] = {
#include "stg01/fish_models/fish_kaji_7_uvs0.inc"
};
static NJS_COLOR fish_kaji_7_uvs1[14] = {
#include "stg01/fish_models/fish_kaji_7_uvs1.inc"
};
static NJS_MESHSET fish_kaji_7_meshsets[2] = {
#include "stg01/fish_models/fish_kaji_7_meshsets.inc"
};
static NJS_POINT3 fish_kaji_7_points[18] = {
#include "stg01/fish_models/fish_kaji_7_points.inc"
};
static NJS_VECTOR fish_kaji_7_normals[18] = {
#include "stg01/fish_models/fish_kaji_7_normals.inc"
};
static NJS_MODEL fish_kaji_7_model = {
#include "stg01/fish_models/fish_kaji_7_model.inc"
};
static NJS_OBJECT fish_kaji_7 = {
#include "stg01/fish_models/fish_kaji_7.inc"
};
static NJS_MATERIAL fish_kaji_6_materials[2] = {
#include "stg01/fish_models/fish_kaji_6_materials.inc"
};
static Sint16 fish_kaji_6_meshes0[12] = {
#include "stg01/fish_models/fish_kaji_6_meshes0.inc"
};
static Sint16 fish_kaji_6_meshes1[16] = {
#include "stg01/fish_models/fish_kaji_6_meshes1.inc"
};
static NJS_COLOR fish_kaji_6_uvs0[10] = {
#include "stg01/fish_models/fish_kaji_6_uvs0.inc"
};
static NJS_COLOR fish_kaji_6_uvs1[14] = {
#include "stg01/fish_models/fish_kaji_6_uvs1.inc"
};
static NJS_MESHSET fish_kaji_6_meshsets[2] = {
#include "stg01/fish_models/fish_kaji_6_meshsets.inc"
};
static NJS_POINT3 fish_kaji_6_points[14] = {
#include "stg01/fish_models/fish_kaji_6_points.inc"
};
static NJS_VECTOR fish_kaji_6_normals[14] = {
#include "stg01/fish_models/fish_kaji_6_normals.inc"
};
static NJS_MODEL fish_kaji_6_model = {
#include "stg01/fish_models/fish_kaji_6_model.inc"
};
static NJS_OBJECT fish_kaji_6 = {
#include "stg01/fish_models/fish_kaji_6.inc"
};
static NJS_OBJECT fish_kaji_5 = {
#include "stg01/fish_models/fish_kaji_5.inc"
};
static NJS_OBJECT fish_kaji_4 = {
#include "stg01/fish_models/fish_kaji_4.inc"
};
static NJS_OBJECT fish_kaji_3 = {
#include "stg01/fish_models/fish_kaji_3.inc"
};
static NJS_OBJECT fish_kaji_2 = {
#include "stg01/fish_models/fish_kaji_2.inc"
};
static NJS_OBJECT fish_kaji_1 = {
#include "stg01/fish_models/fish_kaji_1.inc"
};
NJS_OBJECT fish_kaji_object = {
#include "stg01/fish_models/fish_kaji_object.inc"
};

static NJS_TEXNAME fish_an_texnames[7] = {
    {TEXT_43, 0, 0}, {TEXT_44, 0, 0}, {TEXT_45, 0, 0}, {TEXT_46, 0, 0},
    {TEXT_47, 0, 0}, {TEXT_48, 0, 0}, {TEXT_6, 0, 0},
};
NJS_TEXLIST fish_an_texlist = {
#include "stg01/fish_models/fish_an_texlist.inc"
};
static NJS_MATERIAL fish_an_10_materials[3] = {
#include "stg01/fish_models/fish_an_10_materials.inc"
};
static Sint16 fish_an_10_meshes0[24] = {
#include "stg01/fish_models/fish_an_10_meshes0.inc"
};
static Sint16 fish_an_10_meshes1[5] = {
#include "stg01/fish_models/fish_an_10_meshes1.inc"
};
static Sint16 fish_an_10_meshes2[12] = {
#include "stg01/fish_models/fish_an_10_meshes2.inc"
};
static NJS_COLOR fish_an_10_uvs0[20] = {
#include "stg01/fish_models/fish_an_10_uvs0.inc"
};
static NJS_COLOR fish_an_10_uvs1[4] = {
#include "stg01/fish_models/fish_an_10_uvs1.inc"
};
static NJS_COLOR fish_an_10_uvs2[10] = {
#include "stg01/fish_models/fish_an_10_uvs2.inc"
};
static NJS_MESHSET fish_an_10_meshsets[3] = {
#include "stg01/fish_models/fish_an_10_meshsets.inc"
};
static NJS_POINT3 fish_an_10_points[17] = {
#include "stg01/fish_models/fish_an_10_points.inc"
};
static NJS_VECTOR fish_an_10_normals[17] = {
#include "stg01/fish_models/fish_an_10_normals.inc"
};
static NJS_MODEL fish_an_10_model = {
#include "stg01/fish_models/fish_an_10_model.inc"
};
static NJS_OBJECT fish_an_10 = {
#include "stg01/fish_models/fish_an_10.inc"
};
static NJS_MATERIAL fish_an_9_materials[3] = {
#include "stg01/fish_models/fish_an_9_materials.inc"
};
static Sint16 fish_an_9_meshes0[24] = {
#include "stg01/fish_models/fish_an_9_meshes0.inc"
};
static Sint16 fish_an_9_meshes1[5] = {
#include "stg01/fish_models/fish_an_9_meshes1.inc"
};
static Sint16 fish_an_9_meshes2[12] = {
#include "stg01/fish_models/fish_an_9_meshes2.inc"
};
static NJS_COLOR fish_an_9_uvs0[20] = {
#include "stg01/fish_models/fish_an_9_uvs0.inc"
};
static NJS_COLOR fish_an_9_uvs1[4] = {
#include "stg01/fish_models/fish_an_9_uvs1.inc"
};
static NJS_COLOR fish_an_9_uvs2[10] = {
#include "stg01/fish_models/fish_an_9_uvs2.inc"
};
static NJS_MESHSET fish_an_9_meshsets[3] = {
#include "stg01/fish_models/fish_an_9_meshsets.inc"
};
static NJS_POINT3 fish_an_9_points[17] = {
#include "stg01/fish_models/fish_an_9_points.inc"
};
static NJS_VECTOR fish_an_9_normals[17] = {
#include "stg01/fish_models/fish_an_9_normals.inc"
};
static NJS_MODEL fish_an_9_model = {
#include "stg01/fish_models/fish_an_9_model.inc"
};
static NJS_OBJECT fish_an_9 = {
#include "stg01/fish_models/fish_an_9.inc"
};
static NJS_MATERIAL fish_an_8_materials[4] = {
#include "stg01/fish_models/fish_an_8_materials.inc"
};
static Sint16 fish_an_8_meshes0[58] = {
#include "stg01/fish_models/fish_an_8_meshes0.inc"
};
static Sint16 fish_an_8_meshes1[20] = {
#include "stg01/fish_models/fish_an_8_meshes1.inc"
};
static Sint16 fish_an_8_meshes2[16] = {
#include "stg01/fish_models/fish_an_8_meshes2.inc"
};
static Sint16 fish_an_8_meshes3[11] = {
#include "stg01/fish_models/fish_an_8_meshes3.inc"
};
static NJS_COLOR fish_an_8_uvs0[48] = {
#include "stg01/fish_models/fish_an_8_uvs0.inc"
};
static NJS_COLOR fish_an_8_uvs1[18] = {
#include "stg01/fish_models/fish_an_8_uvs1.inc"
};
static NJS_COLOR fish_an_8_uvs2[14] = {
#include "stg01/fish_models/fish_an_8_uvs2.inc"
};
static NJS_COLOR fish_an_8_uvs3[10] = {
#include "stg01/fish_models/fish_an_8_uvs3.inc"
};
static NJS_MESHSET fish_an_8_meshsets[4] = {
#include "stg01/fish_models/fish_an_8_meshsets.inc"
};
static NJS_POINT3 fish_an_8_points[43] = {
#include "stg01/fish_models/fish_an_8_points.inc"
};
static NJS_VECTOR fish_an_8_normals[43] = {
#include "stg01/fish_models/fish_an_8_normals.inc"
};
static NJS_MODEL fish_an_8_model = {
#include "stg01/fish_models/fish_an_8_model.inc"
};
static NJS_OBJECT fish_an_8 = {
#include "stg01/fish_models/fish_an_8.inc"
};
static NJS_MATERIAL fish_an_7_materials[6] = {
#include "stg01/fish_models/fish_an_7_materials.inc"
};
static Sint16 fish_an_7_meshes0[11] = {
#include "stg01/fish_models/fish_an_7_meshes0.inc"
};
static Sint16 fish_an_7_meshes1[96] = {
#include "stg01/fish_models/fish_an_7_meshes1.inc"
};
static Sint16 fish_an_7_meshes2[18] = {
#include "stg01/fish_models/fish_an_7_meshes2.inc"
};
static Sint16 fish_an_7_meshes3[20] = {
#include "stg01/fish_models/fish_an_7_meshes3.inc"
};
static Sint16 fish_an_7_meshes4[9] = {
#include "stg01/fish_models/fish_an_7_meshes4.inc"
};
static Sint16 fish_an_7_meshes5[8] = {
#include "stg01/fish_models/fish_an_7_meshes5.inc"
};
static NJS_COLOR fish_an_7_uvs0[10] = {
#include "stg01/fish_models/fish_an_7_uvs0.inc"
};
static NJS_COLOR fish_an_7_uvs1[83] = {
#include "stg01/fish_models/fish_an_7_uvs1.inc"
};
static NJS_COLOR fish_an_7_uvs2[16] = {
#include "stg01/fish_models/fish_an_7_uvs2.inc"
};
static NJS_COLOR fish_an_7_uvs3[16] = {
#include "stg01/fish_models/fish_an_7_uvs3.inc"
};
static NJS_COLOR fish_an_7_uvs4[8] = {
#include "stg01/fish_models/fish_an_7_uvs4.inc"
};
static NJS_COLOR fish_an_7_uvs5[7] = {
#include "stg01/fish_models/fish_an_7_uvs5.inc"
};
static NJS_MESHSET fish_an_7_meshsets[6] = {
#include "stg01/fish_models/fish_an_7_meshsets.inc"
};
static NJS_POINT3 fish_an_7_points[69] = {
#include "stg01/fish_models/fish_an_7_points.inc"
};
static NJS_VECTOR fish_an_7_normals[69] = {
#include "stg01/fish_models/fish_an_7_normals.inc"
};
static NJS_MODEL fish_an_7_model = {
#include "stg01/fish_models/fish_an_7_model.inc"
};
static NJS_OBJECT fish_an_7 = {
#include "stg01/fish_models/fish_an_7.inc"
};
static NJS_MATERIAL fish_an_6_materials[4] = {
#include "stg01/fish_models/fish_an_6_materials.inc"
};
static Sint16 fish_an_6_meshes0[19] = {
#include "stg01/fish_models/fish_an_6_meshes0.inc"
};
static Sint16 fish_an_6_meshes1[124] = {
#include "stg01/fish_models/fish_an_6_meshes1.inc"
};
static Sint16 fish_an_6_meshes2[26] = {
#include "stg01/fish_models/fish_an_6_meshes2.inc"
};
static Sint16 fish_an_6_meshes3[19] = {
#include "stg01/fish_models/fish_an_6_meshes3.inc"
};
static NJS_COLOR fish_an_6_uvs0[16] = {
#include "stg01/fish_models/fish_an_6_uvs0.inc"
};
static NJS_COLOR fish_an_6_uvs1[106] = {
#include "stg01/fish_models/fish_an_6_uvs1.inc"
};
static NJS_COLOR fish_an_6_uvs2[22] = {
#include "stg01/fish_models/fish_an_6_uvs2.inc"
};
static NJS_COLOR fish_an_6_uvs3[16] = {
#include "stg01/fish_models/fish_an_6_uvs3.inc"
};
static NJS_MESHSET fish_an_6_meshsets[4] = {
#include "stg01/fish_models/fish_an_6_meshsets.inc"
};
static NJS_POINT3 fish_an_6_points[66] = {
#include "stg01/fish_models/fish_an_6_points.inc"
};
static NJS_VECTOR fish_an_6_normals[66] = {
#include "stg01/fish_models/fish_an_6_normals.inc"
};
static NJS_MODEL fish_an_6_model = {
#include "stg01/fish_models/fish_an_6_model.inc"
};
static NJS_OBJECT fish_an_6 = {
#include "stg01/fish_models/fish_an_6.inc"
};
static NJS_MATERIAL fish_an_5_materials[2] = {
#include "stg01/fish_models/fish_an_5_materials.inc"
};
static Sint16 fish_an_5_meshes0[15] = {
#include "stg01/fish_models/fish_an_5_meshes0.inc"
};
static Sint16 fish_an_5_meshes1[22] = {
#include "stg01/fish_models/fish_an_5_meshes1.inc"
};
static NJS_COLOR fish_an_5_uvs0[14] = {
#include "stg01/fish_models/fish_an_5_uvs0.inc"
};
static NJS_COLOR fish_an_5_uvs1[18] = {
#include "stg01/fish_models/fish_an_5_uvs1.inc"
};
static NJS_MESHSET fish_an_5_meshsets[2] = {
#include "stg01/fish_models/fish_an_5_meshsets.inc"
};
static NJS_POINT3 fish_an_5_points[20] = {
#include "stg01/fish_models/fish_an_5_points.inc"
};
static NJS_VECTOR fish_an_5_normals[20] = {
#include "stg01/fish_models/fish_an_5_normals.inc"
};
static NJS_MODEL fish_an_5_model = {
#include "stg01/fish_models/fish_an_5_model.inc"
};
static NJS_OBJECT fish_an_5 = {
#include "stg01/fish_models/fish_an_5.inc"
};
static NJS_OBJECT fish_an_4 = {
#include "stg01/fish_models/fish_an_4.inc"
};
static NJS_OBJECT fish_an_3 = {
#include "stg01/fish_models/fish_an_3.inc"
};
static NJS_OBJECT fish_an_2 = {
#include "stg01/fish_models/fish_an_2.inc"
};
static NJS_OBJECT fish_an_1 = {
#include "stg01/fish_models/fish_an_1.inc"
};
NJS_OBJECT fish_an_object = {
#include "stg01/fish_models/fish_an_object.inc"
};

static NJS_TEXNAME fish_an_ke_texnames[2] = {
    {TEXT_43, 0, 0},
    {TEXT_49, 0, 0},
};
NJS_TEXLIST fish_an_ke_texlist = {
#include "stg01/fish_models/fish_an_ke_texlist.inc"
};
static NJS_MATERIAL fish_an_ke_4_materials[1] = {
#include "stg01/fish_models/fish_an_ke_4_materials.inc"
};
static Sint16 fish_an_ke_4_meshes0[28] = {
#include "stg01/fish_models/fish_an_ke_4_meshes0.inc"
};
static NJS_COLOR fish_an_ke_4_uvs0[24] = {
#include "stg01/fish_models/fish_an_ke_4_uvs0.inc"
};
static NJS_MESHSET fish_an_ke_4_meshsets[1] = {
#include "stg01/fish_models/fish_an_ke_4_meshsets.inc"
};
static NJS_POINT3 fish_an_ke_4_points[24] = {
#include "stg01/fish_models/fish_an_ke_4_points.inc"
};
static NJS_VECTOR fish_an_ke_4_normals[24] = {
#include "stg01/fish_models/fish_an_ke_4_normals.inc"
};
static NJS_MODEL fish_an_ke_4_model = {
#include "stg01/fish_models/fish_an_ke_4_model.inc"
};
static NJS_OBJECT fish_an_ke_4 = {
#include "stg01/fish_models/fish_an_ke_4.inc"
};
static NJS_MATERIAL fish_an_ke_3_materials[1] = {
#include "stg01/fish_models/fish_an_ke_3_materials.inc"
};
static Sint16 fish_an_ke_3_meshes0[27] = {
#include "stg01/fish_models/fish_an_ke_3_meshes0.inc"
};
static NJS_COLOR fish_an_ke_3_uvs0[24] = {
#include "stg01/fish_models/fish_an_ke_3_uvs0.inc"
};
static NJS_MESHSET fish_an_ke_3_meshsets[1] = {
#include "stg01/fish_models/fish_an_ke_3_meshsets.inc"
};
static NJS_POINT3 fish_an_ke_3_points[12] = {
#include "stg01/fish_models/fish_an_ke_3_points.inc"
};
static NJS_VECTOR fish_an_ke_3_normals[12] = {
#include "stg01/fish_models/fish_an_ke_3_normals.inc"
};
static NJS_MODEL fish_an_ke_3_model = {
#include "stg01/fish_models/fish_an_ke_3_model.inc"
};
static NJS_OBJECT fish_an_ke_3 = {
#include "stg01/fish_models/fish_an_ke_3.inc"
};
static NJS_MATERIAL fish_an_ke_2_materials[1] = {
#include "stg01/fish_models/fish_an_ke_2_materials.inc"
};
static Sint16 fish_an_ke_2_meshes0[9] = {
#include "stg01/fish_models/fish_an_ke_2_meshes0.inc"
};
static NJS_COLOR fish_an_ke_2_uvs0[8] = {
#include "stg01/fish_models/fish_an_ke_2_uvs0.inc"
};
static NJS_MESHSET fish_an_ke_2_meshsets[1] = {
#include "stg01/fish_models/fish_an_ke_2_meshsets.inc"
};
static NJS_POINT3 fish_an_ke_2_points[6] = {
#include "stg01/fish_models/fish_an_ke_2_points.inc"
};
static NJS_VECTOR fish_an_ke_2_normals[6] = {
#include "stg01/fish_models/fish_an_ke_2_normals.inc"
};
static NJS_MODEL fish_an_ke_2_model = {
#include "stg01/fish_models/fish_an_ke_2_model.inc"
};
static NJS_OBJECT fish_an_ke_2 = {
#include "stg01/fish_models/fish_an_ke_2.inc"
};
static NJS_MATERIAL fish_an_ke_1_materials[1] = {
#include "stg01/fish_models/fish_an_ke_1_materials.inc"
};
static Sint16 fish_an_ke_1_meshes0[9] = {
#include "stg01/fish_models/fish_an_ke_1_meshes0.inc"
};
static NJS_COLOR fish_an_ke_1_uvs0[8] = {
#include "stg01/fish_models/fish_an_ke_1_uvs0.inc"
};
static NJS_MESHSET fish_an_ke_1_meshsets[1] = {
#include "stg01/fish_models/fish_an_ke_1_meshsets.inc"
};
static NJS_POINT3 fish_an_ke_1_points[6] = {
#include "stg01/fish_models/fish_an_ke_1_points.inc"
};
static NJS_VECTOR fish_an_ke_1_normals[6] = {
#include "stg01/fish_models/fish_an_ke_1_normals.inc"
};
static NJS_MODEL fish_an_ke_1_model = {
#include "stg01/fish_models/fish_an_ke_1_model.inc"
};
static NJS_OBJECT fish_an_ke_1 = {
#include "stg01/fish_models/fish_an_ke_1.inc"
};
static NJS_OBJECT fish_an_ke_object = {
#include "stg01/fish_models/fish_an_ke_object.inc"
};

static NJS_TEXNAME fish_hh_texnames[7] = {
    {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0}, {TEXT_53, 0, 0},
    {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_11, 0, 0},
};
NJS_TEXLIST fish_hh_texlist = {
#include "stg01/fish_models/fish_hh_texlist.inc"
};
static NJS_MATERIAL fish_hh_10_materials[1] = {
#include "stg01/fish_models/fish_hh_10_materials.inc"
};
static Sint16 fish_hh_10_meshes0[21] = {
#include "stg01/fish_models/fish_hh_10_meshes0.inc"
};
static NJS_COLOR fish_hh_10_uvs0[18] = {
#include "stg01/fish_models/fish_hh_10_uvs0.inc"
};
static NJS_MESHSET fish_hh_10_meshsets[1] = {
#include "stg01/fish_models/fish_hh_10_meshsets.inc"
};
static NJS_POINT3 fish_hh_10_points[9] = {
#include "stg01/fish_models/fish_hh_10_points.inc"
};
static NJS_VECTOR fish_hh_10_normals[9] = {
#include "stg01/fish_models/fish_hh_10_normals.inc"
};
static NJS_MODEL fish_hh_10_model = {
#include "stg01/fish_models/fish_hh_10_model.inc"
};
static NJS_OBJECT fish_hh_10 = {
#include "stg01/fish_models/fish_hh_10.inc"
};
static NJS_MATERIAL fish_hh_9_materials[1] = {
#include "stg01/fish_models/fish_hh_9_materials.inc"
};
static Sint16 fish_hh_9_meshes0[21] = {
#include "stg01/fish_models/fish_hh_9_meshes0.inc"
};
static NJS_COLOR fish_hh_9_uvs0[18] = {
#include "stg01/fish_models/fish_hh_9_uvs0.inc"
};
static NJS_MESHSET fish_hh_9_meshsets[1] = {
#include "stg01/fish_models/fish_hh_9_meshsets.inc"
};
static NJS_POINT3 fish_hh_9_points[9] = {
#include "stg01/fish_models/fish_hh_9_points.inc"
};
static NJS_VECTOR fish_hh_9_normals[9] = {
#include "stg01/fish_models/fish_hh_9_normals.inc"
};
static NJS_MODEL fish_hh_9_model = {
#include "stg01/fish_models/fish_hh_9_model.inc"
};
static NJS_OBJECT fish_hh_9 = {
#include "stg01/fish_models/fish_hh_9.inc"
};
static NJS_MATERIAL fish_hh_8_materials[3] = {
#include "stg01/fish_models/fish_hh_8_materials.inc"
};
static Sint16 fish_hh_8_meshes0[14] = {
#include "stg01/fish_models/fish_hh_8_meshes0.inc"
};
static Sint16 fish_hh_8_meshes1[11] = {
#include "stg01/fish_models/fish_hh_8_meshes1.inc"
};
static Sint16 fish_hh_8_meshes2[50] = {
#include "stg01/fish_models/fish_hh_8_meshes2.inc"
};
static NJS_COLOR fish_hh_8_uvs0[12] = {
#include "stg01/fish_models/fish_hh_8_uvs0.inc"
};
static NJS_COLOR fish_hh_8_uvs1[9] = {
#include "stg01/fish_models/fish_hh_8_uvs1.inc"
};
static NJS_COLOR fish_hh_8_uvs2[40] = {
#include "stg01/fish_models/fish_hh_8_uvs2.inc"
};
static NJS_MESHSET fish_hh_8_meshsets[3] = {
#include "stg01/fish_models/fish_hh_8_meshsets.inc"
};
static NJS_POINT3 fish_hh_8_points[49] = {
#include "stg01/fish_models/fish_hh_8_points.inc"
};
static NJS_VECTOR fish_hh_8_normals[49] = {
#include "stg01/fish_models/fish_hh_8_normals.inc"
};
static NJS_MODEL fish_hh_8_model = {
#include "stg01/fish_models/fish_hh_8_model.inc"
};
static NJS_OBJECT fish_hh_8 = {
#include "stg01/fish_models/fish_hh_8.inc"
};
static NJS_MATERIAL fish_hh_7_materials[6] = {
#include "stg01/fish_models/fish_hh_7_materials.inc"
};
static Sint16 fish_hh_7_meshes0[48] = {
#include "stg01/fish_models/fish_hh_7_meshes0.inc"
};
static Sint16 fish_hh_7_meshes1[86] = {
#include "stg01/fish_models/fish_hh_7_meshes1.inc"
};
static Sint16 fish_hh_7_meshes2[20] = {
#include "stg01/fish_models/fish_hh_7_meshes2.inc"
};
static Sint16 fish_hh_7_meshes3[41] = {
#include "stg01/fish_models/fish_hh_7_meshes3.inc"
};
static Sint16 fish_hh_7_meshes4[21] = {
#include "stg01/fish_models/fish_hh_7_meshes4.inc"
};
static Sint16 fish_hh_7_meshes5[11] = {
#include "stg01/fish_models/fish_hh_7_meshes5.inc"
};
static NJS_COLOR fish_hh_7_uvs0[38] = {
#include "stg01/fish_models/fish_hh_7_uvs0.inc"
};
static NJS_COLOR fish_hh_7_uvs1[74] = {
#include "stg01/fish_models/fish_hh_7_uvs1.inc"
};
static NJS_COLOR fish_hh_7_uvs2[16] = {
#include "stg01/fish_models/fish_hh_7_uvs2.inc"
};
static NJS_COLOR fish_hh_7_uvs3[36] = {
#include "stg01/fish_models/fish_hh_7_uvs3.inc"
};
static NJS_COLOR fish_hh_7_uvs4[18] = {
#include "stg01/fish_models/fish_hh_7_uvs4.inc"
};
static NJS_COLOR fish_hh_7_uvs5[9] = {
#include "stg01/fish_models/fish_hh_7_uvs5.inc"
};
static NJS_MESHSET fish_hh_7_meshsets[6] = {
#include "stg01/fish_models/fish_hh_7_meshsets.inc"
};
static NJS_POINT3 fish_hh_7_points[87] = {
#include "stg01/fish_models/fish_hh_7_points.inc"
};
static NJS_VECTOR fish_hh_7_normals[87] = {
#include "stg01/fish_models/fish_hh_7_normals.inc"
};
static NJS_MODEL fish_hh_7_model = {
#include "stg01/fish_models/fish_hh_7_model.inc"
};
static NJS_OBJECT fish_hh_7 = {
#include "stg01/fish_models/fish_hh_7.inc"
};
static NJS_MATERIAL fish_hh_6_materials[2] = {
#include "stg01/fish_models/fish_hh_6_materials.inc"
};
static Sint16 fish_hh_6_meshes0[26] = {
#include "stg01/fish_models/fish_hh_6_meshes0.inc"
};
static Sint16 fish_hh_6_meshes1[18] = {
#include "stg01/fish_models/fish_hh_6_meshes1.inc"
};
static NJS_COLOR fish_hh_6_uvs0[22] = {
#include "stg01/fish_models/fish_hh_6_uvs0.inc"
};
static NJS_COLOR fish_hh_6_uvs1[16] = {
#include "stg01/fish_models/fish_hh_6_uvs1.inc"
};
static NJS_MESHSET fish_hh_6_meshsets[2] = {
#include "stg01/fish_models/fish_hh_6_meshsets.inc"
};
static NJS_POINT3 fish_hh_6_points[22] = {
#include "stg01/fish_models/fish_hh_6_points.inc"
};
static NJS_VECTOR fish_hh_6_normals[22] = {
#include "stg01/fish_models/fish_hh_6_normals.inc"
};
static NJS_MODEL fish_hh_6_model = {
#include "stg01/fish_models/fish_hh_6_model.inc"
};
static NJS_OBJECT fish_hh_6 = {
#include "stg01/fish_models/fish_hh_6.inc"
};
static NJS_MATERIAL fish_hh_5_materials[4] = {
#include "stg01/fish_models/fish_hh_5_materials.inc"
};
static Sint16 fish_hh_5_meshes0[5] = {
#include "stg01/fish_models/fish_hh_5_meshes0.inc"
};
static Sint16 fish_hh_5_meshes1[15] = {
#include "stg01/fish_models/fish_hh_5_meshes1.inc"
};
static Sint16 fish_hh_5_meshes2[36] = {
#include "stg01/fish_models/fish_hh_5_meshes2.inc"
};
static Sint16 fish_hh_5_meshes3[14] = {
#include "stg01/fish_models/fish_hh_5_meshes3.inc"
};
static NJS_COLOR fish_hh_5_uvs0[4] = {
#include "stg01/fish_models/fish_hh_5_uvs0.inc"
};
static NJS_COLOR fish_hh_5_uvs1[12] = {
#include "stg01/fish_models/fish_hh_5_uvs1.inc"
};
static NJS_COLOR fish_hh_5_uvs2[30] = {
#include "stg01/fish_models/fish_hh_5_uvs2.inc"
};
static NJS_COLOR fish_hh_5_uvs3[12] = {
#include "stg01/fish_models/fish_hh_5_uvs3.inc"
};
static NJS_MESHSET fish_hh_5_meshsets[4] = {
#include "stg01/fish_models/fish_hh_5_meshsets.inc"
};
static NJS_POINT3 fish_hh_5_points[28] = {
#include "stg01/fish_models/fish_hh_5_points.inc"
};
static NJS_VECTOR fish_hh_5_normals[28] = {
#include "stg01/fish_models/fish_hh_5_normals.inc"
};
static NJS_MODEL fish_hh_5_model = {
#include "stg01/fish_models/fish_hh_5_model.inc"
};
static NJS_OBJECT fish_hh_5 = {
#include "stg01/fish_models/fish_hh_5.inc"
};
static NJS_OBJECT fish_hh_4 = {
#include "stg01/fish_models/fish_hh_4.inc"
};
static NJS_OBJECT fish_hh_3 = {
#include "stg01/fish_models/fish_hh_3.inc"
};
static NJS_OBJECT fish_hh_2 = {
#include "stg01/fish_models/fish_hh_2.inc"
};
static NJS_OBJECT fish_hh_1 = {
#include "stg01/fish_models/fish_hh_1.inc"
};
NJS_OBJECT fish_hh_object = {
#include "stg01/fish_models/fish_hh_object.inc"
};

static NJS_TEXNAME fish_ms_texnames[12] = {
    {TEXT_56, 0, 0}, {TEXT_57, 0, 0}, {TEXT_53, 0, 0}, {TEXT_58, 0, 0},
    {TEXT_59, 0, 0}, {TEXT_60, 0, 0}, {TEXT_61, 0, 0}, {TEXT_62, 0, 0},
    {TEXT_63, 0, 0}, {TEXT_64, 0, 0}, {TEXT_11, 0, 0}, {TEXT_65, 0, 0},
};
NJS_TEXLIST fish_ms_texlist = {
#include "stg01/fish_models/fish_ms_texlist.inc"
};
static NJS_MATERIAL fish_ms_10_materials[3] = {
#include "stg01/fish_models/fish_ms_10_materials.inc"
};
static Sint16 fish_ms_10_meshes0[24] = {
#include "stg01/fish_models/fish_ms_10_meshes0.inc"
};
static Sint16 fish_ms_10_meshes1[10] = {
#include "stg01/fish_models/fish_ms_10_meshes1.inc"
};
static Sint16 fish_ms_10_meshes2[5] = {
#include "stg01/fish_models/fish_ms_10_meshes2.inc"
};
static NJS_COLOR fish_ms_10_uvs0[20] = {
#include "stg01/fish_models/fish_ms_10_uvs0.inc"
};
static NJS_COLOR fish_ms_10_uvs1[8] = {
#include "stg01/fish_models/fish_ms_10_uvs1.inc"
};
static NJS_COLOR fish_ms_10_uvs2[4] = {
#include "stg01/fish_models/fish_ms_10_uvs2.inc"
};
static NJS_MESHSET fish_ms_10_meshsets[3] = {
#include "stg01/fish_models/fish_ms_10_meshsets.inc"
};
static NJS_POINT3 fish_ms_10_points[12] = {
#include "stg01/fish_models/fish_ms_10_points.inc"
};
static NJS_VECTOR fish_ms_10_normals[12] = {
#include "stg01/fish_models/fish_ms_10_normals.inc"
};
static NJS_MODEL fish_ms_10_model = {
#include "stg01/fish_models/fish_ms_10_model.inc"
};
static NJS_OBJECT fish_ms_10 = {
#include "stg01/fish_models/fish_ms_10.inc"
};
static NJS_MATERIAL fish_ms_9_materials[3] = {
#include "stg01/fish_models/fish_ms_9_materials.inc"
};
static Sint16 fish_ms_9_meshes0[21] = {
#include "stg01/fish_models/fish_ms_9_meshes0.inc"
};
static Sint16 fish_ms_9_meshes1[10] = {
#include "stg01/fish_models/fish_ms_9_meshes1.inc"
};
static Sint16 fish_ms_9_meshes2[5] = {
#include "stg01/fish_models/fish_ms_9_meshes2.inc"
};
static NJS_COLOR fish_ms_9_uvs0[18] = {
#include "stg01/fish_models/fish_ms_9_uvs0.inc"
};
static NJS_COLOR fish_ms_9_uvs1[8] = {
#include "stg01/fish_models/fish_ms_9_uvs1.inc"
};
static NJS_COLOR fish_ms_9_uvs2[4] = {
#include "stg01/fish_models/fish_ms_9_uvs2.inc"
};
static NJS_MESHSET fish_ms_9_meshsets[3] = {
#include "stg01/fish_models/fish_ms_9_meshsets.inc"
};
static NJS_POINT3 fish_ms_9_points[12] = {
#include "stg01/fish_models/fish_ms_9_points.inc"
};
static NJS_VECTOR fish_ms_9_normals[12] = {
#include "stg01/fish_models/fish_ms_9_normals.inc"
};
static NJS_MODEL fish_ms_9_model = {
#include "stg01/fish_models/fish_ms_9_model.inc"
};
static NJS_OBJECT fish_ms_9 = {
#include "stg01/fish_models/fish_ms_9.inc"
};
static NJS_MATERIAL fish_ms_8_materials[5] = {
#include "stg01/fish_models/fish_ms_8_materials.inc"
};
static Sint16 fish_ms_8_meshes0[20] = {
#include "stg01/fish_models/fish_ms_8_meshes0.inc"
};
static Sint16 fish_ms_8_meshes1[20] = {
#include "stg01/fish_models/fish_ms_8_meshes1.inc"
};
static Sint16 fish_ms_8_meshes2[60] = {
#include "stg01/fish_models/fish_ms_8_meshes2.inc"
};
static Sint16 fish_ms_8_meshes3[5] = {
#include "stg01/fish_models/fish_ms_8_meshes3.inc"
};
static Sint16 fish_ms_8_meshes4[11] = {
#include "stg01/fish_models/fish_ms_8_meshes4.inc"
};
static NJS_COLOR fish_ms_8_uvs0[16] = {
#include "stg01/fish_models/fish_ms_8_uvs0.inc"
};
static NJS_COLOR fish_ms_8_uvs1[15] = {
#include "stg01/fish_models/fish_ms_8_uvs1.inc"
};
static NJS_COLOR fish_ms_8_uvs2[52] = {
#include "stg01/fish_models/fish_ms_8_uvs2.inc"
};
static NJS_COLOR fish_ms_8_uvs3[4] = {
#include "stg01/fish_models/fish_ms_8_uvs3.inc"
};
static NJS_COLOR fish_ms_8_uvs4[9] = {
#include "stg01/fish_models/fish_ms_8_uvs4.inc"
};
static NJS_MESHSET fish_ms_8_meshsets[5] = {
#include "stg01/fish_models/fish_ms_8_meshsets.inc"
};
static NJS_POINT3 fish_ms_8_points[47] = {
#include "stg01/fish_models/fish_ms_8_points.inc"
};
static NJS_VECTOR fish_ms_8_normals[47] = {
#include "stg01/fish_models/fish_ms_8_normals.inc"
};
static NJS_MODEL fish_ms_8_model = {
#include "stg01/fish_models/fish_ms_8_model.inc"
};
static NJS_OBJECT fish_ms_8 = {
#include "stg01/fish_models/fish_ms_8.inc"
};
static NJS_MATERIAL fish_ms_7_materials[5] = {
#include "stg01/fish_models/fish_ms_7_materials.inc"
};
static Sint16 fish_ms_7_meshes0[22] = {
#include "stg01/fish_models/fish_ms_7_meshes0.inc"
};
static Sint16 fish_ms_7_meshes1[64] = {
#include "stg01/fish_models/fish_ms_7_meshes1.inc"
};
static Sint16 fish_ms_7_meshes2[24] = {
#include "stg01/fish_models/fish_ms_7_meshes2.inc"
};
static Sint16 fish_ms_7_meshes3[10] = {
#include "stg01/fish_models/fish_ms_7_meshes3.inc"
};
static Sint16 fish_ms_7_meshes4[24] = {
#include "stg01/fish_models/fish_ms_7_meshes4.inc"
};
static NJS_COLOR fish_ms_7_uvs0[18] = {
#include "stg01/fish_models/fish_ms_7_uvs0.inc"
};
static NJS_COLOR fish_ms_7_uvs1[54] = {
#include "stg01/fish_models/fish_ms_7_uvs1.inc"
};
static NJS_COLOR fish_ms_7_uvs2[20] = {
#include "stg01/fish_models/fish_ms_7_uvs2.inc"
};
static NJS_COLOR fish_ms_7_uvs3[8] = {
#include "stg01/fish_models/fish_ms_7_uvs3.inc"
};
static NJS_COLOR fish_ms_7_uvs4[18] = {
#include "stg01/fish_models/fish_ms_7_uvs4.inc"
};
static NJS_MESHSET fish_ms_7_meshsets[5] = {
#include "stg01/fish_models/fish_ms_7_meshsets.inc"
};
static NJS_POINT3 fish_ms_7_points[60] = {
#include "stg01/fish_models/fish_ms_7_points.inc"
};
static NJS_VECTOR fish_ms_7_normals[60] = {
#include "stg01/fish_models/fish_ms_7_normals.inc"
};
static NJS_MODEL fish_ms_7_model = {
#include "stg01/fish_models/fish_ms_7_model.inc"
};
static NJS_OBJECT fish_ms_7 = {
#include "stg01/fish_models/fish_ms_7.inc"
};
static NJS_MATERIAL fish_ms_6_materials[6] = {
#include "stg01/fish_models/fish_ms_6_materials.inc"
};
static Sint16 fish_ms_6_meshes0[42] = {
#include "stg01/fish_models/fish_ms_6_meshes0.inc"
};
static Sint16 fish_ms_6_meshes1[6] = {
#include "stg01/fish_models/fish_ms_6_meshes1.inc"
};
static Sint16 fish_ms_6_meshes2[6] = {
#include "stg01/fish_models/fish_ms_6_meshes2.inc"
};
static Sint16 fish_ms_6_meshes3[68] = {
#include "stg01/fish_models/fish_ms_6_meshes3.inc"
};
static Sint16 fish_ms_6_meshes4[94] = {
#include "stg01/fish_models/fish_ms_6_meshes4.inc"
};
static Sint16 fish_ms_6_meshes5[20] = {
#include "stg01/fish_models/fish_ms_6_meshes5.inc"
};
static NJS_COLOR fish_ms_6_uvs0[36] = {
#include "stg01/fish_models/fish_ms_6_uvs0.inc"
};
static NJS_COLOR fish_ms_6_uvs1[5] = {
#include "stg01/fish_models/fish_ms_6_uvs1.inc"
};
static NJS_COLOR fish_ms_6_uvs2[5] = {
#include "stg01/fish_models/fish_ms_6_uvs2.inc"
};
static NJS_COLOR fish_ms_6_uvs3[58] = {
#include "stg01/fish_models/fish_ms_6_uvs3.inc"
};
static NJS_COLOR fish_ms_6_uvs4[84] = {
#include "stg01/fish_models/fish_ms_6_uvs4.inc"
};
static NJS_COLOR fish_ms_6_uvs5[16] = {
#include "stg01/fish_models/fish_ms_6_uvs5.inc"
};
static NJS_MESHSET fish_ms_6_meshsets[6] = {
#include "stg01/fish_models/fish_ms_6_meshsets.inc"
};
static NJS_POINT3 fish_ms_6_points[81] = {
#include "stg01/fish_models/fish_ms_6_points.inc"
};
static NJS_VECTOR fish_ms_6_normals[81] = {
#include "stg01/fish_models/fish_ms_6_normals.inc"
};
static NJS_MODEL fish_ms_6_model = {
#include "stg01/fish_models/fish_ms_6_model.inc"
};
static NJS_OBJECT fish_ms_6 = {
#include "stg01/fish_models/fish_ms_6.inc"
};
static NJS_MATERIAL fish_ms_5_materials[3] = {
#include "stg01/fish_models/fish_ms_5_materials.inc"
};
static Sint16 fish_ms_5_meshes0[5] = {
#include "stg01/fish_models/fish_ms_5_meshes0.inc"
};
static Sint16 fish_ms_5_meshes1[17] = {
#include "stg01/fish_models/fish_ms_5_meshes1.inc"
};
static Sint16 fish_ms_5_meshes2[14] = {
#include "stg01/fish_models/fish_ms_5_meshes2.inc"
};
static NJS_COLOR fish_ms_5_uvs0[4] = {
#include "stg01/fish_models/fish_ms_5_uvs0.inc"
};
static NJS_COLOR fish_ms_5_uvs1[14] = {
#include "stg01/fish_models/fish_ms_5_uvs1.inc"
};
static NJS_COLOR fish_ms_5_uvs2[12] = {
#include "stg01/fish_models/fish_ms_5_uvs2.inc"
};
static NJS_MESHSET fish_ms_5_meshsets[3] = {
#include "stg01/fish_models/fish_ms_5_meshsets.inc"
};
static NJS_POINT3 fish_ms_5_points[15] = {
#include "stg01/fish_models/fish_ms_5_points.inc"
};
static NJS_VECTOR fish_ms_5_normals[15] = {
#include "stg01/fish_models/fish_ms_5_normals.inc"
};
static NJS_MODEL fish_ms_5_model = {
#include "stg01/fish_models/fish_ms_5_model.inc"
};
static NJS_OBJECT fish_ms_5 = {
#include "stg01/fish_models/fish_ms_5.inc"
};
static NJS_OBJECT fish_ms_4 = {
#include "stg01/fish_models/fish_ms_4.inc"
};
static NJS_OBJECT fish_ms_3 = {
#include "stg01/fish_models/fish_ms_3.inc"
};
static NJS_OBJECT fish_ms_2 = {
#include "stg01/fish_models/fish_ms_2.inc"
};
static NJS_OBJECT fish_ms_1 = {
#include "stg01/fish_models/fish_ms_1.inc"
};
NJS_OBJECT fish_ms_object = {
#include "stg01/fish_models/fish_ms_object.inc"
};

static NJS_TEXNAME fish_pira_texnames[8] = {
    {TEXT_66, 0, 0}, {TEXT_67, 0, 0}, {TEXT_68, 0, 0}, {TEXT_69, 0, 0},
    {TEXT_70, 0, 0}, {TEXT_53, 0, 0}, {TEXT_11, 0, 0}, {TEXT_71, 0, 0},
};
NJS_TEXLIST fish_pira_texlist = {
#include "stg01/fish_models/fish_pira_texlist.inc"
};
static NJS_MATERIAL fish_pira_10_materials[1] = {
#include "stg01/fish_models/fish_pira_10_materials.inc"
};
static Sint16 fish_pira_10_meshes0[12] = {
#include "stg01/fish_models/fish_pira_10_meshes0.inc"
};
static NJS_COLOR fish_pira_10_uvs0[10] = {
#include "stg01/fish_models/fish_pira_10_uvs0.inc"
};
static NJS_MESHSET fish_pira_10_meshsets[1] = {
#include "stg01/fish_models/fish_pira_10_meshsets.inc"
};
static NJS_POINT3 fish_pira_10_points[6] = {
#include "stg01/fish_models/fish_pira_10_points.inc"
};
static NJS_VECTOR fish_pira_10_normals[6] = {
#include "stg01/fish_models/fish_pira_10_normals.inc"
};
static NJS_MODEL fish_pira_10_model = {
#include "stg01/fish_models/fish_pira_10_model.inc"
};
static NJS_OBJECT fish_pira_10 = {
#include "stg01/fish_models/fish_pira_10.inc"
};
static NJS_MATERIAL fish_pira_9_materials[1] = {
#include "stg01/fish_models/fish_pira_9_materials.inc"
};
static Sint16 fish_pira_9_meshes0[12] = {
#include "stg01/fish_models/fish_pira_9_meshes0.inc"
};
static NJS_COLOR fish_pira_9_uvs0[10] = {
#include "stg01/fish_models/fish_pira_9_uvs0.inc"
};
static NJS_MESHSET fish_pira_9_meshsets[1] = {
#include "stg01/fish_models/fish_pira_9_meshsets.inc"
};
static NJS_POINT3 fish_pira_9_points[6] = {
#include "stg01/fish_models/fish_pira_9_points.inc"
};
static NJS_VECTOR fish_pira_9_normals[6] = {
#include "stg01/fish_models/fish_pira_9_normals.inc"
};
static NJS_MODEL fish_pira_9_model = {
#include "stg01/fish_models/fish_pira_9_model.inc"
};
static NJS_OBJECT fish_pira_9 = {
#include "stg01/fish_models/fish_pira_9.inc"
};
static NJS_MATERIAL fish_pira_8_materials[4] = {
#include "stg01/fish_models/fish_pira_8_materials.inc"
};
static Sint16 fish_pira_8_meshes0[51] = {
#include "stg01/fish_models/fish_pira_8_meshes0.inc"
};
static Sint16 fish_pira_8_meshes1[13] = {
#include "stg01/fish_models/fish_pira_8_meshes1.inc"
};
static Sint16 fish_pira_8_meshes2[10] = {
#include "stg01/fish_models/fish_pira_8_meshes2.inc"
};
static Sint16 fish_pira_8_meshes3[40] = {
#include "stg01/fish_models/fish_pira_8_meshes3.inc"
};
static NJS_COLOR fish_pira_8_uvs0[44] = {
#include "stg01/fish_models/fish_pira_8_uvs0.inc"
};
static NJS_COLOR fish_pira_8_uvs1[11] = {
#include "stg01/fish_models/fish_pira_8_uvs1.inc"
};
static NJS_COLOR fish_pira_8_uvs2[8] = {
#include "stg01/fish_models/fish_pira_8_uvs2.inc"
};
static NJS_COLOR fish_pira_8_uvs3[32] = {
#include "stg01/fish_models/fish_pira_8_uvs3.inc"
};
static NJS_MESHSET fish_pira_8_meshsets[4] = {
#include "stg01/fish_models/fish_pira_8_meshsets.inc"
};
static NJS_POINT3 fish_pira_8_points[50] = {
#include "stg01/fish_models/fish_pira_8_points.inc"
};
static NJS_VECTOR fish_pira_8_normals[50] = {
#include "stg01/fish_models/fish_pira_8_normals.inc"
};
static NJS_MODEL fish_pira_8_model = {
#include "stg01/fish_models/fish_pira_8_model.inc"
};
static NJS_OBJECT fish_pira_8 = {
#include "stg01/fish_models/fish_pira_8.inc"
};
static NJS_MATERIAL fish_pira_7_materials[5] = {
#include "stg01/fish_models/fish_pira_7_materials.inc"
};
static Sint16 fish_pira_7_meshes0[53] = {
#include "stg01/fish_models/fish_pira_7_meshes0.inc"
};
static Sint16 fish_pira_7_meshes1[19] = {
#include "stg01/fish_models/fish_pira_7_meshes1.inc"
};
static Sint16 fish_pira_7_meshes2[42] = {
#include "stg01/fish_models/fish_pira_7_meshes2.inc"
};
static Sint16 fish_pira_7_meshes3[30] = {
#include "stg01/fish_models/fish_pira_7_meshes3.inc"
};
static Sint16 fish_pira_7_meshes4[16] = {
#include "stg01/fish_models/fish_pira_7_meshes4.inc"
};
static NJS_COLOR fish_pira_7_uvs0[46] = {
#include "stg01/fish_models/fish_pira_7_uvs0.inc"
};
static NJS_COLOR fish_pira_7_uvs1[16] = {
#include "stg01/fish_models/fish_pira_7_uvs1.inc"
};
static NJS_COLOR fish_pira_7_uvs2[36] = {
#include "stg01/fish_models/fish_pira_7_uvs2.inc"
};
static NJS_COLOR fish_pira_7_uvs3[24] = {
#include "stg01/fish_models/fish_pira_7_uvs3.inc"
};
static NJS_COLOR fish_pira_7_uvs4[13] = {
#include "stg01/fish_models/fish_pira_7_uvs4.inc"
};
static NJS_MESHSET fish_pira_7_meshsets[5] = {
#include "stg01/fish_models/fish_pira_7_meshsets.inc"
};
static NJS_POINT3 fish_pira_7_points[64] = {
#include "stg01/fish_models/fish_pira_7_points.inc"
};
static NJS_VECTOR fish_pira_7_normals[64] = {
#include "stg01/fish_models/fish_pira_7_normals.inc"
};
static NJS_MODEL fish_pira_7_model = {
#include "stg01/fish_models/fish_pira_7_model.inc"
};
static NJS_OBJECT fish_pira_7 = {
#include "stg01/fish_models/fish_pira_7.inc"
};
static NJS_MATERIAL fish_pira_6_materials[2] = {
#include "stg01/fish_models/fish_pira_6_materials.inc"
};
static Sint16 fish_pira_6_meshes0[21] = {
#include "stg01/fish_models/fish_pira_6_meshes0.inc"
};
static Sint16 fish_pira_6_meshes1[41] = {
#include "stg01/fish_models/fish_pira_6_meshes1.inc"
};
static NJS_COLOR fish_pira_6_uvs0[18] = {
#include "stg01/fish_models/fish_pira_6_uvs0.inc"
};
static NJS_COLOR fish_pira_6_uvs1[36] = {
#include "stg01/fish_models/fish_pira_6_uvs1.inc"
};
static NJS_MESHSET fish_pira_6_meshsets[2] = {
#include "stg01/fish_models/fish_pira_6_meshsets.inc"
};
static NJS_POINT3 fish_pira_6_points[27] = {
#include "stg01/fish_models/fish_pira_6_points.inc"
};
static NJS_VECTOR fish_pira_6_normals[27] = {
#include "stg01/fish_models/fish_pira_6_normals.inc"
};
static NJS_MODEL fish_pira_6_model = {
#include "stg01/fish_models/fish_pira_6_model.inc"
};
static NJS_OBJECT fish_pira_6 = {
#include "stg01/fish_models/fish_pira_6.inc"
};
static NJS_MATERIAL fish_pira_5_materials[2] = {
#include "stg01/fish_models/fish_pira_5_materials.inc"
};
static Sint16 fish_pira_5_meshes0[21] = {
#include "stg01/fish_models/fish_pira_5_meshes0.inc"
};
static Sint16 fish_pira_5_meshes1[21] = {
#include "stg01/fish_models/fish_pira_5_meshes1.inc"
};
static NJS_COLOR fish_pira_5_uvs0[18] = {
#include "stg01/fish_models/fish_pira_5_uvs0.inc"
};
static NJS_COLOR fish_pira_5_uvs1[18] = {
#include "stg01/fish_models/fish_pira_5_uvs1.inc"
};
static NJS_MESHSET fish_pira_5_meshsets[2] = {
#include "stg01/fish_models/fish_pira_5_meshsets.inc"
};
static NJS_POINT3 fish_pira_5_points[17] = {
#include "stg01/fish_models/fish_pira_5_points.inc"
};
static NJS_VECTOR fish_pira_5_normals[17] = {
#include "stg01/fish_models/fish_pira_5_normals.inc"
};
static NJS_MODEL fish_pira_5_model = {
#include "stg01/fish_models/fish_pira_5_model.inc"
};
static NJS_OBJECT fish_pira_5 = {
#include "stg01/fish_models/fish_pira_5.inc"
};
static NJS_OBJECT fish_pira_4 = {
#include "stg01/fish_models/fish_pira_4.inc"
};
static NJS_OBJECT fish_pira_3 = {
#include "stg01/fish_models/fish_pira_3.inc"
};
static NJS_OBJECT fish_pira_2 = {
#include "stg01/fish_models/fish_pira_2.inc"
};
static NJS_OBJECT fish_pira_1 = {
#include "stg01/fish_models/fish_pira_1.inc"
};
NJS_OBJECT fish_pira_object = {
#include "stg01/fish_models/fish_pira_object.inc"
};

static NJS_TEXNAME fish_same_texnames[7] = {
    {TEXT_72, 0, 0}, {TEXT_73, 0, 0}, {TEXT_74, 0, 0}, {TEXT_75, 0, 0},
    {TEXT_76, 0, 0}, {TEXT_11, 0, 0}, {TEXT_53, 0, 0},
};
NJS_TEXLIST fish_same_texlist = {
#include "stg01/fish_models/fish_same_texlist.inc"
};
static NJS_MATERIAL fish_same_10_materials[1] = {
#include "stg01/fish_models/fish_same_10_materials.inc"
};
static Sint16 fish_same_10_meshes0[21] = {
#include "stg01/fish_models/fish_same_10_meshes0.inc"
};
static NJS_COLOR fish_same_10_uvs0[18] = {
#include "stg01/fish_models/fish_same_10_uvs0.inc"
};
static NJS_MESHSET fish_same_10_meshsets[1] = {
#include "stg01/fish_models/fish_same_10_meshsets.inc"
};
static NJS_POINT3 fish_same_10_points[9] = {
#include "stg01/fish_models/fish_same_10_points.inc"
};
static NJS_VECTOR fish_same_10_normals[9] = {
#include "stg01/fish_models/fish_same_10_normals.inc"
};
static NJS_MODEL fish_same_10_model = {
#include "stg01/fish_models/fish_same_10_model.inc"
};
static NJS_OBJECT fish_same_10 = {
#include "stg01/fish_models/fish_same_10.inc"
};
static NJS_MATERIAL fish_same_9_materials[1] = {
#include "stg01/fish_models/fish_same_9_materials.inc"
};
static Sint16 fish_same_9_meshes0[21] = {
#include "stg01/fish_models/fish_same_9_meshes0.inc"
};
static NJS_COLOR fish_same_9_uvs0[18] = {
#include "stg01/fish_models/fish_same_9_uvs0.inc"
};
static NJS_MESHSET fish_same_9_meshsets[1] = {
#include "stg01/fish_models/fish_same_9_meshsets.inc"
};
static NJS_POINT3 fish_same_9_points[9] = {
#include "stg01/fish_models/fish_same_9_points.inc"
};
static NJS_VECTOR fish_same_9_normals[9] = {
#include "stg01/fish_models/fish_same_9_normals.inc"
};
static NJS_MODEL fish_same_9_model = {
#include "stg01/fish_models/fish_same_9_model.inc"
};
static NJS_OBJECT fish_same_9 = {
#include "stg01/fish_models/fish_same_9.inc"
};
static NJS_MATERIAL fish_same_8_materials[3] = {
#include "stg01/fish_models/fish_same_8_materials.inc"
};
static Sint16 fish_same_8_meshes0[17] = {
#include "stg01/fish_models/fish_same_8_meshes0.inc"
};
static Sint16 fish_same_8_meshes1[11] = {
#include "stg01/fish_models/fish_same_8_meshes1.inc"
};
static Sint16 fish_same_8_meshes2[50] = {
#include "stg01/fish_models/fish_same_8_meshes2.inc"
};
static NJS_COLOR fish_same_8_uvs0[14] = {
#include "stg01/fish_models/fish_same_8_uvs0.inc"
};
static NJS_COLOR fish_same_8_uvs1[9] = {
#include "stg01/fish_models/fish_same_8_uvs1.inc"
};
static NJS_COLOR fish_same_8_uvs2[40] = {
#include "stg01/fish_models/fish_same_8_uvs2.inc"
};
static NJS_MESHSET fish_same_8_meshsets[3] = {
#include "stg01/fish_models/fish_same_8_meshsets.inc"
};
static NJS_POINT3 fish_same_8_points[49] = {
#include "stg01/fish_models/fish_same_8_points.inc"
};
static NJS_VECTOR fish_same_8_normals[49] = {
#include "stg01/fish_models/fish_same_8_normals.inc"
};
static NJS_MODEL fish_same_8_model = {
#include "stg01/fish_models/fish_same_8_model.inc"
};
static NJS_OBJECT fish_same_8 = {
#include "stg01/fish_models/fish_same_8.inc"
};
static NJS_MATERIAL fish_same_7_materials[6] = {
#include "stg01/fish_models/fish_same_7_materials.inc"
};
static Sint16 fish_same_7_meshes0[48] = {
#include "stg01/fish_models/fish_same_7_meshes0.inc"
};
static Sint16 fish_same_7_meshes1[67] = {
#include "stg01/fish_models/fish_same_7_meshes1.inc"
};
static Sint16 fish_same_7_meshes2[10] = {
#include "stg01/fish_models/fish_same_7_meshes2.inc"
};
static Sint16 fish_same_7_meshes3[49] = {
#include "stg01/fish_models/fish_same_7_meshes3.inc"
};
static Sint16 fish_same_7_meshes4[21] = {
#include "stg01/fish_models/fish_same_7_meshes4.inc"
};
static Sint16 fish_same_7_meshes5[11] = {
#include "stg01/fish_models/fish_same_7_meshes5.inc"
};
static NJS_COLOR fish_same_7_uvs0[38] = {
#include "stg01/fish_models/fish_same_7_uvs0.inc"
};
static NJS_COLOR fish_same_7_uvs1[58] = {
#include "stg01/fish_models/fish_same_7_uvs1.inc"
};
static NJS_COLOR fish_same_7_uvs2[8] = {
#include "stg01/fish_models/fish_same_7_uvs2.inc"
};
static NJS_COLOR fish_same_7_uvs3[42] = {
#include "stg01/fish_models/fish_same_7_uvs3.inc"
};
static NJS_COLOR fish_same_7_uvs4[18] = {
#include "stg01/fish_models/fish_same_7_uvs4.inc"
};
static NJS_COLOR fish_same_7_uvs5[9] = {
#include "stg01/fish_models/fish_same_7_uvs5.inc"
};
static NJS_MESHSET fish_same_7_meshsets[6] = {
#include "stg01/fish_models/fish_same_7_meshsets.inc"
};
static NJS_POINT3 fish_same_7_points[81] = {
#include "stg01/fish_models/fish_same_7_points.inc"
};
static NJS_VECTOR fish_same_7_normals[81] = {
#include "stg01/fish_models/fish_same_7_normals.inc"
};
static NJS_MODEL fish_same_7_model = {
#include "stg01/fish_models/fish_same_7_model.inc"
};
static NJS_OBJECT fish_same_7 = {
#include "stg01/fish_models/fish_same_7.inc"
};
static NJS_MATERIAL fish_same_6_materials[2] = {
#include "stg01/fish_models/fish_same_6_materials.inc"
};
static Sint16 fish_same_6_meshes0[26] = {
#include "stg01/fish_models/fish_same_6_meshes0.inc"
};
static Sint16 fish_same_6_meshes1[21] = {
#include "stg01/fish_models/fish_same_6_meshes1.inc"
};
static NJS_COLOR fish_same_6_uvs0[22] = {
#include "stg01/fish_models/fish_same_6_uvs0.inc"
};
static NJS_COLOR fish_same_6_uvs1[18] = {
#include "stg01/fish_models/fish_same_6_uvs1.inc"
};
static NJS_MESHSET fish_same_6_meshsets[2] = {
#include "stg01/fish_models/fish_same_6_meshsets.inc"
};
static NJS_POINT3 fish_same_6_points[22] = {
#include "stg01/fish_models/fish_same_6_points.inc"
};
static NJS_VECTOR fish_same_6_normals[22] = {
#include "stg01/fish_models/fish_same_6_normals.inc"
};
static NJS_MODEL fish_same_6_model = {
#include "stg01/fish_models/fish_same_6_model.inc"
};
static NJS_OBJECT fish_same_6 = {
#include "stg01/fish_models/fish_same_6.inc"
};
static NJS_MATERIAL fish_same_5_materials[3] = {
#include "stg01/fish_models/fish_same_5_materials.inc"
};
static Sint16 fish_same_5_meshes0[5] = {
#include "stg01/fish_models/fish_same_5_meshes0.inc"
};
static Sint16 fish_same_5_meshes1[32] = {
#include "stg01/fish_models/fish_same_5_meshes1.inc"
};
static Sint16 fish_same_5_meshes2[36] = {
#include "stg01/fish_models/fish_same_5_meshes2.inc"
};
static NJS_COLOR fish_same_5_uvs0[4] = {
#include "stg01/fish_models/fish_same_5_uvs0.inc"
};
static NJS_COLOR fish_same_5_uvs1[26] = {
#include "stg01/fish_models/fish_same_5_uvs1.inc"
};
static NJS_COLOR fish_same_5_uvs2[30] = {
#include "stg01/fish_models/fish_same_5_uvs2.inc"
};
static NJS_MESHSET fish_same_5_meshsets[3] = {
#include "stg01/fish_models/fish_same_5_meshsets.inc"
};
static NJS_POINT3 fish_same_5_points[28] = {
#include "stg01/fish_models/fish_same_5_points.inc"
};
static NJS_VECTOR fish_same_5_normals[28] = {
#include "stg01/fish_models/fish_same_5_normals.inc"
};
static NJS_MODEL fish_same_5_model = {
#include "stg01/fish_models/fish_same_5_model.inc"
};
static NJS_OBJECT fish_same_5 = {
#include "stg01/fish_models/fish_same_5.inc"
};
static NJS_OBJECT fish_same_4 = {
#include "stg01/fish_models/fish_same_4.inc"
};
static NJS_OBJECT fish_same_3 = {
#include "stg01/fish_models/fish_same_3.inc"
};
static NJS_OBJECT fish_same_2 = {
#include "stg01/fish_models/fish_same_2.inc"
};
static NJS_OBJECT fish_same_1 = {
#include "stg01/fish_models/fish_same_1.inc"
};
NJS_OBJECT fish_same_object = {
#include "stg01/fish_models/fish_same_object.inc"
};

static NJS_TEXNAME fish_una_texnames[6] = {
    {TEXT_77, 0, 0}, {TEXT_78, 0, 0}, {TEXT_79, 0, 0},
    {TEXT_80, 0, 0}, {TEXT_6, 0, 0},  {TEXT_81, 0, 0},
};
NJS_TEXLIST fish_una_texlist = {
#include "stg01/fish_models/fish_una_texlist.inc"
};
static NJS_MATERIAL fish_una_10_materials[2] = {
#include "stg01/fish_models/fish_una_10_materials.inc"
};
static Sint16 fish_una_10_meshes0[10] = {
#include "stg01/fish_models/fish_una_10_meshes0.inc"
};
static Sint16 fish_una_10_meshes1[8] = {
#include "stg01/fish_models/fish_una_10_meshes1.inc"
};
static NJS_COLOR fish_una_10_uvs0[8] = {
#include "stg01/fish_models/fish_una_10_uvs0.inc"
};
static NJS_COLOR fish_una_10_uvs1[7] = {
#include "stg01/fish_models/fish_una_10_uvs1.inc"
};
static NJS_MESHSET fish_una_10_meshsets[2] = {
#include "stg01/fish_models/fish_una_10_meshsets.inc"
};
static NJS_POINT3 fish_una_10_points[9] = {
#include "stg01/fish_models/fish_una_10_points.inc"
};
static NJS_VECTOR fish_una_10_normals[9] = {
#include "stg01/fish_models/fish_una_10_normals.inc"
};
static NJS_MODEL fish_una_10_model = {
#include "stg01/fish_models/fish_una_10_model.inc"
};
static NJS_OBJECT fish_una_10 = {
#include "stg01/fish_models/fish_una_10.inc"
};
static NJS_MATERIAL fish_una_9_materials[2] = {
#include "stg01/fish_models/fish_una_9_materials.inc"
};
static Sint16 fish_una_9_meshes0[10] = {
#include "stg01/fish_models/fish_una_9_meshes0.inc"
};
static Sint16 fish_una_9_meshes1[8] = {
#include "stg01/fish_models/fish_una_9_meshes1.inc"
};
static NJS_COLOR fish_una_9_uvs0[8] = {
#include "stg01/fish_models/fish_una_9_uvs0.inc"
};
static NJS_COLOR fish_una_9_uvs1[7] = {
#include "stg01/fish_models/fish_una_9_uvs1.inc"
};
static NJS_MESHSET fish_una_9_meshsets[2] = {
#include "stg01/fish_models/fish_una_9_meshsets.inc"
};
static NJS_POINT3 fish_una_9_points[9] = {
#include "stg01/fish_models/fish_una_9_points.inc"
};
static NJS_VECTOR fish_una_9_normals[9] = {
#include "stg01/fish_models/fish_una_9_normals.inc"
};
static NJS_MODEL fish_una_9_model = {
#include "stg01/fish_models/fish_una_9_model.inc"
};
static NJS_OBJECT fish_una_9 = {
#include "stg01/fish_models/fish_una_9.inc"
};
static NJS_MATERIAL fish_una_8_materials[3] = {
#include "stg01/fish_models/fish_una_8_materials.inc"
};
static Sint16 fish_una_8_meshes0[12] = {
#include "stg01/fish_models/fish_una_8_meshes0.inc"
};
static Sint16 fish_una_8_meshes1[5] = {
#include "stg01/fish_models/fish_una_8_meshes1.inc"
};
static Sint16 fish_una_8_meshes2[10] = {
#include "stg01/fish_models/fish_una_8_meshes2.inc"
};
static NJS_COLOR fish_una_8_uvs0[10] = {
#include "stg01/fish_models/fish_una_8_uvs0.inc"
};
static NJS_COLOR fish_una_8_uvs1[4] = {
#include "stg01/fish_models/fish_una_8_uvs1.inc"
};
static NJS_COLOR fish_una_8_uvs2[8] = {
#include "stg01/fish_models/fish_una_8_uvs2.inc"
};
static NJS_MESHSET fish_una_8_meshsets[3] = {
#include "stg01/fish_models/fish_una_8_meshsets.inc"
};
static NJS_POINT3 fish_una_8_points[15] = {
#include "stg01/fish_models/fish_una_8_points.inc"
};
static NJS_VECTOR fish_una_8_normals[15] = {
#include "stg01/fish_models/fish_una_8_normals.inc"
};
static NJS_MODEL fish_una_8_model = {
#include "stg01/fish_models/fish_una_8_model.inc"
};
static NJS_OBJECT fish_una_8 = {
#include "stg01/fish_models/fish_una_8.inc"
};
static NJS_MATERIAL fish_una_7_materials[3] = {
#include "stg01/fish_models/fish_una_7_materials.inc"
};
static Sint16 fish_una_7_meshes0[7] = {
#include "stg01/fish_models/fish_una_7_meshes0.inc"
};
static Sint16 fish_una_7_meshes1[11] = {
#include "stg01/fish_models/fish_una_7_meshes1.inc"
};
static Sint16 fish_una_7_meshes2[10] = {
#include "stg01/fish_models/fish_una_7_meshes2.inc"
};
static NJS_COLOR fish_una_7_uvs0[6] = {
#include "stg01/fish_models/fish_una_7_uvs0.inc"
};
static NJS_COLOR fish_una_7_uvs1[10] = {
#include "stg01/fish_models/fish_una_7_uvs1.inc"
};
static NJS_COLOR fish_una_7_uvs2[8] = {
#include "stg01/fish_models/fish_una_7_uvs2.inc"
};
static NJS_MESHSET fish_una_7_meshsets[3] = {
#include "stg01/fish_models/fish_una_7_meshsets.inc"
};
static NJS_POINT3 fish_una_7_points[20] = {
#include "stg01/fish_models/fish_una_7_points.inc"
};
static NJS_VECTOR fish_una_7_normals[20] = {
#include "stg01/fish_models/fish_una_7_normals.inc"
};
static NJS_MODEL fish_una_7_model = {
#include "stg01/fish_models/fish_una_7_model.inc"
};
static NJS_OBJECT fish_una_7 = {
#include "stg01/fish_models/fish_una_7.inc"
};
static NJS_MATERIAL fish_una_6_materials[3] = {
#include "stg01/fish_models/fish_una_6_materials.inc"
};
static Sint16 fish_una_6_meshes0[7] = {
#include "stg01/fish_models/fish_una_6_meshes0.inc"
};
static Sint16 fish_una_6_meshes1[10] = {
#include "stg01/fish_models/fish_una_6_meshes1.inc"
};
static Sint16 fish_una_6_meshes2[11] = {
#include "stg01/fish_models/fish_una_6_meshes2.inc"
};
static NJS_COLOR fish_una_6_uvs0[6] = {
#include "stg01/fish_models/fish_una_6_uvs0.inc"
};
static NJS_COLOR fish_una_6_uvs1[8] = {
#include "stg01/fish_models/fish_una_6_uvs1.inc"
};
static NJS_COLOR fish_una_6_uvs2[10] = {
#include "stg01/fish_models/fish_una_6_uvs2.inc"
};
static NJS_MESHSET fish_una_6_meshsets[3] = {
#include "stg01/fish_models/fish_una_6_meshsets.inc"
};
static NJS_POINT3 fish_una_6_points[20] = {
#include "stg01/fish_models/fish_una_6_points.inc"
};
static NJS_VECTOR fish_una_6_normals[20] = {
#include "stg01/fish_models/fish_una_6_normals.inc"
};
static NJS_MODEL fish_una_6_model = {
#include "stg01/fish_models/fish_una_6_model.inc"
};
static NJS_OBJECT fish_una_6 = {
#include "stg01/fish_models/fish_una_6.inc"
};
static NJS_MATERIAL fish_una_5_materials[3] = {
#include "stg01/fish_models/fish_una_5_materials.inc"
};
static Sint16 fish_una_5_meshes0[14] = {
#include "stg01/fish_models/fish_una_5_meshes0.inc"
};
static Sint16 fish_una_5_meshes1[7] = {
#include "stg01/fish_models/fish_una_5_meshes1.inc"
};
static Sint16 fish_una_5_meshes2[5] = {
#include "stg01/fish_models/fish_una_5_meshes2.inc"
};
static NJS_COLOR fish_una_5_uvs0[12] = {
#include "stg01/fish_models/fish_una_5_uvs0.inc"
};
static NJS_COLOR fish_una_5_uvs1[6] = {
#include "stg01/fish_models/fish_una_5_uvs1.inc"
};
static NJS_COLOR fish_una_5_uvs2[4] = {
#include "stg01/fish_models/fish_una_5_uvs2.inc"
};
static NJS_MESHSET fish_una_5_meshsets[3] = {
#include "stg01/fish_models/fish_una_5_meshsets.inc"
};
static NJS_POINT3 fish_una_5_points[16] = {
#include "stg01/fish_models/fish_una_5_points.inc"
};
static NJS_VECTOR fish_una_5_normals[16] = {
#include "stg01/fish_models/fish_una_5_normals.inc"
};
static NJS_MODEL fish_una_5_model = {
#include "stg01/fish_models/fish_una_5_model.inc"
};
static NJS_OBJECT fish_una_5 = {
#include "stg01/fish_models/fish_una_5.inc"
};
static NJS_MATERIAL fish_una_4_materials[3] = {
#include "stg01/fish_models/fish_una_4_materials.inc"
};
static Sint16 fish_una_4_meshes0[7] = {
#include "stg01/fish_models/fish_una_4_meshes0.inc"
};
static Sint16 fish_una_4_meshes1[11] = {
#include "stg01/fish_models/fish_una_4_meshes1.inc"
};
static Sint16 fish_una_4_meshes2[5] = {
#include "stg01/fish_models/fish_una_4_meshes2.inc"
};
static NJS_COLOR fish_una_4_uvs0[6] = {
#include "stg01/fish_models/fish_una_4_uvs0.inc"
};
static NJS_COLOR fish_una_4_uvs1[10] = {
#include "stg01/fish_models/fish_una_4_uvs1.inc"
};
static NJS_COLOR fish_una_4_uvs2[4] = {
#include "stg01/fish_models/fish_una_4_uvs2.inc"
};
static NJS_MESHSET fish_una_4_meshsets[3] = {
#include "stg01/fish_models/fish_una_4_meshsets.inc"
};
static NJS_POINT3 fish_una_4_points[16] = {
#include "stg01/fish_models/fish_una_4_points.inc"
};
static NJS_VECTOR fish_una_4_normals[16] = {
#include "stg01/fish_models/fish_una_4_normals.inc"
};
static NJS_MODEL fish_una_4_model = {
#include "stg01/fish_models/fish_una_4_model.inc"
};
static NJS_OBJECT fish_una_4 = {
#include "stg01/fish_models/fish_una_4.inc"
};
static NJS_MATERIAL fish_una_3_materials[2] = {
#include "stg01/fish_models/fish_una_3_materials.inc"
};
static Sint16 fish_una_3_meshes0[10] = {
#include "stg01/fish_models/fish_una_3_meshes0.inc"
};
static Sint16 fish_una_3_meshes1[11] = {
#include "stg01/fish_models/fish_una_3_meshes1.inc"
};
static NJS_COLOR fish_una_3_uvs0[8] = {
#include "stg01/fish_models/fish_una_3_uvs0.inc"
};
static NJS_COLOR fish_una_3_uvs1[10] = {
#include "stg01/fish_models/fish_una_3_uvs1.inc"
};
static NJS_MESHSET fish_una_3_meshsets[2] = {
#include "stg01/fish_models/fish_una_3_meshsets.inc"
};
static NJS_POINT3 fish_una_3_points[12] = {
#include "stg01/fish_models/fish_una_3_points.inc"
};
static NJS_VECTOR fish_una_3_normals[12] = {
#include "stg01/fish_models/fish_una_3_normals.inc"
};
static NJS_MODEL fish_una_3_model = {
#include "stg01/fish_models/fish_una_3_model.inc"
};
static NJS_OBJECT fish_una_3 = {
#include "stg01/fish_models/fish_una_3.inc"
};
static NJS_MATERIAL fish_una_2_materials[3] = {
#include "stg01/fish_models/fish_una_2_materials.inc"
};
static Sint16 fish_una_2_meshes0[12] = {
#include "stg01/fish_models/fish_una_2_meshes0.inc"
};
static Sint16 fish_una_2_meshes1[10] = {
#include "stg01/fish_models/fish_una_2_meshes1.inc"
};
static Sint16 fish_una_2_meshes2[16] = {
#include "stg01/fish_models/fish_una_2_meshes2.inc"
};
static NJS_COLOR fish_una_2_uvs0[10] = {
#include "stg01/fish_models/fish_una_2_uvs0.inc"
};
static NJS_COLOR fish_una_2_uvs1[8] = {
#include "stg01/fish_models/fish_una_2_uvs1.inc"
};
static NJS_COLOR fish_una_2_uvs2[13] = {
#include "stg01/fish_models/fish_una_2_uvs2.inc"
};
static NJS_MESHSET fish_una_2_meshsets[3] = {
#include "stg01/fish_models/fish_una_2_meshsets.inc"
};
static NJS_POINT3 fish_una_2_points[11] = {
#include "stg01/fish_models/fish_una_2_points.inc"
};
static NJS_VECTOR fish_una_2_normals[11] = {
#include "stg01/fish_models/fish_una_2_normals.inc"
};
static NJS_MODEL fish_una_2_model = {
#include "stg01/fish_models/fish_una_2_model.inc"
};
static NJS_OBJECT fish_una_2 = {
#include "stg01/fish_models/fish_una_2.inc"
};
static NJS_MATERIAL fish_una_1_materials[5] = {
#include "stg01/fish_models/fish_una_1_materials.inc"
};
static Sint16 fish_una_1_meshes0[61] = {
#include "stg01/fish_models/fish_una_1_meshes0.inc"
};
static Sint16 fish_una_1_meshes1[10] = {
#include "stg01/fish_models/fish_una_1_meshes1.inc"
};
static Sint16 fish_una_1_meshes2[14] = {
#include "stg01/fish_models/fish_una_1_meshes2.inc"
};
static Sint16 fish_una_1_meshes3[19] = {
#include "stg01/fish_models/fish_una_1_meshes3.inc"
};
static Sint16 fish_una_1_meshes4[13] = {
#include "stg01/fish_models/fish_una_1_meshes4.inc"
};
static NJS_COLOR fish_una_1_uvs0[52] = {
#include "stg01/fish_models/fish_una_1_uvs0.inc"
};
static NJS_COLOR fish_una_1_uvs1[8] = {
#include "stg01/fish_models/fish_una_1_uvs1.inc"
};
static NJS_COLOR fish_una_1_uvs2[12] = {
#include "stg01/fish_models/fish_una_1_uvs2.inc"
};
static NJS_COLOR fish_una_1_uvs3[16] = {
#include "stg01/fish_models/fish_una_1_uvs3.inc"
};
static NJS_COLOR fish_una_1_uvs4[11] = {
#include "stg01/fish_models/fish_una_1_uvs4.inc"
};
static NJS_MESHSET fish_una_1_meshsets[5] = {
#include "stg01/fish_models/fish_una_1_meshsets.inc"
};
static NJS_POINT3 fish_una_1_points[36] = {
#include "stg01/fish_models/fish_una_1_points.inc"
};
static NJS_VECTOR fish_una_1_normals[36] = {
#include "stg01/fish_models/fish_una_1_normals.inc"
};
static NJS_MODEL fish_una_1_model = {
#include "stg01/fish_models/fish_una_1_model.inc"
};
static NJS_OBJECT fish_una_1 = {
#include "stg01/fish_models/fish_una_1.inc"
};
NJS_OBJECT fish_una_object = {
#include "stg01/fish_models/fish_una_object.inc"
};

static NJS_TEXNAME fish_utu_texnames[6] = {
    {TEXT_82, 0, 0}, {TEXT_83, 0, 0}, {TEXT_84, 0, 0},
    {TEXT_6, 0, 0},  {TEXT_53, 0, 0}, {TEXT_85, 0, 0},
};
NJS_TEXLIST fish_utu_texlist = {
#include "stg01/fish_models/fish_utu_texlist.inc"
};
static NJS_MATERIAL fish_utu_10_materials[1] = {
#include "stg01/fish_models/fish_utu_10_materials.inc"
};
static Sint16 fish_utu_10_meshes0[10] = {
#include "stg01/fish_models/fish_utu_10_meshes0.inc"
};
static NJS_COLOR fish_utu_10_uvs0[8] = {
#include "stg01/fish_models/fish_utu_10_uvs0.inc"
};
static NJS_MESHSET fish_utu_10_meshsets[1] = {
#include "stg01/fish_models/fish_utu_10_meshsets.inc"
};
static NJS_POINT3 fish_utu_10_points[5] = {
#include "stg01/fish_models/fish_utu_10_points.inc"
};
static NJS_VECTOR fish_utu_10_normals[5] = {
#include "stg01/fish_models/fish_utu_10_normals.inc"
};
static NJS_MODEL fish_utu_10_model = {
#include "stg01/fish_models/fish_utu_10_model.inc"
};
static NJS_OBJECT fish_utu_10 = {
#include "stg01/fish_models/fish_utu_10.inc"
};
static NJS_MATERIAL fish_utu_9_materials[1] = {
#include "stg01/fish_models/fish_utu_9_materials.inc"
};
static Sint16 fish_utu_9_meshes0[10] = {
#include "stg01/fish_models/fish_utu_9_meshes0.inc"
};
static NJS_COLOR fish_utu_9_uvs0[8] = {
#include "stg01/fish_models/fish_utu_9_uvs0.inc"
};
static NJS_MESHSET fish_utu_9_meshsets[1] = {
#include "stg01/fish_models/fish_utu_9_meshsets.inc"
};
static NJS_POINT3 fish_utu_9_points[5] = {
#include "stg01/fish_models/fish_utu_9_points.inc"
};
static NJS_VECTOR fish_utu_9_normals[5] = {
#include "stg01/fish_models/fish_utu_9_normals.inc"
};
static NJS_MODEL fish_utu_9_model = {
#include "stg01/fish_models/fish_utu_9_model.inc"
};
static NJS_OBJECT fish_utu_9 = {
#include "stg01/fish_models/fish_utu_9.inc"
};
static NJS_MATERIAL fish_utu_8_materials[2] = {
#include "stg01/fish_models/fish_utu_8_materials.inc"
};
static Sint16 fish_utu_8_meshes0[12] = {
#include "stg01/fish_models/fish_utu_8_meshes0.inc"
};
static Sint16 fish_utu_8_meshes1[12] = {
#include "stg01/fish_models/fish_utu_8_meshes1.inc"
};
static NJS_COLOR fish_utu_8_uvs0[10] = {
#include "stg01/fish_models/fish_utu_8_uvs0.inc"
};
static NJS_COLOR fish_utu_8_uvs1[10] = {
#include "stg01/fish_models/fish_utu_8_uvs1.inc"
};
static NJS_MESHSET fish_utu_8_meshsets[2] = {
#include "stg01/fish_models/fish_utu_8_meshsets.inc"
};
static NJS_POINT3 fish_utu_8_points[15] = {
#include "stg01/fish_models/fish_utu_8_points.inc"
};
static NJS_VECTOR fish_utu_8_normals[15] = {
#include "stg01/fish_models/fish_utu_8_normals.inc"
};
static NJS_MODEL fish_utu_8_model = {
#include "stg01/fish_models/fish_utu_8_model.inc"
};
static NJS_OBJECT fish_utu_8 = {
#include "stg01/fish_models/fish_utu_8.inc"
};
static NJS_MATERIAL fish_utu_7_materials[2] = {
#include "stg01/fish_models/fish_utu_7_materials.inc"
};
static Sint16 fish_utu_7_meshes0[15] = {
#include "stg01/fish_models/fish_utu_7_meshes0.inc"
};
static Sint16 fish_utu_7_meshes1[10] = {
#include "stg01/fish_models/fish_utu_7_meshes1.inc"
};
static NJS_COLOR fish_utu_7_uvs0[14] = {
#include "stg01/fish_models/fish_utu_7_uvs0.inc"
};
static NJS_COLOR fish_utu_7_uvs1[8] = {
#include "stg01/fish_models/fish_utu_7_uvs1.inc"
};
static NJS_MESHSET fish_utu_7_meshsets[2] = {
#include "stg01/fish_models/fish_utu_7_meshsets.inc"
};
static NJS_POINT3 fish_utu_7_points[20] = {
#include "stg01/fish_models/fish_utu_7_points.inc"
};
static NJS_VECTOR fish_utu_7_normals[20] = {
#include "stg01/fish_models/fish_utu_7_normals.inc"
};
static NJS_MODEL fish_utu_7_model = {
#include "stg01/fish_models/fish_utu_7_model.inc"
};
static NJS_OBJECT fish_utu_7 = {
#include "stg01/fish_models/fish_utu_7.inc"
};
static NJS_MATERIAL fish_utu_6_materials[2] = {
#include "stg01/fish_models/fish_utu_6_materials.inc"
};
static Sint16 fish_utu_6_meshes0[15] = {
#include "stg01/fish_models/fish_utu_6_meshes0.inc"
};
static Sint16 fish_utu_6_meshes1[10] = {
#include "stg01/fish_models/fish_utu_6_meshes1.inc"
};
static NJS_COLOR fish_utu_6_uvs0[14] = {
#include "stg01/fish_models/fish_utu_6_uvs0.inc"
};
static NJS_COLOR fish_utu_6_uvs1[8] = {
#include "stg01/fish_models/fish_utu_6_uvs1.inc"
};
static NJS_MESHSET fish_utu_6_meshsets[2] = {
#include "stg01/fish_models/fish_utu_6_meshsets.inc"
};
static NJS_POINT3 fish_utu_6_points[20] = {
#include "stg01/fish_models/fish_utu_6_points.inc"
};
static NJS_VECTOR fish_utu_6_normals[20] = {
#include "stg01/fish_models/fish_utu_6_normals.inc"
};
static NJS_MODEL fish_utu_6_model = {
#include "stg01/fish_models/fish_utu_6_model.inc"
};
static NJS_OBJECT fish_utu_6 = {
#include "stg01/fish_models/fish_utu_6.inc"
};
static NJS_MATERIAL fish_utu_5_materials[2] = {
#include "stg01/fish_models/fish_utu_5_materials.inc"
};
static Sint16 fish_utu_5_meshes0[18] = {
#include "stg01/fish_models/fish_utu_5_meshes0.inc"
};
static Sint16 fish_utu_5_meshes1[5] = {
#include "stg01/fish_models/fish_utu_5_meshes1.inc"
};
static NJS_COLOR fish_utu_5_uvs0[16] = {
#include "stg01/fish_models/fish_utu_5_uvs0.inc"
};
static NJS_COLOR fish_utu_5_uvs1[4] = {
#include "stg01/fish_models/fish_utu_5_uvs1.inc"
};
static NJS_MESHSET fish_utu_5_meshsets[2] = {
#include "stg01/fish_models/fish_utu_5_meshsets.inc"
};
static NJS_POINT3 fish_utu_5_points[16] = {
#include "stg01/fish_models/fish_utu_5_points.inc"
};
static NJS_VECTOR fish_utu_5_normals[16] = {
#include "stg01/fish_models/fish_utu_5_normals.inc"
};
static NJS_MODEL fish_utu_5_model = {
#include "stg01/fish_models/fish_utu_5_model.inc"
};
static NJS_OBJECT fish_utu_5 = {
#include "stg01/fish_models/fish_utu_5.inc"
};
static NJS_MATERIAL fish_utu_4_materials[2] = {
#include "stg01/fish_models/fish_utu_4_materials.inc"
};
static Sint16 fish_utu_4_meshes0[18] = {
#include "stg01/fish_models/fish_utu_4_meshes0.inc"
};
static Sint16 fish_utu_4_meshes1[5] = {
#include "stg01/fish_models/fish_utu_4_meshes1.inc"
};
static NJS_COLOR fish_utu_4_uvs0[16] = {
#include "stg01/fish_models/fish_utu_4_uvs0.inc"
};
static NJS_COLOR fish_utu_4_uvs1[4] = {
#include "stg01/fish_models/fish_utu_4_uvs1.inc"
};
static NJS_MESHSET fish_utu_4_meshsets[2] = {
#include "stg01/fish_models/fish_utu_4_meshsets.inc"
};
static NJS_POINT3 fish_utu_4_points[16] = {
#include "stg01/fish_models/fish_utu_4_points.inc"
};
static NJS_VECTOR fish_utu_4_normals[16] = {
#include "stg01/fish_models/fish_utu_4_normals.inc"
};
static NJS_MODEL fish_utu_4_model = {
#include "stg01/fish_models/fish_utu_4_model.inc"
};
static NJS_OBJECT fish_utu_4 = {
#include "stg01/fish_models/fish_utu_4.inc"
};
static NJS_MATERIAL fish_utu_3_materials[2] = {
#include "stg01/fish_models/fish_utu_3_materials.inc"
};
static Sint16 fish_utu_3_meshes0[24] = {
#include "stg01/fish_models/fish_utu_3_meshes0.inc"
};
static Sint16 fish_utu_3_meshes1[4] = {
#include "stg01/fish_models/fish_utu_3_meshes1.inc"
};
static NJS_COLOR fish_utu_3_uvs0[20] = {
#include "stg01/fish_models/fish_utu_3_uvs0.inc"
};
static NJS_COLOR fish_utu_3_uvs1[3] = {
#include "stg01/fish_models/fish_utu_3_uvs1.inc"
};
static NJS_MESHSET fish_utu_3_meshsets[2] = {
#include "stg01/fish_models/fish_utu_3_meshsets.inc"
};
static NJS_POINT3 fish_utu_3_points[15] = {
#include "stg01/fish_models/fish_utu_3_points.inc"
};
static NJS_VECTOR fish_utu_3_normals[15] = {
#include "stg01/fish_models/fish_utu_3_normals.inc"
};
static NJS_MODEL fish_utu_3_model = {
#include "stg01/fish_models/fish_utu_3_model.inc"
};
static NJS_OBJECT fish_utu_3 = {
#include "stg01/fish_models/fish_utu_3.inc"
};
static NJS_MATERIAL fish_utu_2_materials[3] = {
#include "stg01/fish_models/fish_utu_2_materials.inc"
};
static Sint16 fish_utu_2_meshes0[23] = {
#include "stg01/fish_models/fish_utu_2_meshes0.inc"
};
static Sint16 fish_utu_2_meshes1[23] = {
#include "stg01/fish_models/fish_utu_2_meshes1.inc"
};
static Sint16 fish_utu_2_meshes2[48] = {
#include "stg01/fish_models/fish_utu_2_meshes2.inc"
};
static NJS_COLOR fish_utu_2_uvs0[20] = {
#include "stg01/fish_models/fish_utu_2_uvs0.inc"
};
static NJS_COLOR fish_utu_2_uvs1[19] = {
#include "stg01/fish_models/fish_utu_2_uvs1.inc"
};
static NJS_COLOR fish_utu_2_uvs2[38] = {
#include "stg01/fish_models/fish_utu_2_uvs2.inc"
};
static NJS_MESHSET fish_utu_2_meshsets[3] = {
#include "stg01/fish_models/fish_utu_2_meshsets.inc"
};
static NJS_POINT3 fish_utu_2_points[45] = {
#include "stg01/fish_models/fish_utu_2_points.inc"
};
static NJS_VECTOR fish_utu_2_normals[45] = {
#include "stg01/fish_models/fish_utu_2_normals.inc"
};
static NJS_MODEL fish_utu_2_model = {
#include "stg01/fish_models/fish_utu_2_model.inc"
};
static NJS_OBJECT fish_utu_2 = {
#include "stg01/fish_models/fish_utu_2.inc"
};
static NJS_MATERIAL fish_utu_1_materials[5] = {
#include "stg01/fish_models/fish_utu_1_materials.inc"
};
static Sint16 fish_utu_1_meshes0[10] = {
#include "stg01/fish_models/fish_utu_1_meshes0.inc"
};
static Sint16 fish_utu_1_meshes1[46] = {
#include "stg01/fish_models/fish_utu_1_meshes1.inc"
};
static Sint16 fish_utu_1_meshes2[10] = {
#include "stg01/fish_models/fish_utu_1_meshes2.inc"
};
static Sint16 fish_utu_1_meshes3[21] = {
#include "stg01/fish_models/fish_utu_1_meshes3.inc"
};
static Sint16 fish_utu_1_meshes4[50] = {
#include "stg01/fish_models/fish_utu_1_meshes4.inc"
};
static NJS_COLOR fish_utu_1_uvs0[8] = {
#include "stg01/fish_models/fish_utu_1_uvs0.inc"
};
static NJS_COLOR fish_utu_1_uvs1[40] = {
#include "stg01/fish_models/fish_utu_1_uvs1.inc"
};
static NJS_COLOR fish_utu_1_uvs2[8] = {
#include "stg01/fish_models/fish_utu_1_uvs2.inc"
};
static NJS_COLOR fish_utu_1_uvs3[17] = {
#include "stg01/fish_models/fish_utu_1_uvs3.inc"
};
static NJS_COLOR fish_utu_1_uvs4[40] = {
#include "stg01/fish_models/fish_utu_1_uvs4.inc"
};
static NJS_MESHSET fish_utu_1_meshsets[5] = {
#include "stg01/fish_models/fish_utu_1_meshsets.inc"
};
static NJS_POINT3 fish_utu_1_points[72] = {
#include "stg01/fish_models/fish_utu_1_points.inc"
};
static NJS_VECTOR fish_utu_1_normals[72] = {
#include "stg01/fish_models/fish_utu_1_normals.inc"
};
static NJS_MODEL fish_utu_1_model = {
#include "stg01/fish_models/fish_utu_1_model.inc"
};
static NJS_OBJECT fish_utu_1 = {
#include "stg01/fish_models/fish_utu_1.inc"
};
NJS_OBJECT fish_utu_object = {
#include "stg01/fish_models/fish_utu_object.inc"
};

static NJS_TEXNAME fish_ryu_texnames[11] = {
    {TEXT_86, 0, 0}, {TEXT_87, 0, 0}, {TEXT_88, 0, 0}, {TEXT_89, 0, 0},
    {TEXT_90, 0, 0}, {TEXT_91, 0, 0}, {TEXT_92, 0, 0}, {TEXT_93, 0, 0},
    {TEXT_94, 0, 0}, {TEXT_11, 0, 0}, {TEXT_95, 0, 0},
};
NJS_TEXLIST fish_ryu_texlist = {
#include "stg01/fish_models/fish_ryu_texlist.inc"
};
static NJS_MATERIAL fish_ryu_10_materials[2] = {
#include "stg01/fish_models/fish_ryu_10_materials.inc"
};
static Sint16 fish_ryu_10_meshes0[10] = {
#include "stg01/fish_models/fish_ryu_10_meshes0.inc"
};
static Sint16 fish_ryu_10_meshes1[8] = {
#include "stg01/fish_models/fish_ryu_10_meshes1.inc"
};
static NJS_COLOR fish_ryu_10_uvs0[8] = {
#include "stg01/fish_models/fish_ryu_10_uvs0.inc"
};
static NJS_COLOR fish_ryu_10_uvs1[7] = {
#include "stg01/fish_models/fish_ryu_10_uvs1.inc"
};
static NJS_MESHSET fish_ryu_10_meshsets[2] = {
#include "stg01/fish_models/fish_ryu_10_meshsets.inc"
};
static NJS_POINT3 fish_ryu_10_points[9] = {
#include "stg01/fish_models/fish_ryu_10_points.inc"
};
static NJS_VECTOR fish_ryu_10_normals[9] = {
#include "stg01/fish_models/fish_ryu_10_normals.inc"
};
static NJS_MODEL fish_ryu_10_model = {
#include "stg01/fish_models/fish_ryu_10_model.inc"
};
static NJS_OBJECT fish_ryu_10 = {
#include "stg01/fish_models/fish_ryu_10.inc"
};
static NJS_MATERIAL fish_ryu_9_materials[2] = {
#include "stg01/fish_models/fish_ryu_9_materials.inc"
};
static Sint16 fish_ryu_9_meshes0[10] = {
#include "stg01/fish_models/fish_ryu_9_meshes0.inc"
};
static Sint16 fish_ryu_9_meshes1[8] = {
#include "stg01/fish_models/fish_ryu_9_meshes1.inc"
};
static NJS_COLOR fish_ryu_9_uvs0[8] = {
#include "stg01/fish_models/fish_ryu_9_uvs0.inc"
};
static NJS_COLOR fish_ryu_9_uvs1[7] = {
#include "stg01/fish_models/fish_ryu_9_uvs1.inc"
};
static NJS_MESHSET fish_ryu_9_meshsets[2] = {
#include "stg01/fish_models/fish_ryu_9_meshsets.inc"
};
static NJS_POINT3 fish_ryu_9_points[9] = {
#include "stg01/fish_models/fish_ryu_9_points.inc"
};
static NJS_VECTOR fish_ryu_9_normals[9] = {
#include "stg01/fish_models/fish_ryu_9_normals.inc"
};
static NJS_MODEL fish_ryu_9_model = {
#include "stg01/fish_models/fish_ryu_9_model.inc"
};
static NJS_OBJECT fish_ryu_9 = {
#include "stg01/fish_models/fish_ryu_9.inc"
};
static NJS_MATERIAL fish_ryu_8_materials[3] = {
#include "stg01/fish_models/fish_ryu_8_materials.inc"
};
static Sint16 fish_ryu_8_meshes0[4] = {
#include "stg01/fish_models/fish_ryu_8_meshes0.inc"
};
static Sint16 fish_ryu_8_meshes1[12] = {
#include "stg01/fish_models/fish_ryu_8_meshes1.inc"
};
static Sint16 fish_ryu_8_meshes2[4] = {
#include "stg01/fish_models/fish_ryu_8_meshes2.inc"
};
static NJS_COLOR fish_ryu_8_uvs0[3] = {
#include "stg01/fish_models/fish_ryu_8_uvs0.inc"
};
static NJS_COLOR fish_ryu_8_uvs1[10] = {
#include "stg01/fish_models/fish_ryu_8_uvs1.inc"
};
static NJS_COLOR fish_ryu_8_uvs2[3] = {
#include "stg01/fish_models/fish_ryu_8_uvs2.inc"
};
static NJS_MESHSET fish_ryu_8_meshsets[3] = {
#include "stg01/fish_models/fish_ryu_8_meshsets.inc"
};
static NJS_POINT3 fish_ryu_8_points[12] = {
#include "stg01/fish_models/fish_ryu_8_points.inc"
};
static NJS_VECTOR fish_ryu_8_normals[12] = {
#include "stg01/fish_models/fish_ryu_8_normals.inc"
};
static NJS_MODEL fish_ryu_8_model = {
#include "stg01/fish_models/fish_ryu_8_model.inc"
};
static NJS_OBJECT fish_ryu_8 = {
#include "stg01/fish_models/fish_ryu_8.inc"
};
static NJS_MATERIAL fish_ryu_7_materials[3] = {
#include "stg01/fish_models/fish_ryu_7_materials.inc"
};
static Sint16 fish_ryu_7_meshes0[15] = {
#include "stg01/fish_models/fish_ryu_7_meshes0.inc"
};
static Sint16 fish_ryu_7_meshes1[5] = {
#include "stg01/fish_models/fish_ryu_7_meshes1.inc"
};
static Sint16 fish_ryu_7_meshes2[5] = {
#include "stg01/fish_models/fish_ryu_7_meshes2.inc"
};
static NJS_COLOR fish_ryu_7_uvs0[14] = {
#include "stg01/fish_models/fish_ryu_7_uvs0.inc"
};
static NJS_COLOR fish_ryu_7_uvs1[4] = {
#include "stg01/fish_models/fish_ryu_7_uvs1.inc"
};
static NJS_COLOR fish_ryu_7_uvs2[4] = {
#include "stg01/fish_models/fish_ryu_7_uvs2.inc"
};
static NJS_MESHSET fish_ryu_7_meshsets[3] = {
#include "stg01/fish_models/fish_ryu_7_meshsets.inc"
};
static NJS_POINT3 fish_ryu_7_points[20] = {
#include "stg01/fish_models/fish_ryu_7_points.inc"
};
static NJS_VECTOR fish_ryu_7_normals[20] = {
#include "stg01/fish_models/fish_ryu_7_normals.inc"
};
static NJS_MODEL fish_ryu_7_model = {
#include "stg01/fish_models/fish_ryu_7_model.inc"
};
static NJS_OBJECT fish_ryu_7 = {
#include "stg01/fish_models/fish_ryu_7.inc"
};
static NJS_MATERIAL fish_ryu_6_materials[3] = {
#include "stg01/fish_models/fish_ryu_6_materials.inc"
};
static Sint16 fish_ryu_6_meshes0[15] = {
#include "stg01/fish_models/fish_ryu_6_meshes0.inc"
};
static Sint16 fish_ryu_6_meshes1[5] = {
#include "stg01/fish_models/fish_ryu_6_meshes1.inc"
};
static Sint16 fish_ryu_6_meshes2[5] = {
#include "stg01/fish_models/fish_ryu_6_meshes2.inc"
};
static NJS_COLOR fish_ryu_6_uvs0[14] = {
#include "stg01/fish_models/fish_ryu_6_uvs0.inc"
};
static NJS_COLOR fish_ryu_6_uvs1[4] = {
#include "stg01/fish_models/fish_ryu_6_uvs1.inc"
};
static NJS_COLOR fish_ryu_6_uvs2[4] = {
#include "stg01/fish_models/fish_ryu_6_uvs2.inc"
};
static NJS_MESHSET fish_ryu_6_meshsets[3] = {
#include "stg01/fish_models/fish_ryu_6_meshsets.inc"
};
static NJS_POINT3 fish_ryu_6_points[20] = {
#include "stg01/fish_models/fish_ryu_6_points.inc"
};
static NJS_VECTOR fish_ryu_6_normals[20] = {
#include "stg01/fish_models/fish_ryu_6_normals.inc"
};
static NJS_MODEL fish_ryu_6_model = {
#include "stg01/fish_models/fish_ryu_6_model.inc"
};
static NJS_OBJECT fish_ryu_6 = {
#include "stg01/fish_models/fish_ryu_6.inc"
};
static NJS_MATERIAL fish_ryu_5_materials[3] = {
#include "stg01/fish_models/fish_ryu_5_materials.inc"
};
static Sint16 fish_ryu_5_meshes0[15] = {
#include "stg01/fish_models/fish_ryu_5_meshes0.inc"
};
static Sint16 fish_ryu_5_meshes1[5] = {
#include "stg01/fish_models/fish_ryu_5_meshes1.inc"
};
static Sint16 fish_ryu_5_meshes2[5] = {
#include "stg01/fish_models/fish_ryu_5_meshes2.inc"
};
static NJS_COLOR fish_ryu_5_uvs0[14] = {
#include "stg01/fish_models/fish_ryu_5_uvs0.inc"
};
static NJS_COLOR fish_ryu_5_uvs1[4] = {
#include "stg01/fish_models/fish_ryu_5_uvs1.inc"
};
static NJS_COLOR fish_ryu_5_uvs2[4] = {
#include "stg01/fish_models/fish_ryu_5_uvs2.inc"
};
static NJS_MESHSET fish_ryu_5_meshsets[3] = {
#include "stg01/fish_models/fish_ryu_5_meshsets.inc"
};
static NJS_POINT3 fish_ryu_5_points[20] = {
#include "stg01/fish_models/fish_ryu_5_points.inc"
};
static NJS_VECTOR fish_ryu_5_normals[20] = {
#include "stg01/fish_models/fish_ryu_5_normals.inc"
};
static NJS_MODEL fish_ryu_5_model = {
#include "stg01/fish_models/fish_ryu_5_model.inc"
};
static NJS_OBJECT fish_ryu_5 = {
#include "stg01/fish_models/fish_ryu_5.inc"
};
static NJS_MATERIAL fish_ryu_4_materials[3] = {
#include "stg01/fish_models/fish_ryu_4_materials.inc"
};
static Sint16 fish_ryu_4_meshes0[15] = {
#include "stg01/fish_models/fish_ryu_4_meshes0.inc"
};
static Sint16 fish_ryu_4_meshes1[5] = {
#include "stg01/fish_models/fish_ryu_4_meshes1.inc"
};
static Sint16 fish_ryu_4_meshes2[5] = {
#include "stg01/fish_models/fish_ryu_4_meshes2.inc"
};
static NJS_COLOR fish_ryu_4_uvs0[14] = {
#include "stg01/fish_models/fish_ryu_4_uvs0.inc"
};
static NJS_COLOR fish_ryu_4_uvs1[4] = {
#include "stg01/fish_models/fish_ryu_4_uvs1.inc"
};
static NJS_COLOR fish_ryu_4_uvs2[4] = {
#include "stg01/fish_models/fish_ryu_4_uvs2.inc"
};
static NJS_MESHSET fish_ryu_4_meshsets[3] = {
#include "stg01/fish_models/fish_ryu_4_meshsets.inc"
};
static NJS_POINT3 fish_ryu_4_points[20] = {
#include "stg01/fish_models/fish_ryu_4_points.inc"
};
static NJS_VECTOR fish_ryu_4_normals[20] = {
#include "stg01/fish_models/fish_ryu_4_normals.inc"
};
static NJS_MODEL fish_ryu_4_model = {
#include "stg01/fish_models/fish_ryu_4_model.inc"
};
static NJS_OBJECT fish_ryu_4 = {
#include "stg01/fish_models/fish_ryu_4.inc"
};
static NJS_MATERIAL fish_ryu_3_materials[3] = {
#include "stg01/fish_models/fish_ryu_3_materials.inc"
};
static Sint16 fish_ryu_3_meshes0[15] = {
#include "stg01/fish_models/fish_ryu_3_meshes0.inc"
};
static Sint16 fish_ryu_3_meshes1[5] = {
#include "stg01/fish_models/fish_ryu_3_meshes1.inc"
};
static Sint16 fish_ryu_3_meshes2[5] = {
#include "stg01/fish_models/fish_ryu_3_meshes2.inc"
};
static NJS_COLOR fish_ryu_3_uvs0[14] = {
#include "stg01/fish_models/fish_ryu_3_uvs0.inc"
};
static NJS_COLOR fish_ryu_3_uvs1[4] = {
#include "stg01/fish_models/fish_ryu_3_uvs1.inc"
};
static NJS_COLOR fish_ryu_3_uvs2[4] = {
#include "stg01/fish_models/fish_ryu_3_uvs2.inc"
};
static NJS_MESHSET fish_ryu_3_meshsets[3] = {
#include "stg01/fish_models/fish_ryu_3_meshsets.inc"
};
static NJS_POINT3 fish_ryu_3_points[20] = {
#include "stg01/fish_models/fish_ryu_3_points.inc"
};
static NJS_VECTOR fish_ryu_3_normals[20] = {
#include "stg01/fish_models/fish_ryu_3_normals.inc"
};
static NJS_MODEL fish_ryu_3_model = {
#include "stg01/fish_models/fish_ryu_3_model.inc"
};
static NJS_OBJECT fish_ryu_3 = {
#include "stg01/fish_models/fish_ryu_3.inc"
};
static NJS_MATERIAL fish_ryu_2_materials[2] = {
#include "stg01/fish_models/fish_ryu_2_materials.inc"
};
static Sint16 fish_ryu_2_meshes0[14] = {
#include "stg01/fish_models/fish_ryu_2_meshes0.inc"
};
static Sint16 fish_ryu_2_meshes1[14] = {
#include "stg01/fish_models/fish_ryu_2_meshes1.inc"
};
static NJS_COLOR fish_ryu_2_uvs0[12] = {
#include "stg01/fish_models/fish_ryu_2_uvs0.inc"
};
static NJS_COLOR fish_ryu_2_uvs1[11] = {
#include "stg01/fish_models/fish_ryu_2_uvs1.inc"
};
static NJS_MESHSET fish_ryu_2_meshsets[2] = {
#include "stg01/fish_models/fish_ryu_2_meshsets.inc"
};
static NJS_POINT3 fish_ryu_2_points[9] = {
#include "stg01/fish_models/fish_ryu_2_points.inc"
};
static NJS_VECTOR fish_ryu_2_normals[9] = {
#include "stg01/fish_models/fish_ryu_2_normals.inc"
};
static NJS_MODEL fish_ryu_2_model = {
#include "stg01/fish_models/fish_ryu_2_model.inc"
};
static NJS_OBJECT fish_ryu_2 = {
#include "stg01/fish_models/fish_ryu_2.inc"
};
static NJS_MATERIAL fish_ryu_1_materials[10] = {
#include "stg01/fish_models/fish_ryu_1_materials.inc"
};
static Sint16 fish_ryu_1_meshes0[16] = {
#include "stg01/fish_models/fish_ryu_1_meshes0.inc"
};
static Sint16 fish_ryu_1_meshes1[46] = {
#include "stg01/fish_models/fish_ryu_1_meshes1.inc"
};
static Sint16 fish_ryu_1_meshes2[28] = {
#include "stg01/fish_models/fish_ryu_1_meshes2.inc"
};
static Sint16 fish_ryu_1_meshes3[61] = {
#include "stg01/fish_models/fish_ryu_1_meshes3.inc"
};
static Sint16 fish_ryu_1_meshes4[5] = {
#include "stg01/fish_models/fish_ryu_1_meshes4.inc"
};
static Sint16 fish_ryu_1_meshes5[17] = {
#include "stg01/fish_models/fish_ryu_1_meshes5.inc"
};
static Sint16 fish_ryu_1_meshes6[20] = {
#include "stg01/fish_models/fish_ryu_1_meshes6.inc"
};
static Sint16 fish_ryu_1_meshes7[30] = {
#include "stg01/fish_models/fish_ryu_1_meshes7.inc"
};
static Sint16 fish_ryu_1_meshes8[10] = {
#include "stg01/fish_models/fish_ryu_1_meshes8.inc"
};
static Sint16 fish_ryu_1_meshes9[11] = {
#include "stg01/fish_models/fish_ryu_1_meshes9.inc"
};
static NJS_COLOR fish_ryu_1_uvs0[13] = {
#include "stg01/fish_models/fish_ryu_1_uvs0.inc"
};
static NJS_COLOR fish_ryu_1_uvs1[38] = {
#include "stg01/fish_models/fish_ryu_1_uvs1.inc"
};
static NJS_COLOR fish_ryu_1_uvs2[24] = {
#include "stg01/fish_models/fish_ryu_1_uvs2.inc"
};
static NJS_COLOR fish_ryu_1_uvs3[51] = {
#include "stg01/fish_models/fish_ryu_1_uvs3.inc"
};
static NJS_COLOR fish_ryu_1_uvs4[4] = {
#include "stg01/fish_models/fish_ryu_1_uvs4.inc"
};
static NJS_COLOR fish_ryu_1_uvs5[14] = {
#include "stg01/fish_models/fish_ryu_1_uvs5.inc"
};
static NJS_COLOR fish_ryu_1_uvs6[16] = {
#include "stg01/fish_models/fish_ryu_1_uvs6.inc"
};
static NJS_COLOR fish_ryu_1_uvs7[24] = {
#include "stg01/fish_models/fish_ryu_1_uvs7.inc"
};
static NJS_COLOR fish_ryu_1_uvs8[8] = {
#include "stg01/fish_models/fish_ryu_1_uvs8.inc"
};
static NJS_COLOR fish_ryu_1_uvs9[9] = {
#include "stg01/fish_models/fish_ryu_1_uvs9.inc"
};
static NJS_MESHSET fish_ryu_1_meshsets[10] = {
#include "stg01/fish_models/fish_ryu_1_meshsets.inc"
};
static NJS_POINT3 fish_ryu_1_points[98] = {
#include "stg01/fish_models/fish_ryu_1_points.inc"
};
static NJS_VECTOR fish_ryu_1_normals[98] = {
#include "stg01/fish_models/fish_ryu_1_normals.inc"
};
static NJS_MODEL fish_ryu_1_model = {
#include "stg01/fish_models/fish_ryu_1_model.inc"
};
static NJS_OBJECT fish_ryu_1 = {
#include "stg01/fish_models/fish_ryu_1.inc"
};
NJS_OBJECT fish_ryu_object = {
#include "stg01/fish_models/fish_ryu_object.inc"
};

static NJS_MKEY_A fish_sb_swim_ang1[16] = {
#include "stg01/fish_models/fish_sb_swim_ang1.inc"
};
static NJS_MKEY_A fish_sb_swim_ang2[16] = {
#include "stg01/fish_models/fish_sb_swim_ang2.inc"
};
static NJS_MKEY_A fish_sb_swim_ang3[16] = {
#include "stg01/fish_models/fish_sb_swim_ang3.inc"
};
static NJS_MKEY_A fish_sb_swim_ang4[16] = {
#include "stg01/fish_models/fish_sb_swim_ang4.inc"
};
static NJS_MKEY_A fish_sb_swim_ang9[16] = {
#include "stg01/fish_models/fish_sb_swim_ang9.inc"
};
static NJS_MKEY_A fish_sb_swim_ang10[16] = {
#include "stg01/fish_models/fish_sb_swim_ang10.inc"
};
static NJS_MKEY_A fish_sb_swim_ang14[16] = {
#include "stg01/fish_models/fish_sb_swim_ang14.inc"
};
static NJS_MKEY_A fish_sb_swim_ang18[16] = {
#include "stg01/fish_models/fish_sb_swim_ang18.inc"
};
static NJS_MKEY_A fish_sb_swim_ang22[16] = {
#include "stg01/fish_models/fish_sb_swim_ang22.inc"
};
static NJS_MDATA2 fish_sb_swim_mdata[26] = {
#include "stg01/fish_models/fish_sb_swim_mdata.inc"
};
static NJS_MOTION fish_sb_swim_motion = {
#include "stg01/fish_models/fish_sb_swim_motion.inc"
};
NJS_ACTION fish_sb_swim_action = {
#include "stg01/fish_models/fish_sb_swim_action.inc"
};

static NJS_MKEY_A fish_sb_bite_ang1[16] = {
#include "stg01/fish_models/fish_sb_bite_ang1.inc"
};
static NJS_MKEY_A fish_sb_bite_ang2[16] = {
#include "stg01/fish_models/fish_sb_bite_ang2.inc"
};
static NJS_MKEY_A fish_sb_bite_ang3[16] = {
#include "stg01/fish_models/fish_sb_bite_ang3.inc"
};
static NJS_MKEY_A fish_sb_bite_ang4[16] = {
#include "stg01/fish_models/fish_sb_bite_ang4.inc"
};
static NJS_MKEY_A fish_sb_bite_ang10[16] = {
#include "stg01/fish_models/fish_sb_bite_ang10.inc"
};
static NJS_MKEY_A fish_sb_bite_ang14[16] = {
#include "stg01/fish_models/fish_sb_bite_ang14.inc"
};
static NJS_MKEY_A fish_sb_bite_ang18[16] = {
#include "stg01/fish_models/fish_sb_bite_ang18.inc"
};
static NJS_MKEY_A fish_sb_bite_ang22[16] = {
#include "stg01/fish_models/fish_sb_bite_ang22.inc"
};
static NJS_MDATA2 fish_sb_bite_mdata[26] = {
#include "stg01/fish_models/fish_sb_bite_mdata.inc"
};
static NJS_MOTION fish_sb_bite_motion = {
#include "stg01/fish_models/fish_sb_bite_motion.inc"
};
NJS_ACTION fish_sb_bite_action = {
#include "stg01/fish_models/fish_sb_bite_action.inc"
};

static NJS_MKEY_A fish_sb_hooked_ang1[16] = {
#include "stg01/fish_models/fish_sb_hooked_ang1.inc"
};
static NJS_MKEY_A fish_sb_hooked_ang2[16] = {
#include "stg01/fish_models/fish_sb_hooked_ang2.inc"
};
static NJS_MKEY_A fish_sb_hooked_ang3[16] = {
#include "stg01/fish_models/fish_sb_hooked_ang3.inc"
};
static NJS_MKEY_A fish_sb_hooked_ang4[16] = {
#include "stg01/fish_models/fish_sb_hooked_ang4.inc"
};
static NJS_MKEY_A fish_sb_hooked_ang10[16] = {
#include "stg01/fish_models/fish_sb_hooked_ang10.inc"
};
static NJS_MKEY_A fish_sb_hooked_ang14[16] = {
#include "stg01/fish_models/fish_sb_hooked_ang14.inc"
};
static NJS_MKEY_A fish_sb_hooked_ang18[16] = {
#include "stg01/fish_models/fish_sb_hooked_ang18.inc"
};
static NJS_MKEY_A fish_sb_hooked_ang22[16] = {
#include "stg01/fish_models/fish_sb_hooked_ang22.inc"
};
static NJS_MDATA2 fish_sb_hooked_mdata[26] = {
#include "stg01/fish_models/fish_sb_hooked_mdata.inc"
};
static NJS_MOTION fish_sb_hooked_motion = {
#include "stg01/fish_models/fish_sb_hooked_motion.inc"
};
NJS_ACTION fish_sb_hooked_action = {
#include "stg01/fish_models/fish_sb_hooked_action.inc"
};

/* a fourth action of fish_sb_object, and its motion */
static NJS_MKEY_A dat_0cad7d7c[32] = {
#include "stg01/fish_models/dat_0cad7d7c.inc"
};
static NJS_MKEY_A dat_0cad7f7c[32] = {
#include "stg01/fish_models/dat_0cad7f7c.inc"
};
static NJS_MKEY_A dat_0cad817c[32] = {
#include "stg01/fish_models/dat_0cad817c.inc"
};
static NJS_MKEY_A dat_0cad837c[32] = {
#include "stg01/fish_models/dat_0cad837c.inc"
};
static NJS_MKEY_A dat_0cad857c[32] = {
#include "stg01/fish_models/dat_0cad857c.inc"
};
static NJS_MKEY_A dat_0cad877c[32] = {
#include "stg01/fish_models/dat_0cad877c.inc"
};
static NJS_MKEY_A dat_0cad897c[32] = {
#include "stg01/fish_models/dat_0cad897c.inc"
};
static NJS_MKEY_A dat_0cad8b7c[32] = {
#include "stg01/fish_models/dat_0cad8b7c.inc"
};
static NJS_MDATA2 dat_0cad8d7c[26] = {
#include "stg01/fish_models/dat_0cad8d7c.inc"
};
static NJS_MOTION dat_0cad8f1c = {
#include "stg01/fish_models/dat_0cad8f1c.inc"
};
static NJS_ACTION dat_0cad8f28 = {
#include "stg01/fish_models/dat_0cad8f28.inc"
};

static NJS_MKEY_A fish_an_swim_ang1[16] = {
#include "stg01/fish_models/fish_an_swim_ang1.inc"
};
static NJS_MKEY_A fish_an_swim_ang2[16] = {
#include "stg01/fish_models/fish_an_swim_ang2.inc"
};
static NJS_MKEY_A fish_an_swim_ang3[16] = {
#include "stg01/fish_models/fish_an_swim_ang3.inc"
};
static NJS_MKEY_A fish_an_swim_ang8[16] = {
#include "stg01/fish_models/fish_an_swim_ang8.inc"
};
static NJS_MKEY_A fish_an_swim_ang9[16] = {
#include "stg01/fish_models/fish_an_swim_ang9.inc"
};
static NJS_MKEY_A fish_an_swim_ang10[16] = {
#include "stg01/fish_models/fish_an_swim_ang10.inc"
};
static NJS_MDATA2 fish_an_swim_mdata[11] = {
#include "stg01/fish_models/fish_an_swim_mdata.inc"
};
static NJS_MOTION fish_an_swim_motion = {
#include "stg01/fish_models/fish_an_swim_motion.inc"
};
NJS_ACTION fish_an_swim_action = {
#include "stg01/fish_models/fish_an_swim_action.inc"
};

static NJS_MKEY_A fish_an_bite_ang1[16] = {
#include "stg01/fish_models/fish_an_bite_ang1.inc"
};
static NJS_MKEY_A fish_an_bite_ang2[16] = {
#include "stg01/fish_models/fish_an_bite_ang2.inc"
};
static NJS_MKEY_A fish_an_bite_ang3[16] = {
#include "stg01/fish_models/fish_an_bite_ang3.inc"
};
static NJS_MKEY_A fish_an_bite_ang7[16] = {
#include "stg01/fish_models/fish_an_bite_ang7.inc"
};
static NJS_MKEY_A fish_an_bite_ang8[16] = {
#include "stg01/fish_models/fish_an_bite_ang8.inc"
};
static NJS_MKEY_A fish_an_bite_ang9[16] = {
#include "stg01/fish_models/fish_an_bite_ang9.inc"
};
static NJS_MKEY_A fish_an_bite_ang10[16] = {
#include "stg01/fish_models/fish_an_bite_ang10.inc"
};
static NJS_MDATA2 fish_an_bite_mdata[11] = {
#include "stg01/fish_models/fish_an_bite_mdata.inc"
};
static NJS_MOTION fish_an_bite_motion = {
#include "stg01/fish_models/fish_an_bite_motion.inc"
};
NJS_ACTION fish_an_bite_action = {
#include "stg01/fish_models/fish_an_bite_action.inc"
};

static NJS_MKEY_A fish_an_hooked_ang1[16] = {
#include "stg01/fish_models/fish_an_hooked_ang1.inc"
};
static NJS_MKEY_A fish_an_hooked_ang2[16] = {
#include "stg01/fish_models/fish_an_hooked_ang2.inc"
};
static NJS_MKEY_A fish_an_hooked_ang3[16] = {
#include "stg01/fish_models/fish_an_hooked_ang3.inc"
};
static NJS_MKEY_A fish_an_hooked_ang7[16] = {
#include "stg01/fish_models/fish_an_hooked_ang7.inc"
};
static NJS_MKEY_A fish_an_hooked_ang8[16] = {
#include "stg01/fish_models/fish_an_hooked_ang8.inc"
};
static NJS_MKEY_A fish_an_hooked_ang9[16] = {
#include "stg01/fish_models/fish_an_hooked_ang9.inc"
};
static NJS_MKEY_A fish_an_hooked_ang10[16] = {
#include "stg01/fish_models/fish_an_hooked_ang10.inc"
};
static NJS_MDATA2 fish_an_hooked_mdata[11] = {
#include "stg01/fish_models/fish_an_hooked_mdata.inc"
};
static NJS_MOTION fish_an_hooked_motion = {
#include "stg01/fish_models/fish_an_hooked_motion.inc"
};
NJS_ACTION fish_an_hooked_action = {
#include "stg01/fish_models/fish_an_hooked_action.inc"
};

static NJS_MKEY_A fish_an_ke_swim_ang1[16] = {
#include "stg01/fish_models/fish_an_ke_swim_ang1.inc"
};
static NJS_MKEY_A fish_an_ke_swim_ang2[16] = {
#include "stg01/fish_models/fish_an_ke_swim_ang2.inc"
};
static NJS_MKEY_A fish_an_ke_swim_ang3[16] = {
#include "stg01/fish_models/fish_an_ke_swim_ang3.inc"
};
static NJS_MKEY_A fish_an_ke_swim_ang4[16] = {
#include "stg01/fish_models/fish_an_ke_swim_ang4.inc"
};
static NJS_MDATA2 fish_an_ke_swim_mdata[5] = {
#include "stg01/fish_models/fish_an_ke_swim_mdata.inc"
};
static NJS_MOTION fish_an_ke_swim_motion = {
#include "stg01/fish_models/fish_an_ke_swim_motion.inc"
};
NJS_ACTION fish_an_ke_swim_action = {
#include "stg01/fish_models/fish_an_ke_swim_action.inc"
};

static NJS_MKEY_A fish_an_ke_bite_ang0[16] = {
#include "stg01/fish_models/fish_an_ke_bite_ang0.inc"
};
static NJS_MKEY_A fish_an_ke_bite_ang1[16] = {
#include "stg01/fish_models/fish_an_ke_bite_ang1.inc"
};
static NJS_MKEY_A fish_an_ke_bite_ang2[16] = {
#include "stg01/fish_models/fish_an_ke_bite_ang2.inc"
};
static NJS_MKEY_A fish_an_ke_bite_ang3[16] = {
#include "stg01/fish_models/fish_an_ke_bite_ang3.inc"
};
static NJS_MKEY_A fish_an_ke_bite_ang4[16] = {
#include "stg01/fish_models/fish_an_ke_bite_ang4.inc"
};
static NJS_MDATA2 fish_an_ke_bite_mdata[5] = {
#include "stg01/fish_models/fish_an_ke_bite_mdata.inc"
};
static NJS_MOTION fish_an_ke_bite_motion = {
#include "stg01/fish_models/fish_an_ke_bite_motion.inc"
};
NJS_ACTION fish_an_ke_bite_action = {
#include "stg01/fish_models/fish_an_ke_bite_action.inc"
};

static NJS_MKEY_A fish_an_ke_hooked_ang0[16] = {
#include "stg01/fish_models/fish_an_ke_hooked_ang0.inc"
};
static NJS_MKEY_A fish_an_ke_hooked_ang1[16] = {
#include "stg01/fish_models/fish_an_ke_hooked_ang1.inc"
};
static NJS_MKEY_A fish_an_ke_hooked_ang2[16] = {
#include "stg01/fish_models/fish_an_ke_hooked_ang2.inc"
};
static NJS_MKEY_A fish_an_ke_hooked_ang3[16] = {
#include "stg01/fish_models/fish_an_ke_hooked_ang3.inc"
};
static NJS_MKEY_A fish_an_ke_hooked_ang4[16] = {
#include "stg01/fish_models/fish_an_ke_hooked_ang4.inc"
};
static NJS_MDATA2 fish_an_ke_hooked_mdata[5] = {
#include "stg01/fish_models/fish_an_ke_hooked_mdata.inc"
};
static NJS_MOTION fish_an_ke_hooked_motion = {
#include "stg01/fish_models/fish_an_ke_hooked_motion.inc"
};
NJS_ACTION fish_an_ke_hooked_action = {
#include "stg01/fish_models/fish_an_ke_hooked_action.inc"
};

static NJS_MKEY_A fish_una_swim_ang1[16] = {
#include "stg01/fish_models/fish_una_swim_ang1.inc"
};
static NJS_MKEY_A fish_una_swim_ang2[16] = {
#include "stg01/fish_models/fish_una_swim_ang2.inc"
};
static NJS_MKEY_A fish_una_swim_ang3[16] = {
#include "stg01/fish_models/fish_una_swim_ang3.inc"
};
static NJS_MKEY_A fish_una_swim_ang4[16] = {
#include "stg01/fish_models/fish_una_swim_ang4.inc"
};
static NJS_MKEY_A fish_una_swim_ang5[16] = {
#include "stg01/fish_models/fish_una_swim_ang5.inc"
};
static NJS_MKEY_A fish_una_swim_ang6[16] = {
#include "stg01/fish_models/fish_una_swim_ang6.inc"
};
static NJS_MKEY_A fish_una_swim_ang7[16] = {
#include "stg01/fish_models/fish_una_swim_ang7.inc"
};
static NJS_MKEY_A fish_una_swim_ang8[16] = {
#include "stg01/fish_models/fish_una_swim_ang8.inc"
};
static NJS_MKEY_A fish_una_swim_ang9[16] = {
#include "stg01/fish_models/fish_una_swim_ang9.inc"
};
static NJS_MKEY_A fish_una_swim_ang10[16] = {
#include "stg01/fish_models/fish_una_swim_ang10.inc"
};
static NJS_MDATA2 fish_una_swim_mdata[11] = {
#include "stg01/fish_models/fish_una_swim_mdata.inc"
};
static NJS_MOTION fish_una_swim_motion = {
#include "stg01/fish_models/fish_una_swim_motion.inc"
};
NJS_ACTION fish_una_swim_action = {
#include "stg01/fish_models/fish_una_swim_action.inc"
};

static NJS_MKEY_A fish_una_bite_ang1[16] = {
#include "stg01/fish_models/fish_una_bite_ang1.inc"
};
static NJS_MKEY_A fish_una_bite_ang2[16] = {
#include "stg01/fish_models/fish_una_bite_ang2.inc"
};
static NJS_MKEY_A fish_una_bite_ang3[16] = {
#include "stg01/fish_models/fish_una_bite_ang3.inc"
};
static NJS_MKEY_A fish_una_bite_ang4[16] = {
#include "stg01/fish_models/fish_una_bite_ang4.inc"
};
static NJS_MKEY_A fish_una_bite_ang5[16] = {
#include "stg01/fish_models/fish_una_bite_ang5.inc"
};
static NJS_MKEY_A fish_una_bite_ang6[16] = {
#include "stg01/fish_models/fish_una_bite_ang6.inc"
};
static NJS_MKEY_A fish_una_bite_ang7[16] = {
#include "stg01/fish_models/fish_una_bite_ang7.inc"
};
static NJS_MKEY_A fish_una_bite_ang8[16] = {
#include "stg01/fish_models/fish_una_bite_ang8.inc"
};
static NJS_MKEY_A fish_una_bite_ang9[16] = {
#include "stg01/fish_models/fish_una_bite_ang9.inc"
};
static NJS_MKEY_A fish_una_bite_ang10[16] = {
#include "stg01/fish_models/fish_una_bite_ang10.inc"
};
static NJS_MDATA2 fish_una_bite_mdata[11] = {
#include "stg01/fish_models/fish_una_bite_mdata.inc"
};
static NJS_MOTION fish_una_bite_motion = {
#include "stg01/fish_models/fish_una_bite_motion.inc"
};
NJS_ACTION fish_una_bite_action = {
#include "stg01/fish_models/fish_una_bite_action.inc"
};

static NJS_MKEY_A fish_una_hooked_ang1[16] = {
#include "stg01/fish_models/fish_una_hooked_ang1.inc"
};
static NJS_MKEY_A fish_una_hooked_ang2[16] = {
#include "stg01/fish_models/fish_una_hooked_ang2.inc"
};
static NJS_MKEY_A fish_una_hooked_ang3[16] = {
#include "stg01/fish_models/fish_una_hooked_ang3.inc"
};
static NJS_MKEY_A fish_una_hooked_ang4[16] = {
#include "stg01/fish_models/fish_una_hooked_ang4.inc"
};
static NJS_MKEY_A fish_una_hooked_ang5[16] = {
#include "stg01/fish_models/fish_una_hooked_ang5.inc"
};
static NJS_MKEY_A fish_una_hooked_ang6[16] = {
#include "stg01/fish_models/fish_una_hooked_ang6.inc"
};
static NJS_MKEY_A fish_una_hooked_ang7[16] = {
#include "stg01/fish_models/fish_una_hooked_ang7.inc"
};
static NJS_MKEY_A fish_una_hooked_ang8[16] = {
#include "stg01/fish_models/fish_una_hooked_ang8.inc"
};
static NJS_MKEY_A fish_una_hooked_ang9[16] = {
#include "stg01/fish_models/fish_una_hooked_ang9.inc"
};
static NJS_MKEY_A fish_una_hooked_ang10[16] = {
#include "stg01/fish_models/fish_una_hooked_ang10.inc"
};
static NJS_MDATA2 fish_una_hooked_mdata[11] = {
#include "stg01/fish_models/fish_una_hooked_mdata.inc"
};
static NJS_MOTION fish_una_hooked_motion = {
#include "stg01/fish_models/fish_una_hooked_motion.inc"
};
NJS_ACTION fish_una_hooked_action = {
#include "stg01/fish_models/fish_una_hooked_action.inc"
};

#pragma section
