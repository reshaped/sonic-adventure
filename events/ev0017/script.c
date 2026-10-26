/* EV0017, Sonic's story: Tails picks Sonic up in the Tornado 2 to chase the Egg
 * Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0017/script.h"
#include "ev0017/sonic_action_2.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 26
#define TEXT_3_LENGTH 37
#define TEXT_4_LENGTH 15
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 11
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 10
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 13
#define TEXT_25_LENGTH 13
#define TEXT_26_LENGTH 12
#define TEXT_27_LENGTH 13
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 13
#define TEXT_30_LENGTH 13
#define TEXT_31_LENGTH 11
#define TEXT_32_LENGTH 11
#define TEXT_33_LENGTH 11
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 11
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 9
#define TEXT_38_LENGTH 11
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 11
#define TEXT_42_LENGTH 9
#define TEXT_43_LENGTH 11
#define TEXT_44_LENGTH 11
#define TEXT_45_LENGTH 11
#define TEXT_46_LENGTH 11
#define TEXT_47_LENGTH 10
#define TEXT_48_LENGTH 10
#define TEXT_49_LENGTH 10
#define TEXT_50_LENGTH 10
#define TEXT_51_LENGTH 9
#define TEXT_52_LENGTH 10
#define TEXT_53_LENGTH 10
#define TEXT_54_LENGTH 10
#define TEXT_55_LENGTH 10
#define TEXT_56_LENGTH 10
#define TEXT_57_LENGTH 10
#define TEXT_58_LENGTH 10
#define TEXT_59_LENGTH 8
#define TEXT_60_LENGTH 8
#define TEXT_61_LENGTH 8
#define TEXT_62_LENGTH 12
#define TEXT_63_LENGTH 12
#define TEXT_64_LENGTH 12
#define TEXT_65_LENGTH 13
#define TEXT_66_LENGTH 13
#include "ev0017/script/text.inc"

extern NJS_ACTION ev0017_sonic_action_0;
extern NJS_ACTION ev0017_sonic_action_1;

/* Sonic's run and jump for the tornado, one key a step. the stick is pushed 226
 * to 243 out of 255 throughout. A is held for the 55th to 60th keys */
static const struct event_pad_key ev0017_jump_keys[108] = {
#include "ev0017/script/ev0017_jump_keys.inc"
};

static const struct event_pad_recording ev0017_jump = {
    ev0017_jump_keys, sizeof(ev0017_jump_keys) / sizeof(ev0017_jump_keys[0])};

