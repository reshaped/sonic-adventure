/* the Egg Hornet in the events: its root node and action, Eggman's body in
 * it, and their task */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "shared/hornet.h"
#include "shared/hornet_model.h" /* NOLINT(misc-include-cleaner) */
#include "shared/jets.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 7
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 6
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 6
#define TEXT_24_LENGTH 7
#define TEXT_25_LENGTH 7
#define TEXT_26_LENGTH 8
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 8
#define TEXT_29_LENGTH 8
#define TEXT_30_LENGTH 8
#define TEXT_31_LENGTH 6
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 6
#define TEXT_34_LENGTH 6
#define TEXT_35_LENGTH 6
#define TEXT_36_LENGTH 5
#define TEXT_37_LENGTH 5
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 5
#include "shared/hornet/text.inc"

/* the hornet arrives at its point once within this of it on every axis */
#define HORNET_NEAR 1.0F
/* how far the bob takes the drawn hornet and Eggman above and below pos */
#define HORNET_BOB_HEIGHT 1.0F
/* the degrees a step that the bob's phase advances */
#define HORNET_BOB_RATE 5.0F
/* the frames a step that the hornet's action plays, either way */
#define HORNET_PLAY_RATE 0.2F
/* a new hornet's speed at either end of a flight, in units a step */
#define HORNET_SPEED 0.1F
/* the y angle the hornet steers by in a step of a flight. it also sets a
 * window about 90 degrees from across. while the way to the point lies in
 * it, the hornet turns by its offset from 90 degrees */
#define HORNET_STEER NJM_DEG_ANG(1)
/* the hornet's bank changes by this a step */
#define HORNET_BANK_RATE NJM_DEG_ANG(0.5)
/* steering banks the hornet no further once its bank is past this */
#define HORNET_BANK_LIMIT NJM_DEG_ANG(30)
/* the share of its offset from its parent that a scattering part moves out
 * by a step */
#define HORNET_SCATTER_RATE 0.01F

/* the hornet's across and ahead at angle 0, which hornet_get_across and
 * hornet_get_ahead turn about y */
static const NJS_VECTOR hornet_across = {1.0F, 0.0F, 0.0F};
static const NJS_VECTOR hornet_ahead = {0.0F, 0.0F, 1.0F};

static NJS_OBJECT event_hornet_object = {
#include "shared/hornet/event_hornet_object.inc"
};
static NJS_TEXNAME event_hornet_textures[25] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0},
};
NJS_TEXLIST event_hornet_texlist = {
#include "shared/hornet/event_hornet_texlist.inc"
};
static NJS_MKEY_F event_hornet_motion_pos0[2] = {
#include "shared/hornet/event_hornet_motion_pos0.inc"
};
static NJS_MKEY_A event_hornet_motion_ang0[22] = {
#include "shared/hornet/event_hornet_motion_ang0.inc"
};
static NJS_MKEY_F event_hornet_motion_pos1[2] = {
#include "shared/hornet/event_hornet_motion_pos1.inc"
};
static NJS_MKEY_A event_hornet_motion_ang1[2] = {
#include "shared/hornet/event_hornet_motion_ang1.inc"
};
static NJS_MKEY_F event_hornet_motion_pos2[2] = {
#include "shared/hornet/event_hornet_motion_pos2.inc"
};
static NJS_MKEY_A event_hornet_motion_ang2[2] = {
#include "shared/hornet/event_hornet_motion_ang2.inc"
};
static NJS_MKEY_F event_hornet_motion_pos3[2] = {
#include "shared/hornet/event_hornet_motion_pos3.inc"
};
static NJS_MKEY_A event_hornet_motion_ang3[2] = {
#include "shared/hornet/event_hornet_motion_ang3.inc"
};
static NJS_MKEY_F event_hornet_motion_pos4[2] = {
#include "shared/hornet/event_hornet_motion_pos4.inc"
};
static NJS_MKEY_A event_hornet_motion_ang4[30] = {
#include "shared/hornet/event_hornet_motion_ang4.inc"
};
static NJS_MKEY_F event_hornet_motion_pos5[2] = {
#include "shared/hornet/event_hornet_motion_pos5.inc"
};
static NJS_MKEY_F event_hornet_motion_pos6[2] = {
#include "shared/hornet/event_hornet_motion_pos6.inc"
};
static NJS_MKEY_F event_hornet_motion_pos7[2] = {
#include "shared/hornet/event_hornet_motion_pos7.inc"
};
static NJS_MKEY_F event_hornet_motion_pos8[2] = {
#include "shared/hornet/event_hornet_motion_pos8.inc"
};
static NJS_MKEY_A event_hornet_motion_ang8[2] = {
#include "shared/hornet/event_hornet_motion_ang8.inc"
};
static NJS_MKEY_F event_hornet_motion_pos9[2] = {
#include "shared/hornet/event_hornet_motion_pos9.inc"
};
static NJS_MKEY_A event_hornet_motion_ang9[30] = {
#include "shared/hornet/event_hornet_motion_ang9.inc"
};
static NJS_MKEY_F event_hornet_motion_pos10[2] = {
#include "shared/hornet/event_hornet_motion_pos10.inc"
};
static NJS_MKEY_F event_hornet_motion_pos11[2] = {
#include "shared/hornet/event_hornet_motion_pos11.inc"
};
static NJS_MKEY_F event_hornet_motion_pos12[2] = {
#include "shared/hornet/event_hornet_motion_pos12.inc"
};
static NJS_MKEY_A event_hornet_motion_ang12[2] = {
#include "shared/hornet/event_hornet_motion_ang12.inc"
};
static NJS_MKEY_F event_hornet_motion_pos13[2] = {
#include "shared/hornet/event_hornet_motion_pos13.inc"
};
static NJS_MKEY_A event_hornet_motion_ang13[21] = {
#include "shared/hornet/event_hornet_motion_ang13.inc"
};
static NJS_MKEY_F event_hornet_motion_pos14[2] = {
#include "shared/hornet/event_hornet_motion_pos14.inc"
};
static NJS_MKEY_A event_hornet_motion_ang14[2] = {
#include "shared/hornet/event_hornet_motion_ang14.inc"
};
static NJS_MDATA2 event_hornet_motion_mdata[15] = {
#include "shared/hornet/event_hornet_motion_mdata.inc"
};
static NJS_MOTION event_hornet_motion = {
#include "shared/hornet/event_hornet_motion.inc"
};
/* @hack type: an array of one. SHC then loads the action's address and reads
 * its motion through it (MOV.L @(4,R4),R3 at 0x0cb833e8 in EV0007). as one
 * NJS_ACTION, it loads the motion's address instead, and event_hornet_step runs
 * 2 bytes short. summed over event_hornet_step and event_hornet_draw, 216 of
 * 506 words differ */
