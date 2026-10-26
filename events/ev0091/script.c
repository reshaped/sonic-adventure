/* EV0091, Knuckles' story: Knuckles lays the pieces he has gathered in the
 * altar. the Master Emerald stays incomplete. a small Egg Carrier rises before
 * the altar, and he asks if the rest is aboard it. later he sees E-102 and
 * resolves to follow him to Eggman's base */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/master_emerald.h"
#include "ev0091/knuckles_action_6.h"
#include "ev0091/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/e102.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/orbs.h"
#include "shared/prop.h"
#include "shared/rays.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 29
#define TEXT_3_LENGTH 17
#define TEXT_4_LENGTH 47
#define TEXT_5_LENGTH 20
#define TEXT_6_LENGTH 22
#define TEXT_7_LENGTH 37
#define TEXT_8_LENGTH 31
#define TEXT_9_LENGTH 53
#define TEXT_10_LENGTH 25
#define TEXT_11_LENGTH 23
#define TEXT_12_LENGTH 66
#define TEXT_13_LENGTH 15
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 10
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 11
#define TEXT_21_LENGTH 11
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 11
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 11
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 10
#define TEXT_30_LENGTH 10
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 10
#define TEXT_33_LENGTH 13
#define TEXT_34_LENGTH 13
#define TEXT_35_LENGTH 3
#define TEXT_36_LENGTH 8
#define TEXT_37_LENGTH 7
#define TEXT_38_LENGTH 13
#define TEXT_39_LENGTH 9
#define TEXT_40_LENGTH 12
#define TEXT_41_LENGTH 10
#define TEXT_42_LENGTH 10
#define TEXT_43_LENGTH 12
#define TEXT_44_LENGTH 11
#include "ev0091/script/text.inc"

/* the Egg Carrier's prop slot */
#define EGG_CARRIER 0

extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_walk_action;
extern NJS_ACTION knuckles_run_action;

extern NJS_ACTION ev0091_knuckles_action_0;
extern NJS_ACTION ev0091_knuckles_action_1;
extern NJS_ACTION ev0091_knuckles_action_2;
extern NJS_ACTION ev0091_knuckles_action_3;
extern NJS_ACTION ev0091_knuckles_action_4;
extern NJS_ACTION ev0091_knuckles_action_5;