static struct event_path_point ev0017_tornado_path_points[36] = {
#include "ev0017/script/ev0017_tornado_path_points.inc"
};
static struct event_path ev0017_tornado_path = {36, ev0017_tornado_path_points};
static NJS_TEXNAME ev0017_egg_carrier_textures[21] = {
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0},
};
static NJS_TEXLIST ev0017_egg_carrier_texlist = {
#include "ev0017/script/ev0017_egg_carrier_texlist.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_11_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_11_materials.inc"
};
static Sint16 ev0017_egg_carrier_11_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_11_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_11_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_11_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_11_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_11_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_11_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_11_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_11_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_11_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_11_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_11_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_11_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_11_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_11_model = {
#include "ev0017/script/ev0017_egg_carrier_11_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_11 = {
#include "ev0017/script/ev0017_egg_carrier_11.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_10_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_10_materials.inc"
};
static Sint16 ev0017_egg_carrier_10_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_10_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_10_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_10_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_10_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_10_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_10_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_10_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_10_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_10_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_10_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_10_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_10_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_10_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_10_model = {
#include "ev0017/script/ev0017_egg_carrier_10_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_10 = {
#include "ev0017/script/ev0017_egg_carrier_10.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_9_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_9_materials.inc"
};
static Sint16 ev0017_egg_carrier_9_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_9_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_9_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_9_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_9_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_9_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_9_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_9_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_9_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_9_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_9_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_9_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_9_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_9_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_9_model = {
#include "ev0017/script/ev0017_egg_carrier_9_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_9 = {
#include "ev0017/script/ev0017_egg_carrier_9.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_8_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_8_materials.inc"
};
static Sint16 ev0017_egg_carrier_8_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_8_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_8_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_8_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_8_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_8_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_8_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_8_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_8_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_8_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_8_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_8_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_8_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_8_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_8_model = {
#include "ev0017/script/ev0017_egg_carrier_8_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_8 = {
#include "ev0017/script/ev0017_egg_carrier_8.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_7_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_7_materials.inc"
};
static Sint16 ev0017_egg_carrier_7_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_7_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_7_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_7_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_7_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_7_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_7_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_7_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_7_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_7_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_7_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_7_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_7_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_7_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_7_model = {
#include "ev0017/script/ev0017_egg_carrier_7_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_7 = {
#include "ev0017/script/ev0017_egg_carrier_7.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_6_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_6_materials.inc"
};
static Sint16 ev0017_egg_carrier_6_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_6_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_6_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_6_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_6_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_6_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_6_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_6_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_6_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_6_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_6_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_6_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_6_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_6_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_6_model = {
#include "ev0017/script/ev0017_egg_carrier_6_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_6 = {
#include "ev0017/script/ev0017_egg_carrier_6.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_5_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_5_materials.inc"
};
static Sint16 ev0017_egg_carrier_5_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_5_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_5_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_5_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_5_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_5_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_5_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_5_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_5_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_5_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_5_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_5_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_5_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_5_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_5_model = {
#include "ev0017/script/ev0017_egg_carrier_5_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_5 = {
#include "ev0017/script/ev0017_egg_carrier_5.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_4_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_4_materials.inc"
};
static Sint16 ev0017_egg_carrier_4_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_4_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_4_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_4_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_4_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_4_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_4_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_4_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_4_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_4_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_4_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_4_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_4_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_4_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_4_model = {
#include "ev0017/script/ev0017_egg_carrier_4_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_4 = {
#include "ev0017/script/ev0017_egg_carrier_4.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_3_materials[2] = {
#include "ev0017/script/ev0017_egg_carrier_3_materials.inc"
};
static Sint16 ev0017_egg_carrier_3_meshes0[10] = {
#include "ev0017/script/ev0017_egg_carrier_3_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_3_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_3_meshes1.inc"
};
static NJS_COLOR ev0017_egg_carrier_3_uvs0[8] = {
#include "ev0017/script/ev0017_egg_carrier_3_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_3_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_3_uvs1.inc"
};
static NJS_MESHSET ev0017_egg_carrier_3_meshsets[2] = {
#include "ev0017/script/ev0017_egg_carrier_3_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_3_points[13] = {
#include "ev0017/script/ev0017_egg_carrier_3_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_3_normals[13] = {
#include "ev0017/script/ev0017_egg_carrier_3_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_3_model = {
#include "ev0017/script/ev0017_egg_carrier_3_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_3 = {
#include "ev0017/script/ev0017_egg_carrier_3.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_2_materials[7] = {
#include "ev0017/script/ev0017_egg_carrier_2_materials.inc"
};
static Sint16 ev0017_egg_carrier_2_meshes0[681] = {
#include "ev0017/script/ev0017_egg_carrier_2_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_2_meshes1[10] = {
#include "ev0017/script/ev0017_egg_carrier_2_meshes1.inc"
};
static Sint16 ev0017_egg_carrier_2_meshes2[49] = {
#include "ev0017/script/ev0017_egg_carrier_2_meshes2.inc"
};
static Sint16 ev0017_egg_carrier_2_meshes3[20] = {
#include "ev0017/script/ev0017_egg_carrier_2_meshes3.inc"
};
static Sint16 ev0017_egg_carrier_2_meshes4[20] = {
#include "ev0017/script/ev0017_egg_carrier_2_meshes4.inc"
};
static Sint16 ev0017_egg_carrier_2_meshes5[196] = {
#include "ev0017/script/ev0017_egg_carrier_2_meshes5.inc"
};
static Sint16 ev0017_egg_carrier_2_meshes6[38] = {
#include "ev0017/script/ev0017_egg_carrier_2_meshes6.inc"
};
static NJS_COLOR ev0017_egg_carrier_2_uvs0[606] = {
#include "ev0017/script/ev0017_egg_carrier_2_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_2_uvs1[8] = {
#include "ev0017/script/ev0017_egg_carrier_2_uvs1.inc"
};
static NJS_COLOR ev0017_egg_carrier_2_uvs2[42] = {
#include "ev0017/script/ev0017_egg_carrier_2_uvs2.inc"
};
static NJS_COLOR ev0017_egg_carrier_2_uvs3[16] = {
#include "ev0017/script/ev0017_egg_carrier_2_uvs3.inc"
};
static NJS_COLOR ev0017_egg_carrier_2_uvs4[16] = {
#include "ev0017/script/ev0017_egg_carrier_2_uvs4.inc"
};
static NJS_COLOR ev0017_egg_carrier_2_uvs5[168] = {
#include "ev0017/script/ev0017_egg_carrier_2_uvs5.inc"
};
static NJS_COLOR ev0017_egg_carrier_2_uvs6[32] = {
#include "ev0017/script/ev0017_egg_carrier_2_uvs6.inc"
};
static NJS_MESHSET ev0017_egg_carrier_2_meshsets[7] = {
#include "ev0017/script/ev0017_egg_carrier_2_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_2_points[390] = {
#include "ev0017/script/ev0017_egg_carrier_2_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_2_normals[390] = {
#include "ev0017/script/ev0017_egg_carrier_2_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_2_model = {
#include "ev0017/script/ev0017_egg_carrier_2_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_2 = {
#include "ev0017/script/ev0017_egg_carrier_2.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_1_materials[15] = {
#include "ev0017/script/ev0017_egg_carrier_1_materials.inc"
};
static Sint16 ev0017_egg_carrier_1_meshes0[64] = {
#include "ev0017/script/ev0017_egg_carrier_1_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_1_meshes1[237] = {
#include "ev0017/script/ev0017_egg_carrier_1_meshes1.inc"
};
static Sint16 ev0017_egg_carrier_1_meshes2[808] = {
#include "ev0017/script/ev0017_egg_carrier_1_meshes2.inc"
};
static Sint16 ev0017_egg_carrier_1_meshes3[38] = {
#include "ev0017/script/ev0017_egg_carrier_1_meshes3.inc"
};
static NJS_COLOR ev0017_egg_carrier_1_uvs0[54] = {
#include "ev0017/script/ev0017_egg_carrier_1_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_1_uvs1[208] = {
#include "ev0017/script/ev0017_egg_carrier_1_uvs1.inc"
};
static NJS_COLOR ev0017_egg_carrier_1_uvs2[714] = {
#include "ev0017/script/ev0017_egg_carrier_1_uvs2.inc"
};
static NJS_COLOR ev0017_egg_carrier_1_uvs3[36] = {
#include "ev0017/script/ev0017_egg_carrier_1_uvs3.inc"
};
static NJS_MESHSET ev0017_egg_carrier_1_meshsets[4] = {
#include "ev0017/script/ev0017_egg_carrier_1_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_1_points[571] = {
#include "ev0017/script/ev0017_egg_carrier_1_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_1_normals[571] = {
#include "ev0017/script/ev0017_egg_carrier_1_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_1_model = {
#include "ev0017/script/ev0017_egg_carrier_1_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_1 = {
#include "ev0017/script/ev0017_egg_carrier_1.inc"
};
static NJS_MATERIAL ev0017_egg_carrier_0_materials[15] = {
#include "ev0017/script/ev0017_egg_carrier_0_materials.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes0[303] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes0.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes1[14] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes1.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes2[7] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes2.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes3[328] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes3.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes4[142] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes4.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes5[448] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes5.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes6[62] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes6.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes7[72] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes7.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes8[96] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes8.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes9[48] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes9.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes10[22] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes10.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes11[11] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes11.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes12[63] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes12.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes13[18] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes13.inc"
};
static Sint16 ev0017_egg_carrier_0_meshes14[15] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshes14.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs0[261] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs0.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs1[11] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs1.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs2[6] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs2.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs3[286] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs3.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs4[132] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs4.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs5[397] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs5.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs6[52] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs6.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs7[54] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs7.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs8[72] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs8.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs9[44] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs9.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs10[20] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs10.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs11[10] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs11.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs12[55] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs12.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs13[16] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs13.inc"
};
static NJS_COLOR ev0017_egg_carrier_0_uvs14[14] = {
#include "ev0017/script/ev0017_egg_carrier_0_uvs14.inc"
};
static NJS_MESHSET ev0017_egg_carrier_0_meshsets[15] = {
#include "ev0017/script/ev0017_egg_carrier_0_meshsets.inc"
};
static NJS_POINT3 ev0017_egg_carrier_0_points[597] = {
#include "ev0017/script/ev0017_egg_carrier_0_points.inc"
};
static NJS_VECTOR ev0017_egg_carrier_0_normals[597] = {
#include "ev0017/script/ev0017_egg_carrier_0_normals.inc"
};
static NJS_MODEL ev0017_egg_carrier_0_model = {
#include "ev0017/script/ev0017_egg_carrier_0_model.inc"
};
static NJS_OBJECT ev0017_egg_carrier_object = {
#include "ev0017/script/ev0017_egg_carrier_object.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang3[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang3.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl3[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl3.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang4[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang4.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl4[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl4.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang5[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang5.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl5[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl5.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang6[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang6.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl6[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl6.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang7[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang7.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl7[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl7.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang8[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang8.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl8[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl8.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang9[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang9.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl9[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl9.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang10[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang10.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl10[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl10.inc"
};
static NJS_MKEY_A ev0017_egg_carrier_ang11[30] = {
#include "ev0017/script/ev0017_egg_carrier_ang11.inc"
};
static NJS_MKEY_F ev0017_egg_carrier_scl11[30] = {
#include "ev0017/script/ev0017_egg_carrier_scl11.inc"
};
static NJS_MDATA3 ev0017_egg_carrier_mdata[12] = {
#include "ev0017/script/ev0017_egg_carrier_mdata.inc"
};
static NJS_MOTION ev0017_egg_carrier_motion = {
#include "ev0017/script/ev0017_egg_carrier_motion.inc"
};
static NJS_ACTION ev0017_egg_carrier_action = {
#include "ev0017/script/ev0017_egg_carrier_action.inc"
};
static NJS_TEXNAME ev0017_tornado_textures[41] = {
    {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0},
    {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0}, {TEXT_33, 0, 0},
    {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0}, {TEXT_37, 0, 0},
    {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0}, {TEXT_41, 0, 0},
    {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0}, {TEXT_45, 0, 0},
    {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0}, {TEXT_49, 0, 0},
    {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0}, {TEXT_53, 0, 0},
    {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0}, {TEXT_57, 0, 0},
    {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0}, {TEXT_61, 0, 0},
    {TEXT_62, 0, 0}, {TEXT_63, 0, 0}, {TEXT_64, 0, 0}, {TEXT_65, 0, 0},
    {TEXT_66, 0, 0},
};
static NJS_TEXLIST ev0017_tornado_texlist = {
#include "ev0017/script/ev0017_tornado_texlist.inc"
};
static NJS_OBJECT ev0017_tornado_77 = {
#include "ev0017/script/ev0017_tornado_77.inc"
};
static NJS_OBJECT ev0017_tornado_76 = {
#include "ev0017/script/ev0017_tornado_76.inc"
};
static NJS_OBJECT ev0017_tornado_75 = {
#include "ev0017/script/ev0017_tornado_75.inc"
};
static NJS_OBJECT ev0017_tornado_74 = {
#include "ev0017/script/ev0017_tornado_74.inc"
};
static NJS_MATERIAL ev0017_tornado_73_materials[2] = {
#include "ev0017/script/ev0017_tornado_73_materials.inc"
};
static Sint16 ev0017_tornado_73_meshes0[10] = {
#include "ev0017/script/ev0017_tornado_73_meshes0.inc"
};
static Sint16 ev0017_tornado_73_meshes1[10] = {
#include "ev0017/script/ev0017_tornado_73_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_73_uvs0[8] = {
#include "ev0017/script/ev0017_tornado_73_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_73_uvs1[8] = {
#include "ev0017/script/ev0017_tornado_73_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_73_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_73_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_73_points[13] = {
#include "ev0017/script/ev0017_tornado_73_points.inc"
};
static NJS_VECTOR ev0017_tornado_73_normals[13] = {
#include "ev0017/script/ev0017_tornado_73_normals.inc"
};
static NJS_MODEL ev0017_tornado_73_model = {
#include "ev0017/script/ev0017_tornado_73_model.inc"
};
static NJS_OBJECT ev0017_tornado_73 = {
#include "ev0017/script/ev0017_tornado_73.inc"
};
static NJS_MATERIAL ev0017_tornado_72_materials[2] = {
#include "ev0017/script/ev0017_tornado_72_materials.inc"
};
static Sint16 ev0017_tornado_72_meshes0[10] = {
#include "ev0017/script/ev0017_tornado_72_meshes0.inc"
};
static Sint16 ev0017_tornado_72_meshes1[10] = {
#include "ev0017/script/ev0017_tornado_72_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_72_uvs0[8] = {
#include "ev0017/script/ev0017_tornado_72_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_72_uvs1[8] = {
#include "ev0017/script/ev0017_tornado_72_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_72_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_72_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_72_points[13] = {
#include "ev0017/script/ev0017_tornado_72_points.inc"
};
static NJS_VECTOR ev0017_tornado_72_normals[13] = {
#include "ev0017/script/ev0017_tornado_72_normals.inc"
};
static NJS_MODEL ev0017_tornado_72_model = {
#include "ev0017/script/ev0017_tornado_72_model.inc"
};
static NJS_OBJECT ev0017_tornado_72 = {
#include "ev0017/script/ev0017_tornado_72.inc"
};
static NJS_MATERIAL ev0017_tornado_71_materials[3] = {
#include "ev0017/script/ev0017_tornado_71_materials.inc"
};
static Sint16 ev0017_tornado_71_meshes0[12] = {
#include "ev0017/script/ev0017_tornado_71_meshes0.inc"
};
static Sint16 ev0017_tornado_71_meshes1[39] = {
#include "ev0017/script/ev0017_tornado_71_meshes1.inc"
};
static Sint16 ev0017_tornado_71_meshes2[39] = {
#include "ev0017/script/ev0017_tornado_71_meshes2.inc"
};
static NJS_COLOR ev0017_tornado_71_uvs0[9] = {
#include "ev0017/script/ev0017_tornado_71_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_71_uvs1[34] = {
#include "ev0017/script/ev0017_tornado_71_uvs1.inc"
};
static NJS_COLOR ev0017_tornado_71_uvs2[34] = {
#include "ev0017/script/ev0017_tornado_71_uvs2.inc"
};
static NJS_MESHSET ev0017_tornado_71_meshsets[3] = {
#include "ev0017/script/ev0017_tornado_71_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_71_points[41] = {
#include "ev0017/script/ev0017_tornado_71_points.inc"
};
static NJS_VECTOR ev0017_tornado_71_normals[41] = {
#include "ev0017/script/ev0017_tornado_71_normals.inc"
};
static NJS_MODEL ev0017_tornado_71_model = {
#include "ev0017/script/ev0017_tornado_71_model.inc"
};
static NJS_OBJECT ev0017_tornado_71 = {
#include "ev0017/script/ev0017_tornado_71.inc"
};
static NJS_MATERIAL ev0017_tornado_70_materials[2] = {
#include "ev0017/script/ev0017_tornado_70_materials.inc"
};
static Sint16 ev0017_tornado_70_meshes0[10] = {
#include "ev0017/script/ev0017_tornado_70_meshes0.inc"
};
static Sint16 ev0017_tornado_70_meshes1[42] = {
#include "ev0017/script/ev0017_tornado_70_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_70_uvs0[8] = {
#include "ev0017/script/ev0017_tornado_70_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_70_uvs1[36] = {
#include "ev0017/script/ev0017_tornado_70_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_70_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_70_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_70_points[44] = {
#include "ev0017/script/ev0017_tornado_70_points.inc"
};
static NJS_VECTOR ev0017_tornado_70_normals[44] = {
#include "ev0017/script/ev0017_tornado_70_normals.inc"
};
static NJS_MODEL ev0017_tornado_70_model = {
#include "ev0017/script/ev0017_tornado_70_model.inc"
};
static NJS_OBJECT ev0017_tornado_70 = {
#include "ev0017/script/ev0017_tornado_70.inc"
};
static NJS_MATERIAL ev0017_tornado_69_materials[2] = {
#include "ev0017/script/ev0017_tornado_69_materials.inc"
};
static Sint16 ev0017_tornado_69_meshes0[79] = {
#include "ev0017/script/ev0017_tornado_69_meshes0.inc"
};
static Sint16 ev0017_tornado_69_meshes1[17] = {
#include "ev0017/script/ev0017_tornado_69_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_69_uvs0[72] = {
#include "ev0017/script/ev0017_tornado_69_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_69_uvs1[14] = {
#include "ev0017/script/ev0017_tornado_69_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_69_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_69_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_69_points[35] = {
#include "ev0017/script/ev0017_tornado_69_points.inc"
};
static NJS_VECTOR ev0017_tornado_69_normals[35] = {
#include "ev0017/script/ev0017_tornado_69_normals.inc"
};
static NJS_MODEL ev0017_tornado_69_model = {
#include "ev0017/script/ev0017_tornado_69_model.inc"
};
static NJS_OBJECT ev0017_tornado_69 = {
#include "ev0017/script/ev0017_tornado_69.inc"
};
static NJS_MATERIAL ev0017_tornado_68_materials[1] = {
#include "ev0017/script/ev0017_tornado_68_materials.inc"
};
static Sint16 ev0017_tornado_68_meshes0[6] = {
#include "ev0017/script/ev0017_tornado_68_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_68_uvs0[5] = {
#include "ev0017/script/ev0017_tornado_68_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_68_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_68_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_68_points[4] = {
#include "ev0017/script/ev0017_tornado_68_points.inc"
};
static NJS_VECTOR ev0017_tornado_68_normals[4] = {
#include "ev0017/script/ev0017_tornado_68_normals.inc"
};
static NJS_MODEL ev0017_tornado_68_model = {
#include "ev0017/script/ev0017_tornado_68_model.inc"
};
static NJS_OBJECT ev0017_tornado_68 = {
#include "ev0017/script/ev0017_tornado_68.inc"
};
static NJS_OBJECT ev0017_tornado_67 = {
#include "ev0017/script/ev0017_tornado_67.inc"
};
static NJS_OBJECT ev0017_tornado_66 = {
#include "ev0017/script/ev0017_tornado_66.inc"
};
static NJS_OBJECT ev0017_tornado_65 = {
#include "ev0017/script/ev0017_tornado_65.inc"
};
static NJS_MATERIAL ev0017_tornado_64_materials[1] = {
#include "ev0017/script/ev0017_tornado_64_materials.inc"
};
static Sint16 ev0017_tornado_64_meshes0[21] = {
#include "ev0017/script/ev0017_tornado_64_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_64_uvs0[18] = {
#include "ev0017/script/ev0017_tornado_64_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_64_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_64_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_64_points[8] = {
#include "ev0017/script/ev0017_tornado_64_points.inc"
};
static NJS_VECTOR ev0017_tornado_64_normals[8] = {
#include "ev0017/script/ev0017_tornado_64_normals.inc"
};
static NJS_MODEL ev0017_tornado_64_model = {
#include "ev0017/script/ev0017_tornado_64_model.inc"
};
static NJS_OBJECT ev0017_tornado_64 = {
#include "ev0017/script/ev0017_tornado_64.inc"
};
static NJS_MATERIAL ev0017_tornado_63_materials[2] = {
#include "ev0017/script/ev0017_tornado_63_materials.inc"
};
static Sint16 ev0017_tornado_63_meshes0[16] = {
#include "ev0017/script/ev0017_tornado_63_meshes0.inc"
};
static Sint16 ev0017_tornado_63_meshes1[21] = {
#include "ev0017/script/ev0017_tornado_63_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_63_uvs0[14] = {
#include "ev0017/script/ev0017_tornado_63_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_63_uvs1[18] = {
#include "ev0017/script/ev0017_tornado_63_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_63_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_63_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_63_points[16] = {
#include "ev0017/script/ev0017_tornado_63_points.inc"
};
static NJS_VECTOR ev0017_tornado_63_normals[16] = {
#include "ev0017/script/ev0017_tornado_63_normals.inc"
};
static NJS_MODEL ev0017_tornado_63_model = {
#include "ev0017/script/ev0017_tornado_63_model.inc"
};
static NJS_OBJECT ev0017_tornado_63 = {
#include "ev0017/script/ev0017_tornado_63.inc"
};
static NJS_MATERIAL ev0017_tornado_62_materials[1] = {
#include "ev0017/script/ev0017_tornado_62_materials.inc"
};
static Sint16 ev0017_tornado_62_meshes0[38] = {
#include "ev0017/script/ev0017_tornado_62_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_62_uvs0[32] = {
#include "ev0017/script/ev0017_tornado_62_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_62_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_62_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_62_points[12] = {
#include "ev0017/script/ev0017_tornado_62_points.inc"
};
static NJS_VECTOR ev0017_tornado_62_normals[12] = {
#include "ev0017/script/ev0017_tornado_62_normals.inc"
};
static NJS_MODEL ev0017_tornado_62_model = {
#include "ev0017/script/ev0017_tornado_62_model.inc"
};
static NJS_OBJECT ev0017_tornado_62 = {
#include "ev0017/script/ev0017_tornado_62.inc"
};
static NJS_OBJECT ev0017_tornado_61 = {
#include "ev0017/script/ev0017_tornado_61.inc"
};
static NJS_OBJECT ev0017_tornado_60 = {
#include "ev0017/script/ev0017_tornado_60.inc"
};
static NJS_OBJECT ev0017_tornado_59 = {
#include "ev0017/script/ev0017_tornado_59.inc"
};
static NJS_OBJECT ev0017_tornado_58 = {
#include "ev0017/script/ev0017_tornado_58.inc"
};
static NJS_OBJECT ev0017_tornado_57 = {
#include "ev0017/script/ev0017_tornado_57.inc"
};
static NJS_MATERIAL ev0017_tornado_56_materials[1] = {
#include "ev0017/script/ev0017_tornado_56_materials.inc"
};
static Sint16 ev0017_tornado_56_meshes0[21] = {
#include "ev0017/script/ev0017_tornado_56_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_56_uvs0[18] = {
#include "ev0017/script/ev0017_tornado_56_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_56_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_56_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_56_points[8] = {
#include "ev0017/script/ev0017_tornado_56_points.inc"
};
static NJS_VECTOR ev0017_tornado_56_normals[8] = {
#include "ev0017/script/ev0017_tornado_56_normals.inc"
};
static NJS_MODEL ev0017_tornado_56_model = {
#include "ev0017/script/ev0017_tornado_56_model.inc"
};
static NJS_OBJECT ev0017_tornado_56 = {
#include "ev0017/script/ev0017_tornado_56.inc"
};
static NJS_MATERIAL ev0017_tornado_55_materials[2] = {
#include "ev0017/script/ev0017_tornado_55_materials.inc"
};
static Sint16 ev0017_tornado_55_meshes0[19] = {
#include "ev0017/script/ev0017_tornado_55_meshes0.inc"
};
static Sint16 ev0017_tornado_55_meshes1[21] = {
#include "ev0017/script/ev0017_tornado_55_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_55_uvs0[16] = {
#include "ev0017/script/ev0017_tornado_55_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_55_uvs1[18] = {
#include "ev0017/script/ev0017_tornado_55_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_55_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_55_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_55_points[16] = {
#include "ev0017/script/ev0017_tornado_55_points.inc"
};
static NJS_VECTOR ev0017_tornado_55_normals[16] = {
#include "ev0017/script/ev0017_tornado_55_normals.inc"
};
static NJS_MODEL ev0017_tornado_55_model = {
#include "ev0017/script/ev0017_tornado_55_model.inc"
};
static NJS_OBJECT ev0017_tornado_55 = {
#include "ev0017/script/ev0017_tornado_55.inc"
};
static NJS_MATERIAL ev0017_tornado_54_materials[1] = {
#include "ev0017/script/ev0017_tornado_54_materials.inc"
};
static Sint16 ev0017_tornado_54_meshes0[32] = {
#include "ev0017/script/ev0017_tornado_54_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_54_uvs0[28] = {
#include "ev0017/script/ev0017_tornado_54_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_54_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_54_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_54_points[12] = {
#include "ev0017/script/ev0017_tornado_54_points.inc"
};
static NJS_VECTOR ev0017_tornado_54_normals[12] = {
#include "ev0017/script/ev0017_tornado_54_normals.inc"
};
static NJS_MODEL ev0017_tornado_54_model = {
#include "ev0017/script/ev0017_tornado_54_model.inc"
};
static NJS_OBJECT ev0017_tornado_54 = {
#include "ev0017/script/ev0017_tornado_54.inc"
};
static NJS_OBJECT ev0017_tornado_53 = {
#include "ev0017/script/ev0017_tornado_53.inc"
};
static NJS_OBJECT ev0017_tornado_52 = {
#include "ev0017/script/ev0017_tornado_52.inc"
};
static NJS_OBJECT ev0017_tornado_51 = {
#include "ev0017/script/ev0017_tornado_51.inc"
};
static NJS_OBJECT ev0017_tornado_50 = {
#include "ev0017/script/ev0017_tornado_50.inc"
};
static NJS_OBJECT ev0017_tornado_49 = {
#include "ev0017/script/ev0017_tornado_49.inc"
};
static NJS_MATERIAL ev0017_tornado_48_materials[1] = {
#include "ev0017/script/ev0017_tornado_48_materials.inc"
};
static Sint16 ev0017_tornado_48_meshes0[16] = {
#include "ev0017/script/ev0017_tornado_48_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_48_uvs0[14] = {
#include "ev0017/script/ev0017_tornado_48_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_48_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_48_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_48_points[8] = {
#include "ev0017/script/ev0017_tornado_48_points.inc"
};
static NJS_VECTOR ev0017_tornado_48_normals[8] = {
#include "ev0017/script/ev0017_tornado_48_normals.inc"
};
static NJS_MODEL ev0017_tornado_48_model = {
#include "ev0017/script/ev0017_tornado_48_model.inc"
};
static NJS_OBJECT ev0017_tornado_48 = {
#include "ev0017/script/ev0017_tornado_48.inc"
};
static NJS_MATERIAL ev0017_tornado_47_materials[2] = {
#include "ev0017/script/ev0017_tornado_47_materials.inc"
};
static Sint16 ev0017_tornado_47_meshes0[21] = {
#include "ev0017/script/ev0017_tornado_47_meshes0.inc"
};
static Sint16 ev0017_tornado_47_meshes1[16] = {
#include "ev0017/script/ev0017_tornado_47_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_47_uvs0[18] = {
#include "ev0017/script/ev0017_tornado_47_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_47_uvs1[14] = {
#include "ev0017/script/ev0017_tornado_47_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_47_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_47_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_47_points[16] = {
#include "ev0017/script/ev0017_tornado_47_points.inc"
};
static NJS_VECTOR ev0017_tornado_47_normals[16] = {
#include "ev0017/script/ev0017_tornado_47_normals.inc"
};
static NJS_MODEL ev0017_tornado_47_model = {
#include "ev0017/script/ev0017_tornado_47_model.inc"
};
static NJS_OBJECT ev0017_tornado_47 = {
#include "ev0017/script/ev0017_tornado_47.inc"
};
static NJS_MATERIAL ev0017_tornado_46_materials[5] = {
#include "ev0017/script/ev0017_tornado_46_materials.inc"
};
static Sint16 ev0017_tornado_46_meshes0[25] = {
#include "ev0017/script/ev0017_tornado_46_meshes0.inc"
};
static Sint16 ev0017_tornado_46_meshes1[44] = {
#include "ev0017/script/ev0017_tornado_46_meshes1.inc"
};
static Sint16 ev0017_tornado_46_meshes2[8] = {
#include "ev0017/script/ev0017_tornado_46_meshes2.inc"
};
static Sint16 ev0017_tornado_46_meshes3[5] = {
#include "ev0017/script/ev0017_tornado_46_meshes3.inc"
};
static Sint16 ev0017_tornado_46_meshes4[10] = {
#include "ev0017/script/ev0017_tornado_46_meshes4.inc"
};
static NJS_COLOR ev0017_tornado_46_uvs0[21] = {
#include "ev0017/script/ev0017_tornado_46_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_46_uvs1[38] = {
#include "ev0017/script/ev0017_tornado_46_uvs1.inc"
};
static NJS_COLOR ev0017_tornado_46_uvs2[7] = {
#include "ev0017/script/ev0017_tornado_46_uvs2.inc"
};
static NJS_COLOR ev0017_tornado_46_uvs3[4] = {
#include "ev0017/script/ev0017_tornado_46_uvs3.inc"
};
static NJS_COLOR ev0017_tornado_46_uvs4[8] = {
#include "ev0017/script/ev0017_tornado_46_uvs4.inc"
};
static NJS_MESHSET ev0017_tornado_46_meshsets[5] = {
#include "ev0017/script/ev0017_tornado_46_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_46_points[36] = {
#include "ev0017/script/ev0017_tornado_46_points.inc"
};
static NJS_VECTOR ev0017_tornado_46_normals[36] = {
#include "ev0017/script/ev0017_tornado_46_normals.inc"
};
static NJS_MODEL ev0017_tornado_46_model = {
#include "ev0017/script/ev0017_tornado_46_model.inc"
};
static NJS_OBJECT ev0017_tornado_46 = {
#include "ev0017/script/ev0017_tornado_46.inc"
};
static NJS_OBJECT ev0017_tornado_45 = {
#include "ev0017/script/ev0017_tornado_45.inc"
};
static NJS_OBJECT ev0017_tornado_44 = {
#include "ev0017/script/ev0017_tornado_44.inc"
};
static NJS_OBJECT ev0017_tornado_43 = {
#include "ev0017/script/ev0017_tornado_43.inc"
};
static NJS_OBJECT ev0017_tornado_42 = {
#include "ev0017/script/ev0017_tornado_42.inc"
};
static NJS_OBJECT ev0017_tornado_41 = {
#include "ev0017/script/ev0017_tornado_41.inc"
};
static NJS_MATERIAL ev0017_tornado_40_materials[1] = {
#include "ev0017/script/ev0017_tornado_40_materials.inc"
};
static Sint16 ev0017_tornado_40_meshes0[19] = {
#include "ev0017/script/ev0017_tornado_40_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_40_uvs0[16] = {
#include "ev0017/script/ev0017_tornado_40_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_40_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_40_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_40_points[8] = {
#include "ev0017/script/ev0017_tornado_40_points.inc"
};
static NJS_VECTOR ev0017_tornado_40_normals[8] = {
#include "ev0017/script/ev0017_tornado_40_normals.inc"
};
static NJS_MODEL ev0017_tornado_40_model = {
#include "ev0017/script/ev0017_tornado_40_model.inc"
};
static NJS_OBJECT ev0017_tornado_40 = {
#include "ev0017/script/ev0017_tornado_40.inc"
};
static NJS_MATERIAL ev0017_tornado_39_materials[2] = {
#include "ev0017/script/ev0017_tornado_39_materials.inc"
};
static Sint16 ev0017_tornado_39_meshes0[21] = {
#include "ev0017/script/ev0017_tornado_39_meshes0.inc"
};
static Sint16 ev0017_tornado_39_meshes1[19] = {
#include "ev0017/script/ev0017_tornado_39_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_39_uvs0[18] = {
#include "ev0017/script/ev0017_tornado_39_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_39_uvs1[16] = {
#include "ev0017/script/ev0017_tornado_39_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_39_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_39_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_39_points[16] = {
#include "ev0017/script/ev0017_tornado_39_points.inc"
};
static NJS_VECTOR ev0017_tornado_39_normals[16] = {
#include "ev0017/script/ev0017_tornado_39_normals.inc"
};
static NJS_MODEL ev0017_tornado_39_model = {
#include "ev0017/script/ev0017_tornado_39_model.inc"
};
static NJS_OBJECT ev0017_tornado_39 = {
#include "ev0017/script/ev0017_tornado_39.inc"
};
static NJS_MATERIAL ev0017_tornado_38_materials[5] = {
#include "ev0017/script/ev0017_tornado_38_materials.inc"
};
static Sint16 ev0017_tornado_38_meshes0[22] = {
#include "ev0017/script/ev0017_tornado_38_meshes0.inc"
};
static Sint16 ev0017_tornado_38_meshes1[44] = {
#include "ev0017/script/ev0017_tornado_38_meshes1.inc"
};
static Sint16 ev0017_tornado_38_meshes2[8] = {
#include "ev0017/script/ev0017_tornado_38_meshes2.inc"
};
static Sint16 ev0017_tornado_38_meshes3[5] = {
#include "ev0017/script/ev0017_tornado_38_meshes3.inc"
};
static Sint16 ev0017_tornado_38_meshes4[10] = {
#include "ev0017/script/ev0017_tornado_38_meshes4.inc"
};
static NJS_COLOR ev0017_tornado_38_uvs0[19] = {
#include "ev0017/script/ev0017_tornado_38_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_38_uvs1[38] = {
#include "ev0017/script/ev0017_tornado_38_uvs1.inc"
};
static NJS_COLOR ev0017_tornado_38_uvs2[7] = {
#include "ev0017/script/ev0017_tornado_38_uvs2.inc"
};
static NJS_COLOR ev0017_tornado_38_uvs3[4] = {
#include "ev0017/script/ev0017_tornado_38_uvs3.inc"
};
static NJS_COLOR ev0017_tornado_38_uvs4[8] = {
#include "ev0017/script/ev0017_tornado_38_uvs4.inc"
};
static NJS_MESHSET ev0017_tornado_38_meshsets[5] = {
#include "ev0017/script/ev0017_tornado_38_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_38_points[36] = {
#include "ev0017/script/ev0017_tornado_38_points.inc"
};
static NJS_VECTOR ev0017_tornado_38_normals[36] = {
#include "ev0017/script/ev0017_tornado_38_normals.inc"
};
static NJS_MODEL ev0017_tornado_38_model = {
#include "ev0017/script/ev0017_tornado_38_model.inc"
};
static NJS_OBJECT ev0017_tornado_38 = {
#include "ev0017/script/ev0017_tornado_38.inc"
};
static NJS_OBJECT ev0017_tornado_37 = {
#include "ev0017/script/ev0017_tornado_37.inc"
};
static NJS_OBJECT ev0017_tornado_36 = {
#include "ev0017/script/ev0017_tornado_36.inc"
};
static NJS_OBJECT ev0017_tornado_35 = {
#include "ev0017/script/ev0017_tornado_35.inc"
};
static NJS_OBJECT ev0017_tornado_34 = {
#include "ev0017/script/ev0017_tornado_34.inc"
};
static NJS_OBJECT ev0017_tornado_33 = {
#include "ev0017/script/ev0017_tornado_33.inc"
};
static NJS_MATERIAL ev0017_tornado_32_materials[5] = {
#include "ev0017/script/ev0017_tornado_32_materials.inc"
};
static Sint16 ev0017_tornado_32_meshes0[433] = {
#include "ev0017/script/ev0017_tornado_32_meshes0.inc"
};
static Sint16 ev0017_tornado_32_meshes1[35] = {
#include "ev0017/script/ev0017_tornado_32_meshes1.inc"
};
static Sint16 ev0017_tornado_32_meshes2[59] = {
#include "ev0017/script/ev0017_tornado_32_meshes2.inc"
};
static Sint16 ev0017_tornado_32_meshes3[21] = {
#include "ev0017/script/ev0017_tornado_32_meshes3.inc"
};
static Sint16 ev0017_tornado_32_meshes4[120] = {
#include "ev0017/script/ev0017_tornado_32_meshes4.inc"
};
static NJS_COLOR ev0017_tornado_32_uvs0[381] = {
#include "ev0017/script/ev0017_tornado_32_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_32_uvs1[31] = {
#include "ev0017/script/ev0017_tornado_32_uvs1.inc"
};
static NJS_COLOR ev0017_tornado_32_uvs2[52] = {
#include "ev0017/script/ev0017_tornado_32_uvs2.inc"
};
static NJS_COLOR ev0017_tornado_32_uvs3[18] = {
#include "ev0017/script/ev0017_tornado_32_uvs3.inc"
};
static NJS_COLOR ev0017_tornado_32_uvs4[104] = {
#include "ev0017/script/ev0017_tornado_32_uvs4.inc"
};
static NJS_MESHSET ev0017_tornado_32_meshsets[5] = {
#include "ev0017/script/ev0017_tornado_32_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_32_points[247] = {
#include "ev0017/script/ev0017_tornado_32_points.inc"
};
static NJS_VECTOR ev0017_tornado_32_normals[247] = {
#include "ev0017/script/ev0017_tornado_32_normals.inc"
};
static NJS_MODEL ev0017_tornado_32_model = {
#include "ev0017/script/ev0017_tornado_32_model.inc"
};
static NJS_OBJECT ev0017_tornado_32 = {
#include "ev0017/script/ev0017_tornado_32.inc"
};
static NJS_OBJECT ev0017_tornado_31 = {
#include "ev0017/script/ev0017_tornado_31.inc"
};
static NJS_OBJECT ev0017_tornado_30 = {
#include "ev0017/script/ev0017_tornado_30.inc"
};
static NJS_OBJECT ev0017_tornado_29 = {
#include "ev0017/script/ev0017_tornado_29.inc"
};
static NJS_OBJECT ev0017_tornado_28 = {
#include "ev0017/script/ev0017_tornado_28.inc"
};
static NJS_OBJECT ev0017_tornado_27 = {
#include "ev0017/script/ev0017_tornado_27.inc"
};
static NJS_OBJECT ev0017_tornado_26 = {
#include "ev0017/script/ev0017_tornado_26.inc"
};
static NJS_MATERIAL ev0017_tornado_25_materials[2] = {
#include "ev0017/script/ev0017_tornado_25_materials.inc"
};
static Sint16 ev0017_tornado_25_meshes0[44] = {
#include "ev0017/script/ev0017_tornado_25_meshes0.inc"
};
static Sint16 ev0017_tornado_25_meshes1[11] = {
#include "ev0017/script/ev0017_tornado_25_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_25_uvs0[40] = {
#include "ev0017/script/ev0017_tornado_25_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_25_uvs1[10] = {
#include "ev0017/script/ev0017_tornado_25_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_25_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_25_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_25_points[22] = {
#include "ev0017/script/ev0017_tornado_25_points.inc"
};
static NJS_VECTOR ev0017_tornado_25_normals[22] = {
#include "ev0017/script/ev0017_tornado_25_normals.inc"
};
static NJS_MODEL ev0017_tornado_25_model = {
#include "ev0017/script/ev0017_tornado_25_model.inc"
};
static NJS_OBJECT ev0017_tornado_25 = {
#include "ev0017/script/ev0017_tornado_25.inc"
};
static NJS_MATERIAL ev0017_tornado_24_materials[1] = {
#include "ev0017/script/ev0017_tornado_24_materials.inc"
};
static Sint16 ev0017_tornado_24_meshes0[21] = {
#include "ev0017/script/ev0017_tornado_24_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_24_uvs0[18] = {
#include "ev0017/script/ev0017_tornado_24_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_24_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_24_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_24_points[8] = {
#include "ev0017/script/ev0017_tornado_24_points.inc"
};
static NJS_VECTOR ev0017_tornado_24_normals[8] = {
#include "ev0017/script/ev0017_tornado_24_normals.inc"
};
static NJS_MODEL ev0017_tornado_24_model = {
#include "ev0017/script/ev0017_tornado_24_model.inc"
};
static NJS_OBJECT ev0017_tornado_24 = {
#include "ev0017/script/ev0017_tornado_24.inc"
};
static NJS_MATERIAL ev0017_tornado_23_materials[2] = {
#include "ev0017/script/ev0017_tornado_23_materials.inc"
};
static Sint16 ev0017_tornado_23_meshes0[16] = {
#include "ev0017/script/ev0017_tornado_23_meshes0.inc"
};
static Sint16 ev0017_tornado_23_meshes1[21] = {
#include "ev0017/script/ev0017_tornado_23_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_23_uvs0[14] = {
#include "ev0017/script/ev0017_tornado_23_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_23_uvs1[18] = {
#include "ev0017/script/ev0017_tornado_23_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_23_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_23_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_23_points[16] = {
#include "ev0017/script/ev0017_tornado_23_points.inc"
};
static NJS_VECTOR ev0017_tornado_23_normals[16] = {
#include "ev0017/script/ev0017_tornado_23_normals.inc"
};
static NJS_MODEL ev0017_tornado_23_model = {
#include "ev0017/script/ev0017_tornado_23_model.inc"
};
static NJS_OBJECT ev0017_tornado_23 = {
#include "ev0017/script/ev0017_tornado_23.inc"
};
static NJS_MATERIAL ev0017_tornado_22_materials[1] = {
#include "ev0017/script/ev0017_tornado_22_materials.inc"
};
static Sint16 ev0017_tornado_22_meshes0[32] = {
#include "ev0017/script/ev0017_tornado_22_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_22_uvs0[28] = {
#include "ev0017/script/ev0017_tornado_22_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_22_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_22_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_22_points[12] = {
#include "ev0017/script/ev0017_tornado_22_points.inc"
};
static NJS_VECTOR ev0017_tornado_22_normals[12] = {
#include "ev0017/script/ev0017_tornado_22_normals.inc"
};
static NJS_MODEL ev0017_tornado_22_model = {
#include "ev0017/script/ev0017_tornado_22_model.inc"
};
static NJS_OBJECT ev0017_tornado_22 = {
#include "ev0017/script/ev0017_tornado_22.inc"
};
static NJS_OBJECT ev0017_tornado_21 = {
#include "ev0017/script/ev0017_tornado_21.inc"
};
static NJS_OBJECT ev0017_tornado_20 = {
#include "ev0017/script/ev0017_tornado_20.inc"
};
static NJS_OBJECT ev0017_tornado_19 = {
#include "ev0017/script/ev0017_tornado_19.inc"
};
static NJS_OBJECT ev0017_tornado_18 = {
#include "ev0017/script/ev0017_tornado_18.inc"
};
static NJS_OBJECT ev0017_tornado_17 = {
#include "ev0017/script/ev0017_tornado_17.inc"
};
static NJS_MATERIAL ev0017_tornado_16_materials[1] = {
#include "ev0017/script/ev0017_tornado_16_materials.inc"
};
static Sint16 ev0017_tornado_16_meshes0[21] = {
#include "ev0017/script/ev0017_tornado_16_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_16_uvs0[18] = {
#include "ev0017/script/ev0017_tornado_16_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_16_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_16_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_16_points[8] = {
#include "ev0017/script/ev0017_tornado_16_points.inc"
};
static NJS_VECTOR ev0017_tornado_16_normals[8] = {
#include "ev0017/script/ev0017_tornado_16_normals.inc"
};
static NJS_MODEL ev0017_tornado_16_model = {
#include "ev0017/script/ev0017_tornado_16_model.inc"
};
static NJS_OBJECT ev0017_tornado_16 = {
#include "ev0017/script/ev0017_tornado_16.inc"
};
static NJS_MATERIAL ev0017_tornado_15_materials[2] = {
#include "ev0017/script/ev0017_tornado_15_materials.inc"
};
static Sint16 ev0017_tornado_15_meshes0[16] = {
#include "ev0017/script/ev0017_tornado_15_meshes0.inc"
};
static Sint16 ev0017_tornado_15_meshes1[21] = {
#include "ev0017/script/ev0017_tornado_15_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_15_uvs0[14] = {
#include "ev0017/script/ev0017_tornado_15_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_15_uvs1[18] = {
#include "ev0017/script/ev0017_tornado_15_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_15_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_15_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_15_points[16] = {
#include "ev0017/script/ev0017_tornado_15_points.inc"
};
static NJS_VECTOR ev0017_tornado_15_normals[16] = {
#include "ev0017/script/ev0017_tornado_15_normals.inc"
};
static NJS_MODEL ev0017_tornado_15_model = {
#include "ev0017/script/ev0017_tornado_15_model.inc"
};
static NJS_OBJECT ev0017_tornado_15 = {
#include "ev0017/script/ev0017_tornado_15.inc"
};
static NJS_MATERIAL ev0017_tornado_14_materials[1] = {
#include "ev0017/script/ev0017_tornado_14_materials.inc"
};
static Sint16 ev0017_tornado_14_meshes0[32] = {
#include "ev0017/script/ev0017_tornado_14_meshes0.inc"
};
static NJS_COLOR ev0017_tornado_14_uvs0[28] = {
#include "ev0017/script/ev0017_tornado_14_uvs0.inc"
};
static NJS_MESHSET ev0017_tornado_14_meshsets[1] = {
#include "ev0017/script/ev0017_tornado_14_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_14_points[12] = {
#include "ev0017/script/ev0017_tornado_14_points.inc"
};
static NJS_VECTOR ev0017_tornado_14_normals[12] = {
#include "ev0017/script/ev0017_tornado_14_normals.inc"
};
static NJS_MODEL ev0017_tornado_14_model = {
#include "ev0017/script/ev0017_tornado_14_model.inc"
};
static NJS_OBJECT ev0017_tornado_14 = {
#include "ev0017/script/ev0017_tornado_14.inc"
};
static NJS_OBJECT ev0017_tornado_13 = {
#include "ev0017/script/ev0017_tornado_13.inc"
};
static NJS_OBJECT ev0017_tornado_12 = {
#include "ev0017/script/ev0017_tornado_12.inc"
};
static NJS_OBJECT ev0017_tornado_11 = {
#include "ev0017/script/ev0017_tornado_11.inc"
};
static NJS_OBJECT ev0017_tornado_10 = {
#include "ev0017/script/ev0017_tornado_10.inc"
};
static NJS_OBJECT ev0017_tornado_9 = {
#include "ev0017/script/ev0017_tornado_9.inc"
};
static NJS_MATERIAL ev0017_tornado_8_materials[4] = {
#include "ev0017/script/ev0017_tornado_8_materials.inc"
};
static Sint16 ev0017_tornado_8_meshes0[18] = {
#include "ev0017/script/ev0017_tornado_8_meshes0.inc"
};
static Sint16 ev0017_tornado_8_meshes1[297] = {
#include "ev0017/script/ev0017_tornado_8_meshes1.inc"
};
static Sint16 ev0017_tornado_8_meshes2[204] = {
#include "ev0017/script/ev0017_tornado_8_meshes2.inc"
};
static Sint16 ev0017_tornado_8_meshes3[12] = {
#include "ev0017/script/ev0017_tornado_8_meshes3.inc"
};
static NJS_COLOR ev0017_tornado_8_uvs0[16] = {
#include "ev0017/script/ev0017_tornado_8_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_8_uvs1[259] = {
#include "ev0017/script/ev0017_tornado_8_uvs1.inc"
};
static NJS_COLOR ev0017_tornado_8_uvs2[172] = {
#include "ev0017/script/ev0017_tornado_8_uvs2.inc"
};
static NJS_COLOR ev0017_tornado_8_uvs3[10] = {
#include "ev0017/script/ev0017_tornado_8_uvs3.inc"
};
static NJS_MESHSET ev0017_tornado_8_meshsets[4] = {
#include "ev0017/script/ev0017_tornado_8_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_8_points[202] = {
#include "ev0017/script/ev0017_tornado_8_points.inc"
};
static NJS_VECTOR ev0017_tornado_8_normals[202] = {
#include "ev0017/script/ev0017_tornado_8_normals.inc"
};
static NJS_MODEL ev0017_tornado_8_model = {
#include "ev0017/script/ev0017_tornado_8_model.inc"
};
static NJS_OBJECT ev0017_tornado_8 = {
#include "ev0017/script/ev0017_tornado_8.inc"
};
static NJS_OBJECT ev0017_tornado_7 = {
#include "ev0017/script/ev0017_tornado_7.inc"
};
static NJS_OBJECT ev0017_tornado_6 = {
#include "ev0017/script/ev0017_tornado_6.inc"
};
static NJS_OBJECT ev0017_tornado_5 = {
#include "ev0017/script/ev0017_tornado_5.inc"
};
static NJS_OBJECT ev0017_tornado_4 = {
#include "ev0017/script/ev0017_tornado_4.inc"
};
static NJS_OBJECT ev0017_tornado_3 = {
#include "ev0017/script/ev0017_tornado_3.inc"
};
static NJS_OBJECT ev0017_tornado_2 = {
#include "ev0017/script/ev0017_tornado_2.inc"
};
static NJS_MATERIAL ev0017_tornado_1_materials[2] = {
#include "ev0017/script/ev0017_tornado_1_materials.inc"
};
static Sint16 ev0017_tornado_1_meshes0[93] = {
#include "ev0017/script/ev0017_tornado_1_meshes0.inc"
};
static Sint16 ev0017_tornado_1_meshes1[36] = {
#include "ev0017/script/ev0017_tornado_1_meshes1.inc"
};
static NJS_COLOR ev0017_tornado_1_uvs0[78] = {
#include "ev0017/script/ev0017_tornado_1_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_1_uvs1[32] = {
#include "ev0017/script/ev0017_tornado_1_uvs1.inc"
};
static NJS_MESHSET ev0017_tornado_1_meshsets[2] = {
#include "ev0017/script/ev0017_tornado_1_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_1_points[44] = {
#include "ev0017/script/ev0017_tornado_1_points.inc"
};
static NJS_VECTOR ev0017_tornado_1_normals[44] = {
#include "ev0017/script/ev0017_tornado_1_normals.inc"
};
static NJS_MODEL ev0017_tornado_1_model = {
#include "ev0017/script/ev0017_tornado_1_model.inc"
};
static NJS_OBJECT ev0017_tornado_1 = {
#include "ev0017/script/ev0017_tornado_1.inc"
};
static NJS_MATERIAL ev0017_tornado_0_materials[19] = {
#include "ev0017/script/ev0017_tornado_0_materials.inc"
};
static Sint16 ev0017_tornado_0_meshes0[204] = {
#include "ev0017/script/ev0017_tornado_0_meshes0.inc"
};
static Sint16 ev0017_tornado_0_meshes1[11] = {
#include "ev0017/script/ev0017_tornado_0_meshes1.inc"
};
static Sint16 ev0017_tornado_0_meshes2[15] = {
#include "ev0017/script/ev0017_tornado_0_meshes2.inc"
};
static Sint16 ev0017_tornado_0_meshes3[5] = {
#include "ev0017/script/ev0017_tornado_0_meshes3.inc"
};
static Sint16 ev0017_tornado_0_meshes4[32] = {
#include "ev0017/script/ev0017_tornado_0_meshes4.inc"
};
static Sint16 ev0017_tornado_0_meshes5[28] = {
#include "ev0017/script/ev0017_tornado_0_meshes5.inc"
};
static Sint16 ev0017_tornado_0_meshes6[28] = {
#include "ev0017/script/ev0017_tornado_0_meshes6.inc"
};
static Sint16 ev0017_tornado_0_meshes7[30] = {
#include "ev0017/script/ev0017_tornado_0_meshes7.inc"
};
static Sint16 ev0017_tornado_0_meshes8[83] = {
#include "ev0017/script/ev0017_tornado_0_meshes8.inc"
};
static Sint16 ev0017_tornado_0_meshes9[10] = {
#include "ev0017/script/ev0017_tornado_0_meshes9.inc"
};
static Sint16 ev0017_tornado_0_meshes10[10] = {
#include "ev0017/script/ev0017_tornado_0_meshes10.inc"
};
static Sint16 ev0017_tornado_0_meshes11[12] = {
#include "ev0017/script/ev0017_tornado_0_meshes11.inc"
};
static Sint16 ev0017_tornado_0_meshes12[98] = {
#include "ev0017/script/ev0017_tornado_0_meshes12.inc"
};
static Sint16 ev0017_tornado_0_meshes13[48] = {
#include "ev0017/script/ev0017_tornado_0_meshes13.inc"
};
static Sint16 ev0017_tornado_0_meshes14[48] = {
#include "ev0017/script/ev0017_tornado_0_meshes14.inc"
};
static Sint16 ev0017_tornado_0_meshes15[15] = {
#include "ev0017/script/ev0017_tornado_0_meshes15.inc"
};
static Sint16 ev0017_tornado_0_meshes16[9] = {
#include "ev0017/script/ev0017_tornado_0_meshes16.inc"
};
static Sint16 ev0017_tornado_0_meshes17[16] = {
#include "ev0017/script/ev0017_tornado_0_meshes17.inc"
};
static Sint16 ev0017_tornado_0_meshes18[6] = {
#include "ev0017/script/ev0017_tornado_0_meshes18.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs0[180] = {
#include "ev0017/script/ev0017_tornado_0_uvs0.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs1[10] = {
#include "ev0017/script/ev0017_tornado_0_uvs1.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs2[12] = {
#include "ev0017/script/ev0017_tornado_0_uvs2.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs3[4] = {
#include "ev0017/script/ev0017_tornado_0_uvs3.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs4[25] = {
#include "ev0017/script/ev0017_tornado_0_uvs4.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs5[22] = {
#include "ev0017/script/ev0017_tornado_0_uvs5.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs6[22] = {
#include "ev0017/script/ev0017_tornado_0_uvs6.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs7[25] = {
#include "ev0017/script/ev0017_tornado_0_uvs7.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs8[68] = {
#include "ev0017/script/ev0017_tornado_0_uvs8.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs9[8] = {
#include "ev0017/script/ev0017_tornado_0_uvs9.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs10[8] = {
#include "ev0017/script/ev0017_tornado_0_uvs10.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs11[10] = {
#include "ev0017/script/ev0017_tornado_0_uvs11.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs12[84] = {
#include "ev0017/script/ev0017_tornado_0_uvs12.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs13[40] = {
#include "ev0017/script/ev0017_tornado_0_uvs13.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs14[40] = {
#include "ev0017/script/ev0017_tornado_0_uvs14.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs15[13] = {
#include "ev0017/script/ev0017_tornado_0_uvs15.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs16[8] = {
#include "ev0017/script/ev0017_tornado_0_uvs16.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs17[14] = {
#include "ev0017/script/ev0017_tornado_0_uvs17.inc"
};
static NJS_COLOR ev0017_tornado_0_uvs18[5] = {
#include "ev0017/script/ev0017_tornado_0_uvs18.inc"
};
static NJS_MESHSET ev0017_tornado_0_meshsets[19] = {
#include "ev0017/script/ev0017_tornado_0_meshsets.inc"
};
static NJS_POINT3 ev0017_tornado_0_points[211] = {
#include "ev0017/script/ev0017_tornado_0_points.inc"
};
static NJS_VECTOR ev0017_tornado_0_normals[211] = {
#include "ev0017/script/ev0017_tornado_0_normals.inc"
};
static NJS_MODEL ev0017_tornado_0_model = {
#include "ev0017/script/ev0017_tornado_0_model.inc"
};
static NJS_OBJECT ev0017_tornado_object = {
#include "ev0017/script/ev0017_tornado_object.inc"
};
static NJS_MKEY_F ev0017_tornado_pos0[45] = {
#include "ev0017/script/ev0017_tornado_pos0.inc"
};
static NJS_MKEY_A ev0017_tornado_ang3[40] = {
#include "ev0017/script/ev0017_tornado_ang3.inc"
};
static NJS_MKEY_A ev0017_tornado_ang6[44] = {
#include "ev0017/script/ev0017_tornado_ang6.inc"
};
static NJS_MKEY_A ev0017_tornado_ang10[43] = {
#include "ev0017/script/ev0017_tornado_ang10.inc"
};
static NJS_MKEY_A ev0017_tornado_ang11[45] = {
#include "ev0017/script/ev0017_tornado_ang11.inc"
};
static NJS_MKEY_A ev0017_tornado_ang18[2] = {
#include "ev0017/script/ev0017_tornado_ang18.inc"
};
static NJS_MKEY_A ev0017_tornado_ang19[44] = {
#include "ev0017/script/ev0017_tornado_ang19.inc"
};
static NJS_MKEY_F ev0017_tornado_pos26[45] = {
#include "ev0017/script/ev0017_tornado_pos26.inc"
};
static NJS_MKEY_A ev0017_tornado_ang30[45] = {
#include "ev0017/script/ev0017_tornado_ang30.inc"
};
static NJS_MKEY_A ev0017_tornado_ang34[45] = {
#include "ev0017/script/ev0017_tornado_ang34.inc"
};
static NJS_MKEY_A ev0017_tornado_ang35[45] = {
#include "ev0017/script/ev0017_tornado_ang35.inc"
};
static NJS_MKEY_A ev0017_tornado_ang36[44] = {
#include "ev0017/script/ev0017_tornado_ang36.inc"
};
static NJS_MKEY_A ev0017_tornado_ang42[45] = {
#include "ev0017/script/ev0017_tornado_ang42.inc"
};
static NJS_MKEY_A ev0017_tornado_ang43[42] = {
#include "ev0017/script/ev0017_tornado_ang43.inc"
};
static NJS_MKEY_A ev0017_tornado_ang44[45] = {
#include "ev0017/script/ev0017_tornado_ang44.inc"
};
static NJS_MKEY_A ev0017_tornado_ang50[45] = {
#include "ev0017/script/ev0017_tornado_ang50.inc"
};
static NJS_MKEY_A ev0017_tornado_ang51[44] = {
#include "ev0017/script/ev0017_tornado_ang51.inc"
};
static NJS_MKEY_A ev0017_tornado_ang52[41] = {
#include "ev0017/script/ev0017_tornado_ang52.inc"
};
static NJS_MKEY_A ev0017_tornado_ang58[45] = {
#include "ev0017/script/ev0017_tornado_ang58.inc"
};
static NJS_MKEY_A ev0017_tornado_ang59[39] = {
#include "ev0017/script/ev0017_tornado_ang59.inc"
};
static NJS_MKEY_A ev0017_tornado_ang60[38] = {
#include "ev0017/script/ev0017_tornado_ang60.inc"
};
static NJS_MKEY_A ev0017_tornado_ang71[45] = {
#include "ev0017/script/ev0017_tornado_ang71.inc"
};
static NJS_MKEY_A ev0017_tornado_ang72[45] = {
#include "ev0017/script/ev0017_tornado_ang72.inc"
};
static NJS_MKEY_F ev0017_tornado_scl72[45] = {
#include "ev0017/script/ev0017_tornado_scl72.inc"
};
static NJS_MKEY_A ev0017_tornado_ang73[45] = {
#include "ev0017/script/ev0017_tornado_ang73.inc"
};
static NJS_MKEY_F ev0017_tornado_scl73[45] = {
#include "ev0017/script/ev0017_tornado_scl73.inc"
};
static NJS_MDATA3 ev0017_tornado_mdata[78] = {
#include "ev0017/script/ev0017_tornado_mdata.inc"
};
static NJS_MOTION ev0017_tornado_motion = {
#include "ev0017/script/ev0017_tornado_motion.inc"
};
static NJS_ACTION ev0017_tornado_action = {
#include "ev0017/script/ev0017_tornado_action.inc"
};

