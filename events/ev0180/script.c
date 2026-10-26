/* EV0180: the Egg Carrier flies by as Sonic comes to Red Mountain */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0180/script.h"
#include "event/play.h"
#include "shared/move.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 11
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
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 13
#include "ev0180/script/text.inc"

/* the Egg Carrier's prop slot */
#define EGG_CARRIER 0

static NJS_TEXNAME ev0180_egg_carrier_textures[21] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},
};
static NJS_TEXLIST ev0180_egg_carrier_texlist = {
#include "ev0180/script/ev0180_egg_carrier_texlist.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_11_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_11_materials.inc"
};
static Sint16 ev0180_egg_carrier_11_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_11_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_11_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_11_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_11_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_11_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_11_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_11_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_11_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_11_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_11_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_11_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_11_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_11_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_11_model = {
#include "ev0180/script/ev0180_egg_carrier_11_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_11 = {
#include "ev0180/script/ev0180_egg_carrier_11.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_10_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_10_materials.inc"
};
static Sint16 ev0180_egg_carrier_10_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_10_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_10_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_10_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_10_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_10_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_10_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_10_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_10_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_10_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_10_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_10_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_10_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_10_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_10_model = {
#include "ev0180/script/ev0180_egg_carrier_10_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_10 = {
#include "ev0180/script/ev0180_egg_carrier_10.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_9_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_9_materials.inc"
};
static Sint16 ev0180_egg_carrier_9_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_9_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_9_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_9_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_9_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_9_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_9_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_9_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_9_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_9_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_9_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_9_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_9_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_9_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_9_model = {
#include "ev0180/script/ev0180_egg_carrier_9_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_9 = {
#include "ev0180/script/ev0180_egg_carrier_9.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_8_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_8_materials.inc"
};
static Sint16 ev0180_egg_carrier_8_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_8_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_8_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_8_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_8_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_8_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_8_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_8_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_8_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_8_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_8_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_8_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_8_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_8_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_8_model = {
#include "ev0180/script/ev0180_egg_carrier_8_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_8 = {
#include "ev0180/script/ev0180_egg_carrier_8.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_7_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_7_materials.inc"
};
static Sint16 ev0180_egg_carrier_7_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_7_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_7_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_7_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_7_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_7_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_7_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_7_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_7_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_7_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_7_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_7_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_7_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_7_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_7_model = {
#include "ev0180/script/ev0180_egg_carrier_7_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_7 = {
#include "ev0180/script/ev0180_egg_carrier_7.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_6_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_6_materials.inc"
};
static Sint16 ev0180_egg_carrier_6_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_6_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_6_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_6_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_6_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_6_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_6_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_6_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_6_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_6_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_6_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_6_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_6_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_6_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_6_model = {
#include "ev0180/script/ev0180_egg_carrier_6_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_6 = {
#include "ev0180/script/ev0180_egg_carrier_6.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_5_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_5_materials.inc"
};
static Sint16 ev0180_egg_carrier_5_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_5_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_5_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_5_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_5_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_5_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_5_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_5_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_5_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_5_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_5_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_5_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_5_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_5_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_5_model = {
#include "ev0180/script/ev0180_egg_carrier_5_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_5 = {
#include "ev0180/script/ev0180_egg_carrier_5.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_4_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_4_materials.inc"
};
static Sint16 ev0180_egg_carrier_4_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_4_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_4_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_4_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_4_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_4_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_4_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_4_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_4_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_4_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_4_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_4_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_4_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_4_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_4_model = {
#include "ev0180/script/ev0180_egg_carrier_4_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_4 = {
#include "ev0180/script/ev0180_egg_carrier_4.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_3_materials[2] = {
#include "ev0180/script/ev0180_egg_carrier_3_materials.inc"
};
static Sint16 ev0180_egg_carrier_3_meshes0[10] = {
#include "ev0180/script/ev0180_egg_carrier_3_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_3_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_3_meshes1.inc"
};
static NJS_COLOR ev0180_egg_carrier_3_uvs0[8] = {
#include "ev0180/script/ev0180_egg_carrier_3_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_3_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_3_uvs1.inc"
};
static NJS_MESHSET ev0180_egg_carrier_3_meshsets[2] = {
#include "ev0180/script/ev0180_egg_carrier_3_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_3_points[13] = {
#include "ev0180/script/ev0180_egg_carrier_3_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_3_normals[13] = {
#include "ev0180/script/ev0180_egg_carrier_3_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_3_model = {
#include "ev0180/script/ev0180_egg_carrier_3_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_3 = {
#include "ev0180/script/ev0180_egg_carrier_3.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_2_materials[7] = {
#include "ev0180/script/ev0180_egg_carrier_2_materials.inc"
};
static Sint16 ev0180_egg_carrier_2_meshes0[681] = {
#include "ev0180/script/ev0180_egg_carrier_2_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_2_meshes1[10] = {
#include "ev0180/script/ev0180_egg_carrier_2_meshes1.inc"
};
static Sint16 ev0180_egg_carrier_2_meshes2[49] = {
#include "ev0180/script/ev0180_egg_carrier_2_meshes2.inc"
};
static Sint16 ev0180_egg_carrier_2_meshes3[20] = {
#include "ev0180/script/ev0180_egg_carrier_2_meshes3.inc"
};
static Sint16 ev0180_egg_carrier_2_meshes4[20] = {
#include "ev0180/script/ev0180_egg_carrier_2_meshes4.inc"
};
static Sint16 ev0180_egg_carrier_2_meshes5[196] = {
#include "ev0180/script/ev0180_egg_carrier_2_meshes5.inc"
};
static Sint16 ev0180_egg_carrier_2_meshes6[38] = {
#include "ev0180/script/ev0180_egg_carrier_2_meshes6.inc"
};
static NJS_COLOR ev0180_egg_carrier_2_uvs0[606] = {
#include "ev0180/script/ev0180_egg_carrier_2_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_2_uvs1[8] = {
#include "ev0180/script/ev0180_egg_carrier_2_uvs1.inc"
};
static NJS_COLOR ev0180_egg_carrier_2_uvs2[42] = {
#include "ev0180/script/ev0180_egg_carrier_2_uvs2.inc"
};
static NJS_COLOR ev0180_egg_carrier_2_uvs3[16] = {
#include "ev0180/script/ev0180_egg_carrier_2_uvs3.inc"
};
static NJS_COLOR ev0180_egg_carrier_2_uvs4[16] = {
#include "ev0180/script/ev0180_egg_carrier_2_uvs4.inc"
};
static NJS_COLOR ev0180_egg_carrier_2_uvs5[168] = {
#include "ev0180/script/ev0180_egg_carrier_2_uvs5.inc"
};
static NJS_COLOR ev0180_egg_carrier_2_uvs6[32] = {
#include "ev0180/script/ev0180_egg_carrier_2_uvs6.inc"
};
static NJS_MESHSET ev0180_egg_carrier_2_meshsets[7] = {
#include "ev0180/script/ev0180_egg_carrier_2_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_2_points[390] = {
#include "ev0180/script/ev0180_egg_carrier_2_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_2_normals[390] = {
#include "ev0180/script/ev0180_egg_carrier_2_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_2_model = {
#include "ev0180/script/ev0180_egg_carrier_2_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_2 = {
#include "ev0180/script/ev0180_egg_carrier_2.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_1_materials[15] = {
#include "ev0180/script/ev0180_egg_carrier_1_materials.inc"
};
static Sint16 ev0180_egg_carrier_1_meshes0[64] = {
#include "ev0180/script/ev0180_egg_carrier_1_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_1_meshes1[237] = {
#include "ev0180/script/ev0180_egg_carrier_1_meshes1.inc"
};
static Sint16 ev0180_egg_carrier_1_meshes2[808] = {
#include "ev0180/script/ev0180_egg_carrier_1_meshes2.inc"
};
static Sint16 ev0180_egg_carrier_1_meshes3[38] = {
#include "ev0180/script/ev0180_egg_carrier_1_meshes3.inc"
};
static NJS_COLOR ev0180_egg_carrier_1_uvs0[54] = {
#include "ev0180/script/ev0180_egg_carrier_1_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_1_uvs1[208] = {
#include "ev0180/script/ev0180_egg_carrier_1_uvs1.inc"
};
static NJS_COLOR ev0180_egg_carrier_1_uvs2[714] = {
#include "ev0180/script/ev0180_egg_carrier_1_uvs2.inc"
};
static NJS_COLOR ev0180_egg_carrier_1_uvs3[36] = {
#include "ev0180/script/ev0180_egg_carrier_1_uvs3.inc"
};
static NJS_MESHSET ev0180_egg_carrier_1_meshsets[4] = {
#include "ev0180/script/ev0180_egg_carrier_1_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_1_points[571] = {
#include "ev0180/script/ev0180_egg_carrier_1_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_1_normals[571] = {
#include "ev0180/script/ev0180_egg_carrier_1_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_1_model = {
#include "ev0180/script/ev0180_egg_carrier_1_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_1 = {
#include "ev0180/script/ev0180_egg_carrier_1.inc"
};
static NJS_MATERIAL ev0180_egg_carrier_0_materials[15] = {
#include "ev0180/script/ev0180_egg_carrier_0_materials.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes0[303] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes0.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes1[14] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes1.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes2[7] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes2.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes3[328] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes3.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes4[142] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes4.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes5[448] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes5.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes6[62] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes6.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes7[72] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes7.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes8[96] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes8.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes9[48] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes9.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes10[22] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes10.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes11[11] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes11.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes12[63] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes12.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes13[18] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes13.inc"
};
static Sint16 ev0180_egg_carrier_0_meshes14[15] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshes14.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs0[261] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs0.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs1[11] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs1.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs2[6] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs2.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs3[286] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs3.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs4[132] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs4.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs5[397] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs5.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs6[52] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs6.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs7[54] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs7.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs8[72] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs8.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs9[44] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs9.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs10[20] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs10.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs11[10] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs11.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs12[55] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs12.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs13[16] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs13.inc"
};
static NJS_COLOR ev0180_egg_carrier_0_uvs14[14] = {
#include "ev0180/script/ev0180_egg_carrier_0_uvs14.inc"
};
static NJS_MESHSET ev0180_egg_carrier_0_meshsets[15] = {
#include "ev0180/script/ev0180_egg_carrier_0_meshsets.inc"
};
static NJS_POINT3 ev0180_egg_carrier_0_points[597] = {
#include "ev0180/script/ev0180_egg_carrier_0_points.inc"
};
static NJS_VECTOR ev0180_egg_carrier_0_normals[597] = {
#include "ev0180/script/ev0180_egg_carrier_0_normals.inc"
};
static NJS_MODEL ev0180_egg_carrier_0_model = {
#include "ev0180/script/ev0180_egg_carrier_0_model.inc"
};
static NJS_OBJECT ev0180_egg_carrier_object = {
#include "ev0180/script/ev0180_egg_carrier_object.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang3[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang3.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl3[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl3.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang4[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang4.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl4[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl4.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang5[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang5.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl5[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl5.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang6[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang6.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl6[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl6.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang7[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang7.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl7[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl7.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang8[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang8.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl8[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl8.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang9[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang9.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl9[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl9.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang10[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang10.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl10[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl10.inc"
};
static NJS_MKEY_A ev0180_egg_carrier_ang11[30] = {
#include "ev0180/script/ev0180_egg_carrier_ang11.inc"
};
static NJS_MKEY_F ev0180_egg_carrier_scl11[30] = {
#include "ev0180/script/ev0180_egg_carrier_scl11.inc"
};
static NJS_MDATA3 ev0180_egg_carrier_mdata[12] = {
#include "ev0180/script/ev0180_egg_carrier_mdata.inc"
};
static NJS_MOTION ev0180_egg_carrier_motion = {
#include "ev0180/script/ev0180_egg_carrier_motion.inc"
};
static NJS_ACTION ev0180_egg_carrier_action = {
#include "ev0180/script/ev0180_egg_carrier_action.inc"
};