static NJS_MATERIAL ev0091_egg_carrier_11_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_11_materials.inc"
};
static Sint16 ev0091_egg_carrier_11_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_11_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_11_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_11_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_11_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_11_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_11_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_11_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_11_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_11_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_11_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_11_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_11_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_11_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_11_model = {
#include "ev0091/script/ev0091_egg_carrier_11_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_11 = {
#include "ev0091/script/ev0091_egg_carrier_11.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_10_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_10_materials.inc"
};
static Sint16 ev0091_egg_carrier_10_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_10_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_10_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_10_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_10_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_10_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_10_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_10_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_10_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_10_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_10_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_10_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_10_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_10_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_10_model = {
#include "ev0091/script/ev0091_egg_carrier_10_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_10 = {
#include "ev0091/script/ev0091_egg_carrier_10.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_9_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_9_materials.inc"
};
static Sint16 ev0091_egg_carrier_9_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_9_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_9_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_9_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_9_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_9_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_9_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_9_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_9_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_9_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_9_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_9_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_9_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_9_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_9_model = {
#include "ev0091/script/ev0091_egg_carrier_9_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_9 = {
#include "ev0091/script/ev0091_egg_carrier_9.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_8_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_8_materials.inc"
};
static Sint16 ev0091_egg_carrier_8_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_8_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_8_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_8_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_8_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_8_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_8_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_8_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_8_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_8_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_8_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_8_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_8_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_8_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_8_model = {
#include "ev0091/script/ev0091_egg_carrier_8_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_8 = {
#include "ev0091/script/ev0091_egg_carrier_8.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_7_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_7_materials.inc"
};
static Sint16 ev0091_egg_carrier_7_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_7_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_7_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_7_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_7_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_7_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_7_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_7_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_7_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_7_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_7_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_7_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_7_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_7_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_7_model = {
#include "ev0091/script/ev0091_egg_carrier_7_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_7 = {
#include "ev0091/script/ev0091_egg_carrier_7.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_6_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_6_materials.inc"
};
static Sint16 ev0091_egg_carrier_6_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_6_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_6_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_6_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_6_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_6_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_6_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_6_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_6_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_6_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_6_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_6_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_6_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_6_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_6_model = {
#include "ev0091/script/ev0091_egg_carrier_6_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_6 = {
#include "ev0091/script/ev0091_egg_carrier_6.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_5_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_5_materials.inc"
};
static Sint16 ev0091_egg_carrier_5_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_5_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_5_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_5_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_5_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_5_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_5_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_5_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_5_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_5_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_5_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_5_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_5_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_5_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_5_model = {
#include "ev0091/script/ev0091_egg_carrier_5_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_5 = {
#include "ev0091/script/ev0091_egg_carrier_5.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_4_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_4_materials.inc"
};
static Sint16 ev0091_egg_carrier_4_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_4_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_4_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_4_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_4_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_4_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_4_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_4_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_4_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_4_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_4_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_4_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_4_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_4_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_4_model = {
#include "ev0091/script/ev0091_egg_carrier_4_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_4 = {
#include "ev0091/script/ev0091_egg_carrier_4.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_3_materials[2] = {
#include "ev0091/script/ev0091_egg_carrier_3_materials.inc"
};
static Sint16 ev0091_egg_carrier_3_meshes0[10] = {
#include "ev0091/script/ev0091_egg_carrier_3_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_3_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_3_meshes1.inc"
};
static NJS_COLOR ev0091_egg_carrier_3_uvs0[8] = {
#include "ev0091/script/ev0091_egg_carrier_3_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_3_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_3_uvs1.inc"
};
static NJS_MESHSET ev0091_egg_carrier_3_meshsets[2] = {
#include "ev0091/script/ev0091_egg_carrier_3_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_3_points[13] = {
#include "ev0091/script/ev0091_egg_carrier_3_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_3_normals[13] = {
#include "ev0091/script/ev0091_egg_carrier_3_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_3_model = {
#include "ev0091/script/ev0091_egg_carrier_3_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_3 = {
#include "ev0091/script/ev0091_egg_carrier_3.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_2_materials[7] = {
#include "ev0091/script/ev0091_egg_carrier_2_materials.inc"
};
static Sint16 ev0091_egg_carrier_2_meshes0[681] = {
#include "ev0091/script/ev0091_egg_carrier_2_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_2_meshes1[10] = {
#include "ev0091/script/ev0091_egg_carrier_2_meshes1.inc"
};
static Sint16 ev0091_egg_carrier_2_meshes2[49] = {
#include "ev0091/script/ev0091_egg_carrier_2_meshes2.inc"
};
static Sint16 ev0091_egg_carrier_2_meshes3[20] = {
#include "ev0091/script/ev0091_egg_carrier_2_meshes3.inc"
};
static Sint16 ev0091_egg_carrier_2_meshes4[20] = {
#include "ev0091/script/ev0091_egg_carrier_2_meshes4.inc"
};
static Sint16 ev0091_egg_carrier_2_meshes5[196] = {
#include "ev0091/script/ev0091_egg_carrier_2_meshes5.inc"
};
static Sint16 ev0091_egg_carrier_2_meshes6[38] = {
#include "ev0091/script/ev0091_egg_carrier_2_meshes6.inc"
};
static NJS_COLOR ev0091_egg_carrier_2_uvs0[606] = {
#include "ev0091/script/ev0091_egg_carrier_2_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_2_uvs1[8] = {
#include "ev0091/script/ev0091_egg_carrier_2_uvs1.inc"
};
static NJS_COLOR ev0091_egg_carrier_2_uvs2[42] = {
#include "ev0091/script/ev0091_egg_carrier_2_uvs2.inc"
};
static NJS_COLOR ev0091_egg_carrier_2_uvs3[16] = {
#include "ev0091/script/ev0091_egg_carrier_2_uvs3.inc"
};
static NJS_COLOR ev0091_egg_carrier_2_uvs4[16] = {
#include "ev0091/script/ev0091_egg_carrier_2_uvs4.inc"
};
static NJS_COLOR ev0091_egg_carrier_2_uvs5[168] = {
#include "ev0091/script/ev0091_egg_carrier_2_uvs5.inc"
};
static NJS_COLOR ev0091_egg_carrier_2_uvs6[32] = {
#include "ev0091/script/ev0091_egg_carrier_2_uvs6.inc"
};
static NJS_MESHSET ev0091_egg_carrier_2_meshsets[7] = {
#include "ev0091/script/ev0091_egg_carrier_2_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_2_points[390] = {
#include "ev0091/script/ev0091_egg_carrier_2_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_2_normals[390] = {
#include "ev0091/script/ev0091_egg_carrier_2_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_2_model = {
#include "ev0091/script/ev0091_egg_carrier_2_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_2 = {
#include "ev0091/script/ev0091_egg_carrier_2.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_1_materials[15] = {
#include "ev0091/script/ev0091_egg_carrier_1_materials.inc"
};
static Sint16 ev0091_egg_carrier_1_meshes0[64] = {
#include "ev0091/script/ev0091_egg_carrier_1_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_1_meshes1[237] = {
#include "ev0091/script/ev0091_egg_carrier_1_meshes1.inc"
};
static Sint16 ev0091_egg_carrier_1_meshes2[808] = {
#include "ev0091/script/ev0091_egg_carrier_1_meshes2.inc"
};
static Sint16 ev0091_egg_carrier_1_meshes3[38] = {
#include "ev0091/script/ev0091_egg_carrier_1_meshes3.inc"
};
static NJS_COLOR ev0091_egg_carrier_1_uvs0[54] = {
#include "ev0091/script/ev0091_egg_carrier_1_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_1_uvs1[208] = {
#include "ev0091/script/ev0091_egg_carrier_1_uvs1.inc"
};
static NJS_COLOR ev0091_egg_carrier_1_uvs2[714] = {
#include "ev0091/script/ev0091_egg_carrier_1_uvs2.inc"
};
static NJS_COLOR ev0091_egg_carrier_1_uvs3[36] = {
#include "ev0091/script/ev0091_egg_carrier_1_uvs3.inc"
};
static NJS_MESHSET ev0091_egg_carrier_1_meshsets[4] = {
#include "ev0091/script/ev0091_egg_carrier_1_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_1_points[571] = {
#include "ev0091/script/ev0091_egg_carrier_1_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_1_normals[571] = {
#include "ev0091/script/ev0091_egg_carrier_1_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_1_model = {
#include "ev0091/script/ev0091_egg_carrier_1_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_1 = {
#include "ev0091/script/ev0091_egg_carrier_1.inc"
};
static NJS_MATERIAL ev0091_egg_carrier_0_materials[15] = {
#include "ev0091/script/ev0091_egg_carrier_0_materials.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes0[303] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes0.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes1[14] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes1.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes2[7] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes2.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes3[328] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes3.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes4[142] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes4.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes5[448] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes5.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes6[62] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes6.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes7[72] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes7.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes8[96] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes8.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes9[48] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes9.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes10[22] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes10.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes11[11] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes11.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes12[63] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes12.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes13[18] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes13.inc"
};
static Sint16 ev0091_egg_carrier_0_meshes14[15] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshes14.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs0[261] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs0.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs1[11] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs1.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs2[6] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs2.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs3[286] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs3.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs4[132] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs4.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs5[397] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs5.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs6[52] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs6.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs7[54] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs7.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs8[72] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs8.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs9[44] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs9.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs10[20] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs10.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs11[10] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs11.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs12[55] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs12.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs13[16] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs13.inc"
};
static NJS_COLOR ev0091_egg_carrier_0_uvs14[14] = {
#include "ev0091/script/ev0091_egg_carrier_0_uvs14.inc"
};
static NJS_MESHSET ev0091_egg_carrier_0_meshsets[15] = {
#include "ev0091/script/ev0091_egg_carrier_0_meshsets.inc"
};
static NJS_POINT3 ev0091_egg_carrier_0_points[597] = {
#include "ev0091/script/ev0091_egg_carrier_0_points.inc"
};
static NJS_VECTOR ev0091_egg_carrier_0_normals[597] = {
#include "ev0091/script/ev0091_egg_carrier_0_normals.inc"
};
static NJS_MODEL ev0091_egg_carrier_0_model = {
#include "ev0091/script/ev0091_egg_carrier_0_model.inc"
};
static NJS_OBJECT ev0091_egg_carrier_object = {
#include "ev0091/script/ev0091_egg_carrier_object.inc"
};
static NJS_TEXNAME ev0091_egg_carrier_textures[21] = {
    {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0},
    {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0}, {TEXT_21, 0, 0},
    {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0}, {TEXT_25, 0, 0},
    {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0},
    {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0}, {TEXT_33, 0, 0},
    {TEXT_34, 0, 0},
};
static NJS_TEXLIST ev0091_egg_carrier_texlist = {
#include "ev0091/script/ev0091_egg_carrier_texlist.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang3[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang3.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl3[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl3.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang4[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang4.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl4[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl4.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang5[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang5.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl5[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl5.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang6[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang6.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl6[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl6.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang7[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang7.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl7[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl7.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang8[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang8.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl8[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl8.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang9[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang9.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl9[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl9.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang10[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang10.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl10[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl10.inc"
};
static NJS_MKEY_A ev0091_egg_carrier_ang11[30] = {
#include "ev0091/script/ev0091_egg_carrier_ang11.inc"
};
static NJS_MKEY_F ev0091_egg_carrier_scl11[30] = {
#include "ev0091/script/ev0091_egg_carrier_scl11.inc"
};
static NJS_MDATA3 ev0091_egg_carrier_mdata[12] = {
#include "ev0091/script/ev0091_egg_carrier_mdata.inc"
};
static NJS_MOTION ev0091_egg_carrier_motion = {
#include "ev0091/script/ev0091_egg_carrier_motion.inc"
};
static NJS_ACTION ev0091_egg_carrier_action = {
#include "ev0091/script/ev0091_egg_carrier_action.inc"
};