static NJS_ACTION event_hornet_action[1] = {
#include "shared/hornet/event_hornet_action.inc"
};
static NJS_MATERIAL event_eggman_body_13_materials[1] = {
#include "shared/hornet/event_eggman_body_13_materials.inc"
};
static Sint16 event_eggman_body_13_meshes0[14] = {
#include "shared/hornet/event_eggman_body_13_meshes0.inc"
};
static NJS_COLOR event_eggman_body_13_uvs0[12] = {
#include "shared/hornet/event_eggman_body_13_uvs0.inc"
};
static NJS_MESHSET event_eggman_body_13_meshsets[1] = {
#include "shared/hornet/event_eggman_body_13_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_13_points[9] = {
#include "shared/hornet/event_eggman_body_13_points.inc"
};
static NJS_VECTOR event_eggman_body_13_normals[9] = {
#include "shared/hornet/event_eggman_body_13_normals.inc"
};
static NJS_MODEL event_eggman_body_13_model = {
#include "shared/hornet/event_eggman_body_13_model.inc"
};
static NJS_OBJECT event_eggman_body_13 = {
#include "shared/hornet/event_eggman_body_13.inc"
};
static NJS_MATERIAL event_eggman_body_12_materials[2] = {
#include "shared/hornet/event_eggman_body_12_materials.inc"
};
static Sint16 event_eggman_body_12_meshes0[20] = {
#include "shared/hornet/event_eggman_body_12_meshes0.inc"
};
static NJS_COLOR event_eggman_body_12_uvs0[18] = {
#include "shared/hornet/event_eggman_body_12_uvs0.inc"
};
static NJS_MESHSET event_eggman_body_12_meshsets[1] = {
#include "shared/hornet/event_eggman_body_12_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_12_points[10] = {
#include "shared/hornet/event_eggman_body_12_points.inc"
};
static NJS_VECTOR event_eggman_body_12_normals[10] = {
#include "shared/hornet/event_eggman_body_12_normals.inc"
};
static NJS_MODEL event_eggman_body_12_model = {
#include "shared/hornet/event_eggman_body_12_model.inc"
};
static NJS_OBJECT event_eggman_body_12 = {
#include "shared/hornet/event_eggman_body_12.inc"
};
static NJS_MATERIAL event_eggman_body_11_materials[1] = {
#include "shared/hornet/event_eggman_body_11_materials.inc"
};
static Sint16 event_eggman_body_11_meshes0[19] = {
#include "shared/hornet/event_eggman_body_11_meshes0.inc"
};
static NJS_MESHSET event_eggman_body_11_meshsets[1] = {
#include "shared/hornet/event_eggman_body_11_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_11_points[8] = {
#include "shared/hornet/event_eggman_body_11_points.inc"
};
static NJS_VECTOR event_eggman_body_11_normals[8] = {
#include "shared/hornet/event_eggman_body_11_normals.inc"
};
static NJS_MODEL event_eggman_body_11_model = {
#include "shared/hornet/event_eggman_body_11_model.inc"
};
static NJS_OBJECT event_eggman_body_11 = {
#include "shared/hornet/event_eggman_body_11.inc"
};
static NJS_MATERIAL event_eggman_body_10_materials[3] = {
#include "shared/hornet/event_eggman_body_10_materials.inc"
};
static Sint16 event_eggman_body_10_meshes0[10] = {
#include "shared/hornet/event_eggman_body_10_meshes0.inc"
};
static Sint16 event_eggman_body_10_meshes1[5] = {
#include "shared/hornet/event_eggman_body_10_meshes1.inc"
};
static Sint16 event_eggman_body_10_meshes2[8] = {
#include "shared/hornet/event_eggman_body_10_meshes2.inc"
};
static NJS_COLOR event_eggman_body_10_uvs1[4] = {
#include "shared/hornet/event_eggman_body_10_uvs1.inc"
};
static NJS_COLOR event_eggman_body_10_uvs2[6] = {
#include "shared/hornet/event_eggman_body_10_uvs2.inc"
};
static NJS_MESHSET event_eggman_body_10_meshsets[3] = {
#include "shared/hornet/event_eggman_body_10_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_10_points[8] = {
#include "shared/hornet/event_eggman_body_10_points.inc"
};
static NJS_VECTOR event_eggman_body_10_normals[8] = {
#include "shared/hornet/event_eggman_body_10_normals.inc"
};
static NJS_MODEL event_eggman_body_10_model = {
#include "shared/hornet/event_eggman_body_10_model.inc"
};
static NJS_OBJECT event_eggman_body_10 = {
#include "shared/hornet/event_eggman_body_10.inc"
};
static NJS_MATERIAL event_eggman_body_9_materials[5] = {
#include "shared/hornet/event_eggman_body_9_materials.inc"
};
static Sint16 event_eggman_body_9_meshes0[16] = {
#include "shared/hornet/event_eggman_body_9_meshes0.inc"
};
static Sint16 event_eggman_body_9_meshes1[11] = {
#include "shared/hornet/event_eggman_body_9_meshes1.inc"
};
static Sint16 event_eggman_body_9_meshes2[19] = {
#include "shared/hornet/event_eggman_body_9_meshes2.inc"
};
static Sint16 event_eggman_body_9_meshes3[5] = {
#include "shared/hornet/event_eggman_body_9_meshes3.inc"
};
static NJS_COLOR event_eggman_body_9_uvs1[10] = {
#include "shared/hornet/event_eggman_body_9_uvs1.inc"
};
static NJS_COLOR event_eggman_body_9_uvs2[16] = {
#include "shared/hornet/event_eggman_body_9_uvs2.inc"
};
static NJS_COLOR event_eggman_body_9_uvs3[4] = {
#include "shared/hornet/event_eggman_body_9_uvs3.inc"
};
static NJS_MESHSET event_eggman_body_9_meshsets[4] = {
#include "shared/hornet/event_eggman_body_9_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_9_points[26] = {
#include "shared/hornet/event_eggman_body_9_points.inc"
};
static NJS_VECTOR event_eggman_body_9_normals[26] = {
#include "shared/hornet/event_eggman_body_9_normals.inc"
};
static NJS_MODEL event_eggman_body_9_model = {
#include "shared/hornet/event_eggman_body_9_model.inc"
};
static NJS_OBJECT event_eggman_body_9 = {
#include "shared/hornet/event_eggman_body_9.inc"
};
static NJS_MATERIAL event_eggman_body_8_materials[1] = {
#include "shared/hornet/event_eggman_body_8_materials.inc"
};
static Sint16 event_eggman_body_8_meshes0[52] = {
#include "shared/hornet/event_eggman_body_8_meshes0.inc"
};
static NJS_COLOR event_eggman_body_8_uvs0[41] = {
#include "shared/hornet/event_eggman_body_8_uvs0.inc"
};
static NJS_MESHSET event_eggman_body_8_meshsets[1] = {
#include "shared/hornet/event_eggman_body_8_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_8_points[13] = {
#include "shared/hornet/event_eggman_body_8_points.inc"
};
static NJS_VECTOR event_eggman_body_8_normals[13] = {
#include "shared/hornet/event_eggman_body_8_normals.inc"
};
static NJS_MODEL event_eggman_body_8_model = {
#include "shared/hornet/event_eggman_body_8_model.inc"
};
static NJS_OBJECT event_eggman_body_8 = {
#include "shared/hornet/event_eggman_body_8.inc"
};
static NJS_MATERIAL event_eggman_body_7_materials[2] = {
#include "shared/hornet/event_eggman_body_7_materials.inc"
};
static Sint16 event_eggman_body_7_meshes0[20] = {
#include "shared/hornet/event_eggman_body_7_meshes0.inc"
};
static NJS_COLOR event_eggman_body_7_uvs0[18] = {
#include "shared/hornet/event_eggman_body_7_uvs0.inc"
};
static NJS_MESHSET event_eggman_body_7_meshsets[1] = {
#include "shared/hornet/event_eggman_body_7_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_7_points[10] = {
#include "shared/hornet/event_eggman_body_7_points.inc"
};
static NJS_VECTOR event_eggman_body_7_normals[10] = {
#include "shared/hornet/event_eggman_body_7_normals.inc"
};
static NJS_MODEL event_eggman_body_7_model = {
#include "shared/hornet/event_eggman_body_7_model.inc"
};
static NJS_OBJECT event_eggman_body_7 = {
#include "shared/hornet/event_eggman_body_7.inc"
};
static NJS_MATERIAL event_eggman_body_6_materials[1] = {
#include "shared/hornet/event_eggman_body_6_materials.inc"
};
static Sint16 event_eggman_body_6_meshes0[19] = {
#include "shared/hornet/event_eggman_body_6_meshes0.inc"
};
static NJS_MESHSET event_eggman_body_6_meshsets[1] = {
#include "shared/hornet/event_eggman_body_6_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_6_points[8] = {
#include "shared/hornet/event_eggman_body_6_points.inc"
};
static NJS_VECTOR event_eggman_body_6_normals[8] = {
#include "shared/hornet/event_eggman_body_6_normals.inc"
};
static NJS_MODEL event_eggman_body_6_model = {
#include "shared/hornet/event_eggman_body_6_model.inc"
};
static NJS_OBJECT event_eggman_body_6 = {
#include "shared/hornet/event_eggman_body_6.inc"
};
static NJS_MATERIAL event_eggman_body_5_materials[3] = {
#include "shared/hornet/event_eggman_body_5_materials.inc"
};
static Sint16 event_eggman_body_5_meshes0[10] = {
#include "shared/hornet/event_eggman_body_5_meshes0.inc"
};
static Sint16 event_eggman_body_5_meshes1[5] = {
#include "shared/hornet/event_eggman_body_5_meshes1.inc"
};
static Sint16 event_eggman_body_5_meshes2[8] = {
#include "shared/hornet/event_eggman_body_5_meshes2.inc"
};
static NJS_COLOR event_eggman_body_5_uvs1[4] = {
#include "shared/hornet/event_eggman_body_5_uvs1.inc"
};
static NJS_COLOR event_eggman_body_5_uvs2[6] = {
#include "shared/hornet/event_eggman_body_5_uvs2.inc"
};
static NJS_MESHSET event_eggman_body_5_meshsets[3] = {
#include "shared/hornet/event_eggman_body_5_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_5_points[8] = {
#include "shared/hornet/event_eggman_body_5_points.inc"
};
static NJS_VECTOR event_eggman_body_5_normals[8] = {
#include "shared/hornet/event_eggman_body_5_normals.inc"
};
static NJS_MODEL event_eggman_body_5_model = {
#include "shared/hornet/event_eggman_body_5_model.inc"
};
static NJS_OBJECT event_eggman_body_5 = {
#include "shared/hornet/event_eggman_body_5.inc"
};
static NJS_MATERIAL event_eggman_body_4_materials[5] = {
#include "shared/hornet/event_eggman_body_4_materials.inc"
};
static Sint16 event_eggman_body_4_meshes0[16] = {
#include "shared/hornet/event_eggman_body_4_meshes0.inc"
};
static Sint16 event_eggman_body_4_meshes1[11] = {
#include "shared/hornet/event_eggman_body_4_meshes1.inc"
};
static Sint16 event_eggman_body_4_meshes2[19] = {
#include "shared/hornet/event_eggman_body_4_meshes2.inc"
};
static Sint16 event_eggman_body_4_meshes3[5] = {
#include "shared/hornet/event_eggman_body_4_meshes3.inc"
};
static NJS_COLOR event_eggman_body_4_uvs1[10] = {
#include "shared/hornet/event_eggman_body_4_uvs1.inc"
};
static NJS_COLOR event_eggman_body_4_uvs2[16] = {
#include "shared/hornet/event_eggman_body_4_uvs2.inc"
};
static NJS_COLOR event_eggman_body_4_uvs3[4] = {
#include "shared/hornet/event_eggman_body_4_uvs3.inc"
};
static NJS_MESHSET event_eggman_body_4_meshsets[4] = {
#include "shared/hornet/event_eggman_body_4_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_4_points[26] = {
#include "shared/hornet/event_eggman_body_4_points.inc"
};
static NJS_VECTOR event_eggman_body_4_normals[26] = {
#include "shared/hornet/event_eggman_body_4_normals.inc"
};
static NJS_MODEL event_eggman_body_4_model = {
#include "shared/hornet/event_eggman_body_4_model.inc"
};
static NJS_OBJECT event_eggman_body_4 = {
#include "shared/hornet/event_eggman_body_4.inc"
};
static NJS_MATERIAL event_eggman_body_3_materials[1] = {
#include "shared/hornet/event_eggman_body_3_materials.inc"
};
static Sint16 event_eggman_body_3_meshes0[52] = {
#include "shared/hornet/event_eggman_body_3_meshes0.inc"
};
static NJS_COLOR event_eggman_body_3_uvs0[41] = {
#include "shared/hornet/event_eggman_body_3_uvs0.inc"
};
static NJS_MESHSET event_eggman_body_3_meshsets[1] = {
#include "shared/hornet/event_eggman_body_3_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_3_points[13] = {
#include "shared/hornet/event_eggman_body_3_points.inc"
};
static NJS_VECTOR event_eggman_body_3_normals[13] = {
#include "shared/hornet/event_eggman_body_3_normals.inc"
};
static NJS_MODEL event_eggman_body_3_model = {
#include "shared/hornet/event_eggman_body_3_model.inc"
};
static NJS_OBJECT event_eggman_body_3 = {
#include "shared/hornet/event_eggman_body_3.inc"
};
static NJS_MATERIAL event_eggman_body_2_materials[1] = {
#include "shared/hornet/event_eggman_body_2_materials.inc"
};
static Sint16 event_eggman_body_2_meshes0[38] = {
#include "shared/hornet/event_eggman_body_2_meshes0.inc"
};
static NJS_COLOR event_eggman_body_2_uvs0[31] = {
#include "shared/hornet/event_eggman_body_2_uvs0.inc"
};
static NJS_MESHSET event_eggman_body_2_meshsets[1] = {
#include "shared/hornet/event_eggman_body_2_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_2_points[15] = {
#include "shared/hornet/event_eggman_body_2_points.inc"
};
static NJS_VECTOR event_eggman_body_2_normals[15] = {
#include "shared/hornet/event_eggman_body_2_normals.inc"
};
static NJS_MODEL event_eggman_body_2_model = {
#include "shared/hornet/event_eggman_body_2_model.inc"
};
static NJS_OBJECT event_eggman_body_2 = {
#include "shared/hornet/event_eggman_body_2.inc"
};
static NJS_MATERIAL event_eggman_body_1_materials[6] = {
#include "shared/hornet/event_eggman_body_1_materials.inc"
};
static Sint16 event_eggman_body_1_meshes0[167] = {
#include "shared/hornet/event_eggman_body_1_meshes0.inc"
};
static Sint16 event_eggman_body_1_meshes1[13] = {
#include "shared/hornet/event_eggman_body_1_meshes1.inc"
};
static Sint16 event_eggman_body_1_meshes2[20] = {
#include "shared/hornet/event_eggman_body_1_meshes2.inc"
};
static Sint16 event_eggman_body_1_meshes3[38] = {
#include "shared/hornet/event_eggman_body_1_meshes3.inc"
};
static Sint16 event_eggman_body_1_meshes4[11] = {
#include "shared/hornet/event_eggman_body_1_meshes4.inc"
};
static NJS_COLOR event_eggman_body_1_uvs0[143] = {
#include "shared/hornet/event_eggman_body_1_uvs0.inc"
};
static NJS_COLOR event_eggman_body_1_uvs1[11] = {
#include "shared/hornet/event_eggman_body_1_uvs1.inc"
};
static NJS_COLOR event_eggman_body_1_uvs2[16] = {
#include "shared/hornet/event_eggman_body_1_uvs2.inc"
};
static NJS_COLOR event_eggman_body_1_uvs3[30] = {
#include "shared/hornet/event_eggman_body_1_uvs3.inc"
};
static NJS_MESHSET event_eggman_body_1_meshsets[5] = {
#include "shared/hornet/event_eggman_body_1_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_1_points[107] = {
#include "shared/hornet/event_eggman_body_1_points.inc"
};
static NJS_VECTOR event_eggman_body_1_normals[107] = {
#include "shared/hornet/event_eggman_body_1_normals.inc"
};
static NJS_MODEL event_eggman_body_1_model = {
#include "shared/hornet/event_eggman_body_1_model.inc"
};
static NJS_OBJECT event_eggman_body_1 = {
#include "shared/hornet/event_eggman_body_1.inc"
};
static NJS_MATERIAL event_eggman_body_materials[6] = {
#include "shared/hornet/event_eggman_body_materials.inc"
};
static Sint16 event_eggman_body_meshes0[97] = {
#include "shared/hornet/event_eggman_body_meshes0.inc"
};
static Sint16 event_eggman_body_meshes1[15] = {
#include "shared/hornet/event_eggman_body_meshes1.inc"
};
static Sint16 event_eggman_body_meshes2[12] = {
#include "shared/hornet/event_eggman_body_meshes2.inc"
};
static Sint16 event_eggman_body_meshes3[14] = {
#include "shared/hornet/event_eggman_body_meshes3.inc"
};
static NJS_COLOR event_eggman_body_uvs0[80] = {
#include "shared/hornet/event_eggman_body_uvs0.inc"
};
static NJS_COLOR event_eggman_body_uvs1[12] = {
#include "shared/hornet/event_eggman_body_uvs1.inc"
};
static NJS_COLOR event_eggman_body_uvs2[10] = {
#include "shared/hornet/event_eggman_body_uvs2.inc"
};
static NJS_COLOR event_eggman_body_uvs3[12] = {
#include "shared/hornet/event_eggman_body_uvs3.inc"
};
static NJS_MESHSET event_eggman_body_meshsets[4] = {
#include "shared/hornet/event_eggman_body_meshsets.inc"
};
static NJS_POINT3 event_eggman_body_points[41] = {
#include "shared/hornet/event_eggman_body_points.inc"
};
static NJS_VECTOR event_eggman_body_normals[41] = {
#include "shared/hornet/event_eggman_body_normals.inc"
};
static NJS_MODEL event_eggman_body_model = {
#include "shared/hornet/event_eggman_body_model.inc"
};
static NJS_OBJECT event_eggman_body_object = {
#include "shared/hornet/event_eggman_body_object.inc"
};
static NJS_TEXNAME event_eggman_body_textures[14] = {
    {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0},
    {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0}, {TEXT_33, 0, 0},
    {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0}, {TEXT_37, 0, 0},
    {TEXT_38, 0, 0}, {TEXT_39, 0, 0},
};
NJS_TEXLIST event_eggman_body_texlist = {
#include "shared/hornet/event_eggman_body_texlist.inc"
};