/* no code uses this word. it keeps ev0180_textures at its address in the
 * image */
static Uint32 dat_0cb9401c = 0;

void ev0180_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_sound_init(4);
        sound_load_file("e_000d.mlt");
        event_prop_create(-3857.7F, 2400.6F, -2000.0F, 0, 0x1000, 0, 0.1F, 0.1F,
                          0.1F, &ev0180_egg_carrier_object,
                          &ev0180_egg_carrier_texlist, EGG_CARRIER);
        event_prop_set_action(EGG_CARRIER, &ev0180_egg_carrier_action, 0);
        event_prop_play(EGG_CARRIER, 0, 1.0F, EVENT_PROP_FOREVER);
        event_move(event_prop_get(EGG_CARRIER), -1283.7F, 1020.6F, 240.4F,
                   -1478.5F, 1200.5F, -1035.68F, 500);
        event_sound_start(1, 1234, 1800);
        event_sound_fade_volume(1, 106, 1);
        event_wait(1);
        event_set_pos(event_get_player(0), -45.0F, 643.0F, 503.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -184.0F, 510.6F, 699.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0fde, 0x2825, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 400, -33.0F, 672.6F, 589.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 400, 0x08de, 0x2725, 0);
        event_wait(420);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_reset_player(0);
        event_move_stop_all();
        event_prop_kill(EGG_CARRIER);
        event_set_pos(event_get_player(0), 18.0F, 38.0F, 636.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0180_textures[] = {
    {"EC_LIGHT", &ev0180_egg_carrier_texlist},
    {NULL, NULL},
};