static struct task *ev0017_tornado = NULL;
static struct task *ev0017_fade = NULL;
/* Sonic's node while ev0017_tornado_sonic_stub stands in for it, and NULL
 * while the node is in the tree */
static NJS_OBJECT *ev0017_tornado_sonic = NULL;

/* the node that stands in for Sonic's. ev0017_hide_sonic gives it Sonic's
 * child and sibling. every other field is 0, so its scale of 0 hides the
 * nodes below it. it keeps the tree's nodes in the order
 * ev0017_tornado_mdata animates them */
static NJS_OBJECT ev0017_tornado_sonic_stub;

/* hide the Sonic on the tornado, so that only player 0's Sonic shows until
 * he jumps aboard. ev0017_tornado_sonic_stub takes the place of his node,
 * which is ev0017_tornado_object's third child */
static void ev0017_hide_sonic(void)
{
    ev0017_tornado_sonic_stub.model = NULL;
    ev0017_tornado_sonic_stub.child =
        ev0017_tornado_object.child->sibling->sibling->child;
    ev0017_tornado_sonic_stub.sibling =
        ev0017_tornado_object.child->sibling->sibling->sibling;
    ev0017_tornado_sonic = ev0017_tornado_object.child->sibling->sibling;
    ev0017_tornado_object.child->sibling->sibling = &ev0017_tornado_sonic_stub;
}