struct task *event_hornet_task = NULL;

static void event_hornet_init(struct task *task);
static void event_hornet_draw(struct task *task);

void fun_0cb82d64(void)
{
}

void event_hornet_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z)
{
    struct hornet_flight *flight;
    struct task_work *work;
    /* @hack type: the hornet's task through an array of one, which SHC keeps
     * at sp+24, above the arguments it saves on the stack (MOV.L R2,@(24,R15)
     * at 0x0cb82dde in EV0007). as one pointer passed by address, it takes
     * sp+0, the arguments move up 4, the function runs 4 bytes long and 61
     * of its 66 words differ */
    struct task *hornet[1];

    event_hornet_task = task_create(TASK_WORK, 5, event_hornet_init);
    flight = heap_alloc(sizeof(struct hornet_flight));
    work = event_hornet_task->work;
    HORNET(work)->flight = flight;
    work->mode = HORNET_HOVER;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
    /* @hack type: the bob, a Float, cleared through vars.l[0], which stores
     * R6's 0 (MOV.L R6,@(8,R4) at 0x0cb82dc4 in EV0007). as
     * HORNET(work)->bob = 0.0F, FLDI0 loads FR4 for it, the function runs 4
     * bytes long and 41 of its 66 words differ */
    work->vars.l[0] = 0;
    HORNET(work)->frame = 0.0F;
    flight->speed = HORNET_SPEED;
    flight->bank = 0;
    fun_0cb83e9c();
    hornet[0] = event_hornet_task;
    event_jets_create(hornet);
}