/* no code uses this word. it keeps ev0091_fade at its address in the image */
static Uint32 dat_0cb96414 = 0;

/* the screen's fade: white as the event opens, then green, and near black
 * as the scene cuts away */
static struct task *ev0091_fade = NULL;

void ev0091_script(int phase)
{
    /* @hack spelling: the Master Emerald's flag through a local, which SHC
     * keeps in a stack slot from the first store to the second (0x0cb80080,
     * 0x0cb809fe). stored by name, script.c's code ends 4 bytes short of the
     * image's, and 1866 of 2067 words differ */
    Uint8 *form;
    /* @hack spelling: event_talk_start called through a local. SHC then keeps
     * the Egg Carrier's x, -2.7, in FR12 and its z, 2153.4, in FR14
     * (0x0cb800d4, 0x0cb800d8). called by name, the two trade registers, and 6
     * of 2067 words differ */
    void (*talk_start)(struct task *task, const char *text);
    /* @hack spelling: event_voice_play called through a local. SHC then keeps
     * its address in R9 and event_talk_start's in R8 (0x0cb80300, 0x0cb8025a).
     * called by name, those two trade registers, as do FR12 and FR14, and 34 of
     * 2067 words differ */
    void (*voice_play)(int voice);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_0091.mlt");
        event_sound_init(4);
        /* the emerald's object sets the same for Knuckles once event 130
         * has played */
        *(form = &event_flags[FLAG_MASTER_EMERALD]) = MASTER_EMERALD_SHATTERED;
        event_set_pos(event_get_player(0), -3.4F, -247.0F, 2469.6F);
        event_set_ang(event_get_player(0), 0, 0x8900, 0);
        event_create_player(2, e102_step, -37.3F, -238.0F, 1973.0F, 0, 0, 0);
        event_wait(1);
        event_prop_create(-2.7F, -160.6F, 2153.4F, 0, 0xad00, 0, 0.002F, 0.002F,
                          0.002F, &ev0091_egg_carrier_object,
                          &ev0091_egg_carrier_texlist, EGG_CARRIER);
        event_prop_set_action(EGG_CARRIER, &ev0091_egg_carrier_action, 0);
        event_prop_play(EGG_CARRIER, 0, 1.0F, EVENT_PROP_FOREVER);
        event_sound_start(1, SE_TIKAL_CHIME, 90);
        /* the white holds at 0.95 for the 30 steps to the next speed. a
         * speed of -1e-12 changes nothing: 0.95 less 1e-12 rounds back to
         * 0.95 as a float */
        ev0091_fade = event_fade_create(-1e-12F, 0.95F, 1.0F, 1.0F, 1.0F);
        event_wait(30);
        if (ev0091_fade != NULL) {
            event_fade_set_speed(ev0091_fade, -0.018F);
        }
        stream_request_music(MUSIC_MSTCLN);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -124.2F, -211.4F, 2455.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0xd615, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -80.9F, -211.7F, 2527.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0xf700, 0xda15, 0);
        event_wait(152);
        event_talk_stop(event_get_player(0));
        talk_start = event_talk_start;
        talk_start(event_get_player(0), TEXT_35);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -9.4F, -244.5F, 2463.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0720, 0x9e15, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -9.0F, -242.0F, 2462.8F);
        event_wait(100);
        event_talk_stop(event_get_player(0));
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.7F, -241.9F, 2480.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf150, 0xfd70 + 0x10000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, -4.07F, -243.5F, 2499.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x0c50, 0xfe70, 0);
        event_wait(60);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_36);
        voice_play = event_voice_play;
        voice_play(1614);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6.0F, -240.73F, 2455.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x024f, 0x667a, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, 3.0F, -240.4F, 2460.1F);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_36);
        voice_play(1615);
        event_message_show(TEXT_2);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(50);
        event_play_action(event_get_player(0), &ev0091_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_37);
        voice_play(1616);
        event_message_show(TEXT_3);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -28.45F, -244.0F, 2483.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x084f, 0xd57a, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -23.7F, -244.1F, 2487.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0x094f, 0xee7a, 0xff00);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_38);
        voice_play(1617);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_39);
        voice_play(1618);
        event_message_show(TEXT_5);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(34);
        event_set_pos(event_get_player(0), -4.4F, -157.0F, 2252.5F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_run_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -2.87F, -158.0F, 2201.9F, 0.4F,
                           0.07F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -8.79F, -141.1F, 2301.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf84f, 0xfc7a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -8.65F, -135.3F, 2301.3F);
        event_wait(20);
        event_run_to_speed(event_get_player(0), -2.87F, -158.0F, 2201.9F, 0.5F,
                           1.0F);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 23.94F, -158.0F, 2227.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x074f, 0x187a, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 23.94F, -156.0F, 2227.2F);
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 0.9F, EVENT_REPEAT, 8);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -7.45F, -153.6F, 2195.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa4f, 0x9b7a, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -5.57F, -150.67F, 2194.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xfc4f, 0x8e7a, 0);
        event_play_action(event_get_player(0), &ev0091_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -6.6F, -150.4F, 2192.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xfc4f, 0x8e7a, 0);
        event_wait(20);
        event_orbs_start(12);
        event_wait(15);
        event_rays_create(-2.87F, -151.0F, 2197.9F, 1, 10, 0.4F, 1.2F,
                          0xff50dc50, 0x00325a32);
        event_wait(45);
        event_sound_play(1225, 128, 0, 0);
        event_wait(20);
        event_play_action(event_get_player(0), &ev0091_knuckles_action_2,
                          &knuckles_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &ev0091_knuckles_action_3,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(25);
        event_rays_kill();
        event_wait(20);
        event_sound_start(2, 1224, 1800);
        event_sound_fade_volume(2, -10, 1);
        event_wait(1);
        event_sound_fade_volume(2, 90, 220);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -34.6F, -147.6F, 2162.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x084f, 0xaa7a, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -55.57F, -155.6F, 2156.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x284f, 0xb17a, 0x0e00);
        event_wait(60);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x304f, 0xbb7a, 0x0e00);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 44.57F, -14.4F, 2195.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd54f, 0x1e7a, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -17.45F, -150.9F, 2223.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x044f, 0xf57a, 0);
        event_wait(100);
        event_sound_stop(2);
        event_sound_play(1225, 128, 0, 0);
        event_sound_play(1224, 120, 0, 0);
        event_rays_create(-3.52F, -136.0F, 2151.0F, 2, 12, 2.0F, 5.0F,
                          0xff50dc50, 0x00325a32);
        if (ev0091_fade != NULL) {
            event_fade_set_speed(ev0091_fade, 0.028F);
        }
        event_wait(40);
        /* the emerald's object sets the same for Knuckles once this event,
         * 145, has played */
        *form = MASTER_EMERALD_INCOMPLETE;
        event_wait(20);
        if (ev0091_fade != NULL) {
            event_fade_set_speed(ev0091_fade, -0.022F);
        }
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1.76F, -153.8F, 2193.46F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x084f, 0x697a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 0.76F, -153.0F, 2195.13F);
        event_wait(80);
        event_rays_kill();
        event_play_action(event_get_player(0), &ev0091_knuckles_action_0,
                          &knuckles_texlist, 0.9F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, -0.2F, -152.5F, 2196.66F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0x144f, 0x6a7a, 0);
        event_wait(45);
        event_play_action(event_get_player(0), &ev0091_knuckles_action_4,
                          &knuckles_texlist, 0.9F, EVENT_REPEAT, 4);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 24.6F, -125.7F, 2169.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xdb4f, 0x447a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 20.76F, -123.2F, 2163.0F);
        event_run_to_speed(event_get_player(0), -2.48F, -157.0F, 2185.4F, 0.4F,
                           0.1F);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_39);
        voice_play(1619);
        event_message_show(TEXT_6);
        event_talk_stop(event_get_player(0));
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -7.3F, -149.3F, 2187.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a4f, 0xfb7a, 0);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 23.0F, -150.98F, 2192.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x024f, 0x277a, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 19.6F, -150.98F, 2197.0F);
        talk_start(event_get_player(0), TEXT_39);
        voice_play(1620);
        event_message_show(TEXT_7);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 19.6F, -150.98F, 2197.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0x084f, 0x1f7a, 0);
        event_move(event_prop_get(EGG_CARRIER), -2.7F, -160.6F, 2153.4F, -2.7F,
                   -139.6F, 2153.4F, 60);
        event_wait(60);
        event_sound_play(1228, 128, 0, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.58F, -153.4F, 2180.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e4f, 0x797a, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 8, 0x184f, 0x777a, 0);
        event_wait(30);
        event_set_pos(event_get_player(0), 6.3F, -152.6F, 2173.44F);
        event_set_ang(event_get_player(0), 0, 0x8d78, 0);
        event_play_action(event_get_player(0), &ev0091_knuckles_action_5,
                          &knuckles_texlist, 0.6F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.4F, -149.5F, 2183.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a4f, 0x037a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 3.3F, -149.5F, 2183.0F);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_40);
        voice_play(1621);
        event_message_show(TEXT_8);
        event_camera_set_fov(EVENT_EASE_LINEAR, 70, 0x2aab);
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.23F, -145.86F, 2177.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c4f, 0x027a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -1.469F, -144.6F, 2173.2F);
        event_wait(80);
        EVENT_KILL(ev0091_fade);
        ev0091_fade = event_fade_create(-0.002F, 0.4F, 0.0F, 0.8F, 0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -14.2F, -137.12F, 2152.93F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xec4f, 0xc37a, 0x0200);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, -6.783F, -135.87F,
                             2157.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0xe74f, 0xd77a, 0x0200);
        event_wait(15);
        voice_play(1622);
        event_message_show(TEXT_9);
        event_wait(105);
        event_message_close();
        if (ev0091_fade != NULL) {
            event_fade_set_color(ev0091_fade, 0.0F, 0.05F, 0.0F);
        }
        if (ev0091_fade != NULL) {
            event_fade_set_alpha(ev0091_fade, 0.2F);
        }
        if (ev0091_fade != NULL) {
            event_fade_set_speed(ev0091_fade, 0.01F);
        }
        event_wait(50);
        if (ev0091_fade != NULL) {
            event_fade_set_speed(ev0091_fade, -0.02F);
        }
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_walk_action,
                          &knuckles_texlist, 0.7F, EVENT_REPEAT, 16);
        event_set_pos(event_get_player(0), -1475.3F, -408.1F, 2126.7F);
        event_set_ang(event_get_player(0), 0, 0xbd30, 0);
        event_run_to_speed(event_get_player(0), -1529.97F, -401.1F, 2122.7F,
                           1.0F, 0.1F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1540.4F, -398.12F, 2117.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0043, 0xbf08, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x0043, 0xaf08, 0);
        event_run_wait(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -1538.4F, -398.12F,
                             2118.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x0443, 0xaf08, 0);
        event_play_action(event_get_player(0), &ev0091_knuckles_action_6,
                          &knuckles_texlist, 0.6F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_41);
        voice_play(1623);
        event_message_show(TEXT_10);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(40);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1535.9F, -396.9F, 2125.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x054f, 0xce7a, 0);
        event_play_action(event_get_player(0), &ev0091_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 4, -1534.9F, -396.7F, 2125.2F);
        event_wait(25);
        event_set_pos(event_get_player(2), -1657.0F, -320.11F, 2108.52F);
        event_set_ang(event_get_player(2), 0xfb00, 0xc47a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1516.1F, -402.2F, 2132.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f4f, 0x357a, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -1516.1F, -402.2F, 2131.2F);
        stream_stop_music();
        stream_request_music(MUSIC_THEE102);
        event_wait(10);
        /* from here the camera jolts in y for 16 steps, and pad 0 rumbles */
        event_sound_play(1226, 60, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -1516.1F, -402.2F, 2131.2F);
        event_wait(2);
        vibration_start(0, 2, 60, 3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -1516.1F, -403.0F, 2131.2F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -1516.1F, -402.2F, 2131.2F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, -1516.1F, -404.0F, 2131.2F);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -1516.1F, -403.2F, 2131.2F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -1516.1F, -402.2F, 2131.2F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, -1516.1F, -403.0F, 2131.2F);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -1516.1F, -398.2F, 2131.2F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 30, event_get_player(2),
                                  0.0F, 12.0F, 0.0F, 0);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 0.0F,
                           0.0F);
        event_wait(30);
        event_run_to_speed(event_get_player(2), -1709.3F, -380.0F, 1764.9F,
                           4.0F, 0.1F);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1653.1F, -387.2F, 1850.2F);
        event_wait(85);
        event_camera_look_clear();
        event_set_ang(event_get_player(0), 0x0250, 0xa97a, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1533.5F, -398.4F, 2140.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x09e3, 0x0618, 0x0a00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, -1528.7F, -398.4F, 2139.8F);
        event_sound_start(1, 1227, 1800);
        event_sound_fade_volume(1, 80, 1);
        event_wait(1);
        event_sound_fade_volume(1, 0, 120);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_42);
        voice_play(1624);
        event_message_show(TEXT_11);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1536.2F, -396.0F, 2112.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xffe3, 0x9718, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, -1532.0F, -395.9F,
                             2117.4F);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_43);
        voice_play(1625);
        event_message_show(TEXT_12);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 25, -1533.0F, -396.87F,
                             2119.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 25, 0x09e3, 0xa518, 0xfd00);
        event_talk_stop(event_get_player(0));
        talk_start(event_get_player(0), TEXT_44);
        voice_play(1626);
        event_message_show(TEXT_13);
        event_voice_wait();
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_wait(80);
        event_run_clear(event_get_player(2));
        event_look_clear(event_get_player(0));
        event_move_stop_all();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_move_stop_all();
        event_reset_player(0);
        event_run_clear(event_get_player(2));
        event_kill_player(2);
        event_look_clear(event_get_player(0));
        event_voice_stop();
        EVENT_KILL(ev0091_fade);
        event_prop_kill(EGG_CARRIER);
        event_rays_kill();
        event_set_pos(event_get_player(0), -1529.97F, -401.1F, 2122.7F);
        event_set_ang(event_get_player(0), 0, 0xbd30, 0);
        event_orbs_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0091_textures[] = {
    {"EC_LIGHT", &ev0091_egg_carrier_texlist},
    {NULL, NULL},
};