/* undo ev0017_hide_sonic, if Sonic is hidden */
static void ev0017_show_sonic(void)
{
    if (ev0017_tornado_sonic != NULL) {
        /* @hack spelling: child as (&ev0017_tornado_object)->child. SHC then
         * loads the object's address from the pool word at 0x0cb80094,
         * which ev0017_hide_sonic loads it from too. it reads child 44 bytes
         * past that address, as MOV.L @(44,R2),R3 at 0x0cb80042 does. as
         * ev0017_tornado_object.child, SHC adds a pool word that holds the
         * child field's address, and 5 of 12 words differ */
        (&ev0017_tornado_object)->child->sibling->sibling =
            ev0017_tornado_sonic;
        ev0017_tornado_sonic = NULL;
    }
}

void ev0017_script(int phase)
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
        ev0017_fade =
            event_fade_create(EVENT_FADE_STILL, 0.0F, 0.0F, 0.0F, 0.0F);
        event_camera_look_clear();
        event_set_pos(event_get_player(0), -404.6F, 1039.6F, 3138.4F);
        event_set_ang(event_get_player(0), 0, 0x3000, 0);
        event_prop_create(-85.7F, 1290.6F, 3437.4F, 0, 0xad00, 0, 0.03F, 0.03F,
                          0.03F, &ev0017_egg_carrier_object,
                          &ev0017_egg_carrier_texlist, 0);
        event_prop_set_action(0, &ev0017_egg_carrier_action, 0);
        event_prop_play(0, 0, 1.0F, EVENT_PROP_FOREVER);
        event_place_actor(&ev0017_tornado, -738.9F, 1120.0F, 3336.6F, 0, 0xf000,
                          0);
        event_wait(1);
        event_set_flags(ev0017_tornado, EVENT_IGNORE_GROUND);
        event_set_pos(ev0017_tornado, -738.9F, 1120.0F, 3336.6F);
        event_set_ang(ev0017_tornado, 0xe000, 0xff00, 0);
        event_play_action(ev0017_tornado, &ev0017_tornado_action,
                          &ev0017_tornado_texlist, 1.0F, EVENT_REPEAT, 0);
        ev0017_hide_sonic();
        event_move(event_prop_get(0), -85.7F, 1290.6F, 3437.4F, 195.5F, 1255.5F,
                   3580.68F, 1000);
        event_sound_start(1, 1234, 1800);
        event_sound_fade_volume(1, 106, 1);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -197.7F, 1168.6F, 3510.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c15, 0xbf96, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 190, -326.7F, 1210.6F, 3367.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 190, 0x0815, 0xb596, 0);
        event_wait(40);
        event_sound_fade_volume(1, 20, 120);
        event_wait(140);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -420.6F, 1040.6F, 3130.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0820, 0x9a93, 0x0800);
        event_play_action(event_get_player(0), &ev0017_sonic_action_0,
                          &sonic_texlist, 1.0F, 0, 20);
        event_play_action(event_get_player(0), &ev0017_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 130, -412.8F, 1040.6F,
                             3124.85F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 130, 0x0c20, 0x9b93, 0x0800);
        event_wait(140);
        event_look_at(event_get_player(0), -378.8F, 1071.7F, 3150.5F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -391.0F, 1058.8F, 3144.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe040, 0x2ddd, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -393.05F, 1056.66F,
                             3144.0F);
        event_wait(100);
        event_look_clear(event_get_player(0));
        event_sound_start(2, 1233, 1800);
        event_sound_fade_volume(2, 80, 120);
        event_look_at_task(event_get_player(0), ev0017_tornado, 0.0F, 1.0F,
                           0.0F);
        event_move(ev0017_tornado, -738.9F, 1120.0F, 3336.6F, -379.8F, 1085.2F,
                   3363.8F, 120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 85, -395.96F, 1048.3F, 3135.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 85, 0xfc20, 0x6593, 0x0a00);
        event_wait(140);
        event_set_pos(ev0017_tornado, -820.7F, 940.6F, 3392.7F);
        event_set_ang(ev0017_tornado, -0x2500, 0x0c00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -795.6F, 939.0F, 3368.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0020, 0x6893, 0xf700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -822.7F, 947.6F, 3364.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xfa20, 0x7893, 0xf100);
        event_sound_fade_volume(2, 110, 5);
        event_voice_play(1067);
        event_message_show(TEXT_1);
        event_wait(40);
        event_move(ev0017_tornado, -820.7F, 940.6F, 3392.7F, -772.9F, 934.6F,
                   3380.8F, 60);
        stream_request_music(MUSIC_THEMILES);
        event_wait(50);
        event_set_ang(event_get_player(0), 0, 0x1f00, 0);
        event_play_action(event_get_player(0), &ev0017_sonic_action_2,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_move_stop_all();
        event_camera_look_clear();
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -415.9F, 1092.4F, 3196.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xde20, 0x0793, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -350.83F, 1099.85F,
                             3176.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xe020, 0x2093, 0);
        event_sound_start(3, 1228, 1800);
        event_sound_fade_volume(3, 100, 1);
        event_wait(17);
        event_play_path(ev0017_tornado, &ev0017_tornado_path, 0.094F, 0);
        event_voice_play(1068);
        event_message_show(TEXT_2);
        event_wait(50);
        event_look_clear(event_get_player(0));
        event_message_close();
        event_sound_stop(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -418.8F, 1043.8F, 3125.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0030, 0x6f44, 0x0acb);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 110, -409.3F, 1043.45F,
                             3129.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x04e5, 0xa5f5, 0xf600);
        event_clear_motions(event_get_player(0));
        event_sound_fade_volume(3, 110, 1);
        event_sound_fade_pitch(3, -1440, 100);
        event_wait(15);
        event_play_pad(0, &ev0017_jump);
        event_sound_fade_volume(3, 128, 100);
        event_wait(106);
        event_set_pos(event_get_player(0), -451.6F, 1043.6F, 2985.9F);
        event_camera_look_clear();
        ev0017_show_sonic();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -223.15F, 1050.7F, 3116.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xeec2, 0x2571, 0xf100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 95, -230.23F, 1057.95F,
                             3126.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xe9c1, 0x2097, 0xf020);
        event_sound_fade_volume(3, 110, 1);
        event_wait(1);
        event_sound_fade_volume(3, 105, 90);
        event_wait(95);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -196.9F, 1019.6F, 3212.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b30, 0x1f44, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -145.55F, 1057.0F,
                             3178.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x09c7, 0x1f44, 0xfd00);
        event_sound_fade_volume(3, 100, 1);
        event_wait(1);
        event_sound_fade_volume(3, 125, 39);
        event_wait_paths(ev0017_tornado);
        event_sound_fade_volume(3, 110, 1);
        event_sound_fade_pitch(3, -2440, 1);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -147.23F, 1057.71F,
                             3176.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0ac7, 0x1e44, 0xfd00);
        event_clear_paths(ev0017_tornado);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -324.7F, 1273.7F, 3166.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf4d2, 0x5489, 0);
        event_sound_fade_volume(3, 115, 1);
        event_sound_fade_pitch(3, -1440, 1);
        event_wait(1);
        event_set_pos(ev0017_tornado, -355.7F, 1257.6F, 3170.4F);
        event_set_ang(ev0017_tornado, 0, 0xed00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -330.7F, 1273.66F,
                             3153.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0xeed0, 0x5189, 0xff00);
        event_wait(30);
        event_voice_play(1069);
        event_message_show(TEXT_3);
        event_voice_wait();
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -367.4F, 1257.7F, 3152.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0030, 0x8f44, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -373.4F, 1262.4F, 3152.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0230, 0x9044, 0);
        event_voice_play(1070);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -362.3F, 1255.1F, 3162.9F);
        event_wait(5);
        event_sound_fade_volume(3, 128, 1);
        event_sound_fade_pitch(3, 2440, 1);
        event_wait(1);
        event_move(ev0017_tornado, -355.7F, 1257.6F, 3170.4F, -268.0F, 1258.17F,
                   3216.9F, 210);
        event_camera_set_pos(EVENT_EASE_LINEAR, 13, -373.7F, 1238.75F, 3140.2F);
        event_wait(13);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -420.1F, 1247.77F, 3152.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0x0530, 0xa644, 0xe800);
        event_wait(45);
        event_sound_fade_volume(3, 30, 160);
        event_sound_fade_pitch(3, -1440, 160);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -468.1F, 1220.0F, 3122.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0c30, 0xae44, 0xf400);
        event_wait(30);
        if (ev0017_fade != NULL) {
            event_fade_set_speed(ev0017_fade, 0.02F);
        }
        event_wait(90);
        event_move_stop_all();
        event_sound_stop(3);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop(1);
        event_sound_stop(2);
        event_sound_stop(3);
        event_sound_stop_all();
        event_reset_player(0);
        ev0017_show_sonic();
        event_move_stop_all();
        event_prop_kill(0);
        event_kill_task(&ev0017_tornado);
        event_set_pos(event_get_player(0), -451.6F, 1043.6F, 2985.9F);
        if (ev0017_fade != NULL) {
            task_kill(ev0017_fade);
            ev0017_fade = NULL;
        }
        break;
    case EVENT_UNLOAD:
        event_sound_stop_all();
        break;
    }
}

struct texture_pvm ev0017_textures[] = {
    {"EC_LIGHT", &ev0017_egg_carrier_texlist},
    {"EV_TR2BEFORE_WITH_SONIC", &ev0017_tornado_texlist},
    {NULL, NULL},
};