void event_hornet_kill(void)
{
    if (event_hornet_task != NULL) {
        task_kill(event_hornet_task);
        event_hornet_task = NULL;
    }
}

void event_hornet_fly_to(Float x, Float y, Float z)
{
    struct task_work *work = event_hornet_task->work;
    struct hornet_flight *flight = HORNET(work)->flight;

    flight->to.x = x;
    flight->to.y = y;
    flight->to.z = z;
    flight->from.x = work->pos.x;
    flight->from.y = work->pos.y;
    flight->from.z = work->pos.z;
    flight->unk_0c[0] = 0.0F;
    flight->unk_0c[1] = 0.0F;
    flight->unk_0c[2] = 0.0F;
    flight->unk_0c[3] = 0.0F;
    flight->unk_0c[4] = 0.0F;
    flight->unk_0c[5] = 0.0F;
    work->mode = HORNET_FLY;
}

void event_hornet_play(void)
{
    struct task_work *work = event_hornet_task->work;

    work->mode = HORNET_PLAY;
}

void event_hornet_place(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                        Angle ang_z)
{
    struct task_work *work = event_hornet_task->work;

    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
}

void event_hornet_turn(Angle x, Angle y, Angle z, int steps)
{
    struct task_work *work = event_hornet_task->work;
    struct hornet_flight *flight = HORNET(work)->flight;

    flight->turn.x = (x - work->ang.x) / steps;
    flight->turn.y = (y - work->ang.y) / steps;
    flight->turn.z = (z - work->ang.z) / steps;
    flight->steps = steps;
    work->mode = HORNET_TURN;
}

void event_hornet_jets_off(void)
{
    event_jets_set_mode(JETS_OFF);
}

void event_hornet_jets_on(void)
{
    event_jets_set_mode(JETS_ON);
}

void event_hornet_jets_boost(void)
{
    event_jets_set_mode(JETS_BOOST);
}

void event_hornet_jets_hold(void)
{
    event_jets_set_mode(JETS_HOLD);
}

void event_hornet_set_speed(Float speed)
{
    struct task_work *work = event_hornet_task->work;
    struct hornet_flight *flight = HORNET(work)->flight;

    flight->speed = speed;
}

void event_hornet_scatter(void)
{
    struct task_work *work = event_hornet_task->work;

    work->mode = HORNET_SCATTER;
}

/* hornet_across, turned about y by ang, into *out */
static void hornet_get_across(Angle ang, NJS_VECTOR *out)
{
    NJS_VECTOR across;

    across = hornet_across;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, ang);
    njCalcVector(NULL, &across, out);
    njPopMatrix(1);
}

/* hornet_ahead, turned about y by ang, into *out */
static void hornet_get_ahead(Angle ang, NJS_VECTOR *out)
{
    NJS_VECTOR ahead;

    ahead = hornet_ahead;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, ang);
    njCalcVector(NULL, &ahead, out);
    njPopMatrix(1);
}

/* make one and other unit vectors in place, and return the cosine of the
 * angle between them */
static Float hornet_normalize_cos(NJS_VECTOR *one, NJS_VECTOR *other)
{
    njUnitVector(one);
    njUnitVector(other);
    return (one->x * other->x) + (one->y * other->y) + (one->z * other->z);
}

/* move pos onto to and return TRUE when pos is within HORNET_NEAR of to on
 * every axis. otherwise return FALSE */
static Bool hornet_arrive(NJS_POINT3 *to, NJS_POINT3 *pos)
{
    if (pos->x > to->x - HORNET_NEAR && pos->x < to->x + HORNET_NEAR) {
        if (pos->y > to->y - HORNET_NEAR && pos->y < to->y + HORNET_NEAR) {
            if (pos->z > to->z - HORNET_NEAR && pos->z < to->z + HORNET_NEAR) {
                pos->x = to->x;
                pos->y = to->y;
                pos->z = to->z;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static Float hornet_distance(NJS_POINT3 one, NJS_POINT3 other)
{
    Float x = one.x - other.x;
    Float y = one.y - other.y;
    Float z = one.z - other.z;
    /* @hack spelling: the distance through a local, which gives y FR6 and z
     * FR5 (0x0cb8307c in EV0007). returned directly, the two trade and 8 of
     * 23 words differ */
    Float distance = _builtin_sqrtf((x * x) + (y * y) + (z * z));

    return distance;
}

/* grow every node's offset from its parent by HORNET_SCATTER_RATE of it */
static void hornet_scatter_tree(NJS_OBJECT *object)
{
    NJS_VECTOR step;

    step.x = object->pos[0] * HORNET_SCATTER_RATE;
    step.y = object->pos[1] * HORNET_SCATTER_RATE;
    step.z = object->pos[2] * HORNET_SCATTER_RATE;
    object->pos[0] += step.x;
    object->pos[1] += step.y;
    object->pos[2] += step.z;
    if (object->child != NULL) {
        hornet_scatter_tree(object->child);
    }
    if (object->sibling != NULL) {
        hornet_scatter_tree(object->sibling);
    }
}

/* draw every node's model at the node's own offset from the current matrix.
 * no node's angles or parents' offsets apply */
static void hornet_draw_parts(NJS_OBJECT *object)
{
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    draw_model(object->model);
    njPopMatrix(1);
    if (object->child != NULL) {
        hornet_draw_parts(object->child);
    }
    if (object->sibling != NULL) {
        hornet_draw_parts(object->sibling);
    }
}

static void event_hornet_step(struct task *task);
static void event_hornet_destroy(struct task *task);

static void event_hornet_init(struct task *task)
{
    task->step = event_hornet_step;
    task->draw = event_hornet_draw;
    task->destructor = event_hornet_destroy;
}

static void event_hornet_step(struct task *task)
{
    struct task_work *work = task->work;
    struct hornet_flight *flight = HORNET(work)->flight;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a vector. the image's ADD #-60,R15 reserves them (0x0cb83166
     * in EV0007); without them, it reserves 48 and 2 of 340 words differ */
    NJS_VECTOR unused;
    NJS_VECTOR across;
    NJS_VECTOR toward;
    NJS_VECTOR ahead;
    Float cosine;
    Float left;
    Float flown;
    Float speed;
    Float from_y;
    Float rise;
    Angle angle;

    switch (work->mode) {
    case HORNET_HOVER:
        HORNET(work)->bob += HORNET_BOB_RATE;
        break;
    case HORNET_FLY:
        HORNET(work)->bob += HORNET_BOB_RATE;
        hornet_get_across(work->ang.y, &across);
        toward.x = flight->to.x - work->pos.x;
        toward.y = flight->to.y - work->pos.y;
        toward.z = flight->to.z - work->pos.z;
        cosine = hornet_normalize_cos(&toward, &across);
        angle = (Angle)(acosf(cosine) * ANGLE_PER_RADIAN);
        /* angle is the angle between across and the way to the point. it is
         * 90 degrees for any point in the upright plane through ahead. that
         * plane holds points behind the hornet as well as ahead. within
         * HORNET_STEER of 90, the y angle turns by 90 degrees less angle,
         * the way steering would. that turn faces a level point ahead. it
         * takes a level point behind twice as far from straight behind. the
         * bank then eases back to level. otherwise the hornet steers toward
         * the point's side by HORNET_STEER, and banks as it turns */
        if (angle >= NJM_DEG_ANG(90) - HORNET_STEER &&
            angle <= NJM_DEG_ANG(90) + HORNET_STEER) {
            work->ang.y -= angle - NJM_DEG_ANG(90);
            if (flight->bank <= HORNET_BANK_RATE &&
                flight->bank >= -HORNET_BANK_RATE) {
                flight->bank = 0;
            } else if (flight->bank > 0) {
                flight->bank -= HORNET_BANK_RATE;
            } else if (flight->bank < 0) {
                flight->bank += HORNET_BANK_RATE;
            }
        } else if (cosine < 0.0F) {
            work->ang.y -= HORNET_STEER;
            if (flight->bank <= HORNET_BANK_LIMIT) {
                flight->bank += HORNET_BANK_RATE;
            }
        } else {
            work->ang.y += HORNET_STEER;
            if (flight->bank >= -HORNET_BANK_LIMIT) {
                flight->bank -= HORNET_BANK_RATE;
            }
        }
        left = hornet_distance(flight->to, work->pos);
        flown = hornet_distance(flight->from, work->pos);
        from_y = flight->from.y;
        rise = flight->to.y - from_y;
        /* the height and the speed follow flown / (left + flown), the share
         * of the way flown. the height runs from from.y to to.y with it. the
         * speed is flight->speed plus the sine of the share times 180
         * degrees. that sine peaks at 1 halfway */
        work->pos.y = from_y + (rise * (flown / (left + flown)));
        speed = njSin((Angle)(((flown * 180.0F) / (left + flown)) *
                              ANGLE_PER_DEGREE)) +
                flight->speed;
        hornet_get_ahead(work->ang.y, &ahead);
        work->pos.x += ahead.x * speed;
        work->pos.y += ahead.y * speed;
        work->pos.z += ahead.z * speed;
        if (hornet_arrive(&flight->to, &work->pos)) {
            work->mode = HORNET_HOVER;
        }
        break;
    case HORNET_TURN:
        HORNET(work)->bob += HORNET_BOB_RATE;
        if (--flight->steps < 0) {
            work->mode = HORNET_HOVER;
        }
        work->ang.x += flight->turn.x;
        work->ang.y += flight->turn.y;
        work->ang.z += flight->turn.z;
        break;
    case HORNET_PLAY:
        HORNET(work)->frame += HORNET_PLAY_RATE;
        if (HORNET(work)->frame >=
            event_hornet_action->motion->nbFrame - 1.0F) {
            HORNET(work)->frame = event_hornet_action->motion->nbFrame - 1.0F;
            work->mode = HORNET_HOVER;
        }
        break;
    case HORNET_SCATTER:
        hornet_scatter_tree(&event_hornet_object);
        break;
    case HORNET_REWIND:
        HORNET(work)->frame -= HORNET_PLAY_RATE;
        if (HORNET(work)->frame <= 0.0F) {
            HORNET(work)->frame = 0.0F;
            work->mode = HORNET_HOVER;
        }
        break;
    }
    event_hornet_draw(task);
}

/* draw Eggman's body, then the hornet around him. both stand at pos, raised
 * by the bob, and turn by ang and then the bank */
static void event_hornet_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct hornet_flight *flight = HORNET(work)->flight;

    njSetTexture(&event_eggman_body_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x,
                work->pos.y +
                    (njSin((Angle)(HORNET(work)->bob * ANGLE_PER_DEGREE)) *
                     HORNET_BOB_HEIGHT),
                work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, flight->bank);
    draw_tree(&event_eggman_body_object);
    njPopMatrix(1);
    njSetTexture(&event_hornet_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x,
                work->pos.y +
                    (njSin((Angle)(HORNET(work)->bob * ANGLE_PER_DEGREE)) *
                     HORNET_BOB_HEIGHT),
                work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, flight->bank);
    switch (work->mode) {
    case HORNET_HOVER:
    case HORNET_FLY:
    case HORNET_TURN:
    case HORNET_PLAY:
    case HORNET_REWIND:
        draw_action(event_hornet_action, HORNET(work)->frame);
        break;
    case HORNET_SCATTER:
        /* no break ends the last case. with one, njPopMatrix's address takes
         * R3, not R2 (0x0cb83596 in EV0007), and 2 of 166 words differ */
        hornet_draw_parts(&event_hornet_object);
    }
    njPopMatrix(1);
}

static void event_hornet_destroy(struct task *task)
{
    struct task_work *work = task->work;

    event_jets_kill();
    if (HORNET(work)->flight != NULL) {
        heap_free(HORNET(work)->flight);
        HORNET(work)->flight = NULL;
    }
}
