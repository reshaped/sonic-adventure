/* EV0030.PRS's plane: Tails' new plane, with Tails in it */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "ev0030/plane.h"
#include "event/event.h"
#include "event/play.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 11
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 11
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 12
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 10
#define TEXT_29_LENGTH 11
#define TEXT_30_LENGTH 12
#define TEXT_31_LENGTH 11
#define TEXT_32_LENGTH 11
#define TEXT_33_LENGTH 10
#define TEXT_34_LENGTH 14
#define TEXT_35_LENGTH 14
#define TEXT_36_LENGTH 12
#include "ev0030/plane/text.inc"

/* the node of ev0030_plane_object that Tails' head is drawn at */
#define PLANE_HEAD 12

/* an animation: the plane's action, and the motion and shape motion of Tails'
 * head. the head is a model of its own, drawn at PLANE_HEAD. all three play
 * at the plane's frame */
struct plane_anim {
    NJS_ACTION *action;
    NJS_MOTION *head_motion;
    NJS_MOTION *head_shape;
    Sint32 next; /* the animation that takes over at this one's last frame */
};

/* the plane's view of work->vars */
struct plane_vars {
    Float speed; /* the frames added to frame each step */
    Float frame;
    Sint32 anim; /* its animation, an index into ev0030_plane_anims */
};

#define VARS(work) ((struct plane_vars *)&(work)->vars)

static NJS_TEXNAME ev0030_plane_textures[33] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0},
};
NJS_TEXLIST ev0030_plane_texlist = {
#include "ev0030/plane/ev0030_plane_texlist.inc"
};
static NJS_TEXNAME ev0030_head_textures[5] = {
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_34, 0, 0},
    {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
};
NJS_TEXLIST ev0030_head_texlist = {
#include "ev0030/plane/ev0030_head_texlist.inc"
};

static NJS_OBJECT ev0030_plane_44 = {
#include "ev0030/plane/ev0030_plane_44.inc"
};
static NJS_MATERIAL ev0030_plane_43_materials[4] = {
#include "ev0030/plane/ev0030_plane_43_materials.inc"
};
static Sint16 ev0030_plane_43_meshes0[28] = {
#include "ev0030/plane/ev0030_plane_43_meshes0.inc"
};
static Sint16 ev0030_plane_43_meshes1[80] = {
#include "ev0030/plane/ev0030_plane_43_meshes1.inc"
};
static Sint16 ev0030_plane_43_meshes2[5] = {
#include "ev0030/plane/ev0030_plane_43_meshes2.inc"
};
static Sint16 ev0030_plane_43_meshes3[26] = {
#include "ev0030/plane/ev0030_plane_43_meshes3.inc"
};
static NJS_COLOR ev0030_plane_43_uvs0[22] = {
#include "ev0030/plane/ev0030_plane_43_uvs0.inc"
};
static NJS_COLOR ev0030_plane_43_uvs1[70] = {
#include "ev0030/plane/ev0030_plane_43_uvs1.inc"
};
static NJS_COLOR ev0030_plane_43_uvs2[4] = {
#include "ev0030/plane/ev0030_plane_43_uvs2.inc"
};
static NJS_COLOR ev0030_plane_43_uvs3[22] = {
#include "ev0030/plane/ev0030_plane_43_uvs3.inc"
};
static NJS_MESHSET ev0030_plane_43_meshsets[4] = {
#include "ev0030/plane/ev0030_plane_43_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_43_points[57] = {
#include "ev0030/plane/ev0030_plane_43_points.inc"
};
static NJS_VECTOR ev0030_plane_43_normals[57] = {
#include "ev0030/plane/ev0030_plane_43_normals.inc"
};
static NJS_MODEL ev0030_plane_43_model = {
#include "ev0030/plane/ev0030_plane_43_model.inc"
};
static NJS_OBJECT ev0030_plane_43 = {
#include "ev0030/plane/ev0030_plane_43.inc"
};
static NJS_OBJECT ev0030_plane_42 = {
#include "ev0030/plane/ev0030_plane_42.inc"
};
static NJS_OBJECT ev0030_plane_41 = {
#include "ev0030/plane/ev0030_plane_41.inc"
};
static NJS_OBJECT ev0030_plane_40 = {
#include "ev0030/plane/ev0030_plane_40.inc"
};
static NJS_OBJECT ev0030_plane_39 = {
#include "ev0030/plane/ev0030_plane_39.inc"
};
static NJS_OBJECT ev0030_plane_38 = {
#include "ev0030/plane/ev0030_plane_38.inc"
};
static NJS_OBJECT ev0030_plane_37 = {
#include "ev0030/plane/ev0030_plane_37.inc"
};
static NJS_MATERIAL ev0030_plane_36_materials[1] = {
#include "ev0030/plane/ev0030_plane_36_materials.inc"
};
static Sint16 ev0030_plane_36_meshes0[16] = {
#include "ev0030/plane/ev0030_plane_36_meshes0.inc"
};
static NJS_COLOR ev0030_plane_36_uvs0[14] = {
#include "ev0030/plane/ev0030_plane_36_uvs0.inc"
};
static NJS_MESHSET ev0030_plane_36_meshsets[1] = {
#include "ev0030/plane/ev0030_plane_36_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_36_points[8] = {
#include "ev0030/plane/ev0030_plane_36_points.inc"
};
static NJS_VECTOR ev0030_plane_36_normals[8] = {
#include "ev0030/plane/ev0030_plane_36_normals.inc"
};
static NJS_MODEL ev0030_plane_36_model = {
#include "ev0030/plane/ev0030_plane_36_model.inc"
};
static NJS_OBJECT ev0030_plane_36 = {
#include "ev0030/plane/ev0030_plane_36.inc"
};
static NJS_MATERIAL ev0030_plane_35_materials[1] = {
#include "ev0030/plane/ev0030_plane_35_materials.inc"
};
static Sint16 ev0030_plane_35_meshes0[11] = {
#include "ev0030/plane/ev0030_plane_35_meshes0.inc"
};
static NJS_COLOR ev0030_plane_35_uvs0[10] = {
#include "ev0030/plane/ev0030_plane_35_uvs0.inc"
};
static NJS_MESHSET ev0030_plane_35_meshsets[1] = {
#include "ev0030/plane/ev0030_plane_35_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_35_points[8] = {
#include "ev0030/plane/ev0030_plane_35_points.inc"
};
static NJS_VECTOR ev0030_plane_35_normals[8] = {
#include "ev0030/plane/ev0030_plane_35_normals.inc"
};
static NJS_MODEL ev0030_plane_35_model = {
#include "ev0030/plane/ev0030_plane_35_model.inc"
};
static NJS_OBJECT ev0030_plane_35 = {
#include "ev0030/plane/ev0030_plane_35.inc"
};
static NJS_MATERIAL ev0030_plane_34_materials[1] = {
#include "ev0030/plane/ev0030_plane_34_materials.inc"
};
static Sint16 ev0030_plane_34_meshes0[35] = {
#include "ev0030/plane/ev0030_plane_34_meshes0.inc"
};
static NJS_COLOR ev0030_plane_34_uvs0[30] = {
#include "ev0030/plane/ev0030_plane_34_uvs0.inc"
};
static NJS_MESHSET ev0030_plane_34_meshsets[1] = {
#include "ev0030/plane/ev0030_plane_34_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_34_points[12] = {
#include "ev0030/plane/ev0030_plane_34_points.inc"
};
static NJS_VECTOR ev0030_plane_34_normals[12] = {
#include "ev0030/plane/ev0030_plane_34_normals.inc"
};
static NJS_MODEL ev0030_plane_34_model = {
#include "ev0030/plane/ev0030_plane_34_model.inc"
};
static NJS_OBJECT ev0030_plane_34 = {
#include "ev0030/plane/ev0030_plane_34.inc"
};
static NJS_MATERIAL ev0030_plane_33_materials[3] = {
#include "ev0030/plane/ev0030_plane_33_materials.inc"
};
static Sint16 ev0030_plane_33_meshes0[20] = {
#include "ev0030/plane/ev0030_plane_33_meshes0.inc"
};
static Sint16 ev0030_plane_33_meshes1[16] = {
#include "ev0030/plane/ev0030_plane_33_meshes1.inc"
};
static Sint16 ev0030_plane_33_meshes2[11] = {
#include "ev0030/plane/ev0030_plane_33_meshes2.inc"
};
static NJS_COLOR ev0030_plane_33_uvs0[16] = {
#include "ev0030/plane/ev0030_plane_33_uvs0.inc"
};
static NJS_COLOR ev0030_plane_33_uvs1[14] = {
#include "ev0030/plane/ev0030_plane_33_uvs1.inc"
};
static NJS_COLOR ev0030_plane_33_uvs2[10] = {
#include "ev0030/plane/ev0030_plane_33_uvs2.inc"
};
static NJS_MESHSET ev0030_plane_33_meshsets[3] = {
#include "ev0030/plane/ev0030_plane_33_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_33_points[16] = {
#include "ev0030/plane/ev0030_plane_33_points.inc"
};
static NJS_VECTOR ev0030_plane_33_normals[16] = {
#include "ev0030/plane/ev0030_plane_33_normals.inc"
};
static NJS_MODEL ev0030_plane_33_model = {
#include "ev0030/plane/ev0030_plane_33_model.inc"
};
static NJS_OBJECT ev0030_plane_33 = {
#include "ev0030/plane/ev0030_plane_33.inc"
};
static NJS_MATERIAL ev0030_plane_32_materials[2] = {
#include "ev0030/plane/ev0030_plane_32_materials.inc"
};
static Sint16 ev0030_plane_32_meshes0[19] = {
#include "ev0030/plane/ev0030_plane_32_meshes0.inc"
};
static Sint16 ev0030_plane_32_meshes1[5] = {
#include "ev0030/plane/ev0030_plane_32_meshes1.inc"
};
static NJS_COLOR ev0030_plane_32_uvs0[16] = {
#include "ev0030/plane/ev0030_plane_32_uvs0.inc"
};
static NJS_COLOR ev0030_plane_32_uvs1[4] = {
#include "ev0030/plane/ev0030_plane_32_uvs1.inc"
};
static NJS_MESHSET ev0030_plane_32_meshsets[2] = {
#include "ev0030/plane/ev0030_plane_32_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_32_points[9] = {
#include "ev0030/plane/ev0030_plane_32_points.inc"
};
static NJS_VECTOR ev0030_plane_32_normals[9] = {
#include "ev0030/plane/ev0030_plane_32_normals.inc"
};
static NJS_MODEL ev0030_plane_32_model = {
#include "ev0030/plane/ev0030_plane_32_model.inc"
};
static NJS_OBJECT ev0030_plane_32 = {
#include "ev0030/plane/ev0030_plane_32.inc"
};
static NJS_MATERIAL ev0030_plane_31_materials[3] = {
#include "ev0030/plane/ev0030_plane_31_materials.inc"
};
static Sint16 ev0030_plane_31_meshes0[28] = {
#include "ev0030/plane/ev0030_plane_31_meshes0.inc"
};
static Sint16 ev0030_plane_31_meshes1[6] = {
#include "ev0030/plane/ev0030_plane_31_meshes1.inc"
};
static Sint16 ev0030_plane_31_meshes2[11] = {
#include "ev0030/plane/ev0030_plane_31_meshes2.inc"
};
static NJS_COLOR ev0030_plane_31_uvs0[23] = {
#include "ev0030/plane/ev0030_plane_31_uvs0.inc"
};
static NJS_COLOR ev0030_plane_31_uvs1[5] = {
#include "ev0030/plane/ev0030_plane_31_uvs1.inc"
};
static NJS_COLOR ev0030_plane_31_uvs2[10] = {
#include "ev0030/plane/ev0030_plane_31_uvs2.inc"
};
static NJS_MESHSET ev0030_plane_31_meshsets[3] = {
#include "ev0030/plane/ev0030_plane_31_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_31_points[18] = {
#include "ev0030/plane/ev0030_plane_31_points.inc"
};
static NJS_VECTOR ev0030_plane_31_normals[18] = {
#include "ev0030/plane/ev0030_plane_31_normals.inc"
};
static NJS_MODEL ev0030_plane_31_model = {
#include "ev0030/plane/ev0030_plane_31_model.inc"
};
static NJS_OBJECT ev0030_plane_31 = {
#include "ev0030/plane/ev0030_plane_31.inc"
};
static NJS_OBJECT ev0030_plane_30 = {
#include "ev0030/plane/ev0030_plane_30.inc"
};
static NJS_OBJECT ev0030_plane_29 = {
#include "ev0030/plane/ev0030_plane_29.inc"
};
static NJS_OBJECT ev0030_plane_28 = {
#include "ev0030/plane/ev0030_plane_28.inc"
};
static NJS_OBJECT ev0030_plane_27 = {
#include "ev0030/plane/ev0030_plane_27.inc"
};
static NJS_OBJECT ev0030_plane_26 = {
#include "ev0030/plane/ev0030_plane_26.inc"
};
static NJS_MATERIAL ev0030_plane_25_materials[1] = {
#include "ev0030/plane/ev0030_plane_25_materials.inc"
};
static Sint16 ev0030_plane_25_meshes0[19] = {
#include "ev0030/plane/ev0030_plane_25_meshes0.inc"
};
static NJS_COLOR ev0030_plane_25_uvs0[16] = {
#include "ev0030/plane/ev0030_plane_25_uvs0.inc"
};
static NJS_MESHSET ev0030_plane_25_meshsets[1] = {
#include "ev0030/plane/ev0030_plane_25_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_25_points[8] = {
#include "ev0030/plane/ev0030_plane_25_points.inc"
};
static NJS_VECTOR ev0030_plane_25_normals[8] = {
#include "ev0030/plane/ev0030_plane_25_normals.inc"
};
static NJS_MODEL ev0030_plane_25_model = {
#include "ev0030/plane/ev0030_plane_25_model.inc"
};
static NJS_OBJECT ev0030_plane_25 = {
#include "ev0030/plane/ev0030_plane_25.inc"
};
static NJS_MATERIAL ev0030_plane_24_materials[1] = {
#include "ev0030/plane/ev0030_plane_24_materials.inc"
};
static Sint16 ev0030_plane_24_meshes0[11] = {
#include "ev0030/plane/ev0030_plane_24_meshes0.inc"
};
static NJS_COLOR ev0030_plane_24_uvs0[10] = {
#include "ev0030/plane/ev0030_plane_24_uvs0.inc"
};
static NJS_MESHSET ev0030_plane_24_meshsets[1] = {
#include "ev0030/plane/ev0030_plane_24_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_24_points[8] = {
#include "ev0030/plane/ev0030_plane_24_points.inc"
};
static NJS_VECTOR ev0030_plane_24_normals[8] = {
#include "ev0030/plane/ev0030_plane_24_normals.inc"
};
static NJS_MODEL ev0030_plane_24_model = {
#include "ev0030/plane/ev0030_plane_24_model.inc"
};
static NJS_OBJECT ev0030_plane_24 = {
#include "ev0030/plane/ev0030_plane_24.inc"
};
static NJS_MATERIAL ev0030_plane_23_materials[1] = {
#include "ev0030/plane/ev0030_plane_23_materials.inc"
};
static Sint16 ev0030_plane_23_meshes0[35] = {
#include "ev0030/plane/ev0030_plane_23_meshes0.inc"
};
static NJS_COLOR ev0030_plane_23_uvs0[30] = {
#include "ev0030/plane/ev0030_plane_23_uvs0.inc"
};
static NJS_MESHSET ev0030_plane_23_meshsets[1] = {
#include "ev0030/plane/ev0030_plane_23_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_23_points[12] = {
#include "ev0030/plane/ev0030_plane_23_points.inc"
};
static NJS_VECTOR ev0030_plane_23_normals[12] = {
#include "ev0030/plane/ev0030_plane_23_normals.inc"
};
static NJS_MODEL ev0030_plane_23_model = {
#include "ev0030/plane/ev0030_plane_23_model.inc"
};
static NJS_OBJECT ev0030_plane_23 = {
#include "ev0030/plane/ev0030_plane_23.inc"
};
static NJS_MATERIAL ev0030_plane_22_materials[3] = {
#include "ev0030/plane/ev0030_plane_22_materials.inc"
};
static Sint16 ev0030_plane_22_meshes0[20] = {
#include "ev0030/plane/ev0030_plane_22_meshes0.inc"
};
static Sint16 ev0030_plane_22_meshes1[16] = {
#include "ev0030/plane/ev0030_plane_22_meshes1.inc"
};
static Sint16 ev0030_plane_22_meshes2[11] = {
#include "ev0030/plane/ev0030_plane_22_meshes2.inc"
};
static NJS_COLOR ev0030_plane_22_uvs0[16] = {
#include "ev0030/plane/ev0030_plane_22_uvs0.inc"
};
static NJS_COLOR ev0030_plane_22_uvs1[14] = {
#include "ev0030/plane/ev0030_plane_22_uvs1.inc"
};
static NJS_COLOR ev0030_plane_22_uvs2[10] = {
#include "ev0030/plane/ev0030_plane_22_uvs2.inc"
};
static NJS_MESHSET ev0030_plane_22_meshsets[3] = {
#include "ev0030/plane/ev0030_plane_22_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_22_points[16] = {
#include "ev0030/plane/ev0030_plane_22_points.inc"
};
static NJS_VECTOR ev0030_plane_22_normals[16] = {
#include "ev0030/plane/ev0030_plane_22_normals.inc"
};
static NJS_MODEL ev0030_plane_22_model = {
#include "ev0030/plane/ev0030_plane_22_model.inc"
};
static NJS_OBJECT ev0030_plane_22 = {
#include "ev0030/plane/ev0030_plane_22.inc"
};
static NJS_MATERIAL ev0030_plane_21_materials[2] = {
#include "ev0030/plane/ev0030_plane_21_materials.inc"
};
static Sint16 ev0030_plane_21_meshes0[19] = {
#include "ev0030/plane/ev0030_plane_21_meshes0.inc"
};
static Sint16 ev0030_plane_21_meshes1[5] = {
#include "ev0030/plane/ev0030_plane_21_meshes1.inc"
};
static NJS_COLOR ev0030_plane_21_uvs0[16] = {
#include "ev0030/plane/ev0030_plane_21_uvs0.inc"
};
static NJS_COLOR ev0030_plane_21_uvs1[4] = {
#include "ev0030/plane/ev0030_plane_21_uvs1.inc"
};
static NJS_MESHSET ev0030_plane_21_meshsets[2] = {
#include "ev0030/plane/ev0030_plane_21_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_21_points[9] = {
#include "ev0030/plane/ev0030_plane_21_points.inc"
};
static NJS_VECTOR ev0030_plane_21_normals[9] = {
#include "ev0030/plane/ev0030_plane_21_normals.inc"
};
static NJS_MODEL ev0030_plane_21_model = {
#include "ev0030/plane/ev0030_plane_21_model.inc"
};
static NJS_OBJECT ev0030_plane_21 = {
#include "ev0030/plane/ev0030_plane_21.inc"
};
static NJS_MATERIAL ev0030_plane_20_materials[3] = {
#include "ev0030/plane/ev0030_plane_20_materials.inc"
};
static Sint16 ev0030_plane_20_meshes0[28] = {
#include "ev0030/plane/ev0030_plane_20_meshes0.inc"
};
static Sint16 ev0030_plane_20_meshes1[6] = {
#include "ev0030/plane/ev0030_plane_20_meshes1.inc"
};
static Sint16 ev0030_plane_20_meshes2[11] = {
#include "ev0030/plane/ev0030_plane_20_meshes2.inc"
};
static NJS_COLOR ev0030_plane_20_uvs0[23] = {
#include "ev0030/plane/ev0030_plane_20_uvs0.inc"
};
static NJS_COLOR ev0030_plane_20_uvs1[5] = {
#include "ev0030/plane/ev0030_plane_20_uvs1.inc"
};
static NJS_COLOR ev0030_plane_20_uvs2[10] = {
#include "ev0030/plane/ev0030_plane_20_uvs2.inc"
};
static NJS_MESHSET ev0030_plane_20_meshsets[3] = {
#include "ev0030/plane/ev0030_plane_20_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_20_points[18] = {
#include "ev0030/plane/ev0030_plane_20_points.inc"
};
static NJS_VECTOR ev0030_plane_20_normals[18] = {
#include "ev0030/plane/ev0030_plane_20_normals.inc"
};
static NJS_MODEL ev0030_plane_20_model = {
#include "ev0030/plane/ev0030_plane_20_model.inc"
};
static NJS_OBJECT ev0030_plane_20 = {
#include "ev0030/plane/ev0030_plane_20.inc"
};
static NJS_OBJECT ev0030_plane_19 = {
#include "ev0030/plane/ev0030_plane_19.inc"
};
static NJS_OBJECT ev0030_plane_18 = {
#include "ev0030/plane/ev0030_plane_18.inc"
};
static NJS_OBJECT ev0030_plane_17 = {
#include "ev0030/plane/ev0030_plane_17.inc"
};
static NJS_OBJECT ev0030_plane_16 = {
#include "ev0030/plane/ev0030_plane_16.inc"
};
static NJS_OBJECT ev0030_plane_15 = {
#include "ev0030/plane/ev0030_plane_15.inc"
};
static NJS_MATERIAL ev0030_plane_14_materials[1] = {
#include "ev0030/plane/ev0030_plane_14_materials.inc"
};
static Sint16 ev0030_plane_14_meshes0[9] = {
#include "ev0030/plane/ev0030_plane_14_meshes0.inc"
};
static NJS_COLOR ev0030_plane_14_uvs0[8] = {
#include "ev0030/plane/ev0030_plane_14_uvs0.inc"
};
static NJS_MESHSET ev0030_plane_14_meshsets[1] = {
#include "ev0030/plane/ev0030_plane_14_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_14_points[8] = {
#include "ev0030/plane/ev0030_plane_14_points.inc"
};
static NJS_VECTOR ev0030_plane_14_normals[8] = {
#include "ev0030/plane/ev0030_plane_14_normals.inc"
};
static NJS_MODEL ev0030_plane_14_model = {
#include "ev0030/plane/ev0030_plane_14_model.inc"
};
static NJS_OBJECT ev0030_plane_14 = {
#include "ev0030/plane/ev0030_plane_14.inc"
};
static NJS_MATERIAL ev0030_plane_13_materials[1] = {
#include "ev0030/plane/ev0030_plane_13_materials.inc"
};
static Sint16 ev0030_plane_13_meshes0[9] = {
#include "ev0030/plane/ev0030_plane_13_meshes0.inc"
};
static NJS_COLOR ev0030_plane_13_uvs0[8] = {
#include "ev0030/plane/ev0030_plane_13_uvs0.inc"
};
static NJS_MESHSET ev0030_plane_13_meshsets[1] = {
#include "ev0030/plane/ev0030_plane_13_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_13_points[8] = {
#include "ev0030/plane/ev0030_plane_13_points.inc"
};
static NJS_VECTOR ev0030_plane_13_normals[8] = {
#include "ev0030/plane/ev0030_plane_13_normals.inc"
};
static NJS_MODEL ev0030_plane_13_model = {
#include "ev0030/plane/ev0030_plane_13_model.inc"
};
static NJS_OBJECT ev0030_plane_13 = {
#include "ev0030/plane/ev0030_plane_13.inc"
};
static NJS_OBJECT ev0030_plane_12 = {
#include "ev0030/plane/ev0030_plane_12.inc"
};
static NJS_OBJECT ev0030_plane_11 = {
#include "ev0030/plane/ev0030_plane_11.inc"
};
static NJS_OBJECT ev0030_plane_10 = {
#include "ev0030/plane/ev0030_plane_10.inc"
};
static NJS_OBJECT ev0030_plane_9 = {
#include "ev0030/plane/ev0030_plane_9.inc"
};
static NJS_OBJECT ev0030_plane_8 = {
#include "ev0030/plane/ev0030_plane_8.inc"
};
static NJS_OBJECT ev0030_plane_7 = {
#include "ev0030/plane/ev0030_plane_7.inc"
};
static NJS_OBJECT ev0030_plane_6 = {
#include "ev0030/plane/ev0030_plane_6.inc"
};
static NJS_OBJECT ev0030_plane_5 = {
#include "ev0030/plane/ev0030_plane_5.inc"
};
static NJS_MATERIAL ev0030_plane_4_materials[4] = {
#include "ev0030/plane/ev0030_plane_4_materials.inc"
};
static Sint16 ev0030_plane_4_meshes0[40] = {
#include "ev0030/plane/ev0030_plane_4_meshes0.inc"
};
static Sint16 ev0030_plane_4_meshes1[32] = {
#include "ev0030/plane/ev0030_plane_4_meshes1.inc"
};
static Sint16 ev0030_plane_4_meshes2[39] = {
#include "ev0030/plane/ev0030_plane_4_meshes2.inc"
};
static Sint16 ev0030_plane_4_meshes3[12] = {
#include "ev0030/plane/ev0030_plane_4_meshes3.inc"
};
static NJS_COLOR ev0030_plane_4_uvs0[31] = {
#include "ev0030/plane/ev0030_plane_4_uvs0.inc"
};
static NJS_COLOR ev0030_plane_4_uvs1[25] = {
#include "ev0030/plane/ev0030_plane_4_uvs1.inc"
};
static NJS_COLOR ev0030_plane_4_uvs2[34] = {
#include "ev0030/plane/ev0030_plane_4_uvs2.inc"
};
static NJS_COLOR ev0030_plane_4_uvs3[9] = {
#include "ev0030/plane/ev0030_plane_4_uvs3.inc"
};
static NJS_MESHSET ev0030_plane_4_meshsets[4] = {
#include "ev0030/plane/ev0030_plane_4_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_4_points[41] = {
#include "ev0030/plane/ev0030_plane_4_points.inc"
};
static NJS_VECTOR ev0030_plane_4_normals[41] = {
#include "ev0030/plane/ev0030_plane_4_normals.inc"
};
static NJS_MODEL ev0030_plane_4_model = {
#include "ev0030/plane/ev0030_plane_4_model.inc"
};
static NJS_OBJECT ev0030_plane_4 = {
#include "ev0030/plane/ev0030_plane_4.inc"
};
static NJS_MATERIAL ev0030_plane_3_materials[2] = {
#include "ev0030/plane/ev0030_plane_3_materials.inc"
};
static Sint16 ev0030_plane_3_meshes0[93] = {
#include "ev0030/plane/ev0030_plane_3_meshes0.inc"
};
static Sint16 ev0030_plane_3_meshes1[36] = {
#include "ev0030/plane/ev0030_plane_3_meshes1.inc"
};
static NJS_COLOR ev0030_plane_3_uvs0[78] = {
#include "ev0030/plane/ev0030_plane_3_uvs0.inc"
};
static NJS_COLOR ev0030_plane_3_uvs1[32] = {
#include "ev0030/plane/ev0030_plane_3_uvs1.inc"
};
static NJS_MESHSET ev0030_plane_3_meshsets[2] = {
#include "ev0030/plane/ev0030_plane_3_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_3_points[44] = {
#include "ev0030/plane/ev0030_plane_3_points.inc"
};
static NJS_VECTOR ev0030_plane_3_normals[44] = {
#include "ev0030/plane/ev0030_plane_3_normals.inc"
};
static NJS_MODEL ev0030_plane_3_model = {
#include "ev0030/plane/ev0030_plane_3_model.inc"
};
static NJS_OBJECT ev0030_plane_3 = {
#include "ev0030/plane/ev0030_plane_3.inc"
};
static NJS_MATERIAL ev0030_plane_2_materials[18] = {
#include "ev0030/plane/ev0030_plane_2_materials.inc"
};
static Sint16 ev0030_plane_2_meshes0[9] = {
#include "ev0030/plane/ev0030_plane_2_meshes0.inc"
};
static Sint16 ev0030_plane_2_meshes1[201] = {
#include "ev0030/plane/ev0030_plane_2_meshes1.inc"
};
static Sint16 ev0030_plane_2_meshes2[15] = {
#include "ev0030/plane/ev0030_plane_2_meshes2.inc"
};
static Sint16 ev0030_plane_2_meshes3[6] = {
#include "ev0030/plane/ev0030_plane_2_meshes3.inc"
};
static Sint16 ev0030_plane_2_meshes4[83] = {
#include "ev0030/plane/ev0030_plane_2_meshes4.inc"
};
static Sint16 ev0030_plane_2_meshes5[16] = {
#include "ev0030/plane/ev0030_plane_2_meshes5.inc"
};
static Sint16 ev0030_plane_2_meshes6[12] = {
#include "ev0030/plane/ev0030_plane_2_meshes6.inc"
};
static Sint16 ev0030_plane_2_meshes7[15] = {
#include "ev0030/plane/ev0030_plane_2_meshes7.inc"
};
static Sint16 ev0030_plane_2_meshes8[28] = {
#include "ev0030/plane/ev0030_plane_2_meshes8.inc"
};
static Sint16 ev0030_plane_2_meshes9[11] = {
#include "ev0030/plane/ev0030_plane_2_meshes9.inc"
};
static Sint16 ev0030_plane_2_meshes10[32] = {
#include "ev0030/plane/ev0030_plane_2_meshes10.inc"
};
static Sint16 ev0030_plane_2_meshes11[28] = {
#include "ev0030/plane/ev0030_plane_2_meshes11.inc"
};
static Sint16 ev0030_plane_2_meshes12[92] = {
#include "ev0030/plane/ev0030_plane_2_meshes12.inc"
};
static Sint16 ev0030_plane_2_meshes13[30] = {
#include "ev0030/plane/ev0030_plane_2_meshes13.inc"
};
static Sint16 ev0030_plane_2_meshes14[10] = {
#include "ev0030/plane/ev0030_plane_2_meshes14.inc"
};
static Sint16 ev0030_plane_2_meshes15[10] = {
#include "ev0030/plane/ev0030_plane_2_meshes15.inc"
};
static Sint16 ev0030_plane_2_meshes16[5] = {
#include "ev0030/plane/ev0030_plane_2_meshes16.inc"
};
static Sint16 ev0030_plane_2_meshes17[87] = {
#include "ev0030/plane/ev0030_plane_2_meshes17.inc"
};
static NJS_COLOR ev0030_plane_2_uvs0[8] = {
#include "ev0030/plane/ev0030_plane_2_uvs0.inc"
};
static NJS_COLOR ev0030_plane_2_uvs1[178] = {
#include "ev0030/plane/ev0030_plane_2_uvs1.inc"
};
static NJS_COLOR ev0030_plane_2_uvs2[13] = {
#include "ev0030/plane/ev0030_plane_2_uvs2.inc"
};
static NJS_COLOR ev0030_plane_2_uvs3[5] = {
#include "ev0030/plane/ev0030_plane_2_uvs3.inc"
};
static NJS_COLOR ev0030_plane_2_uvs4[68] = {
#include "ev0030/plane/ev0030_plane_2_uvs4.inc"
};
static NJS_COLOR ev0030_plane_2_uvs5[14] = {
#include "ev0030/plane/ev0030_plane_2_uvs5.inc"
};
static NJS_COLOR ev0030_plane_2_uvs6[10] = {
#include "ev0030/plane/ev0030_plane_2_uvs6.inc"
};
static NJS_COLOR ev0030_plane_2_uvs7[12] = {
#include "ev0030/plane/ev0030_plane_2_uvs7.inc"
};
static NJS_COLOR ev0030_plane_2_uvs8[22] = {
#include "ev0030/plane/ev0030_plane_2_uvs8.inc"
};
static NJS_COLOR ev0030_plane_2_uvs9[10] = {
#include "ev0030/plane/ev0030_plane_2_uvs9.inc"
};
static NJS_COLOR ev0030_plane_2_uvs10[25] = {
#include "ev0030/plane/ev0030_plane_2_uvs10.inc"
};
static NJS_COLOR ev0030_plane_2_uvs11[22] = {
#include "ev0030/plane/ev0030_plane_2_uvs11.inc"
};
static NJS_COLOR ev0030_plane_2_uvs12[80] = {
#include "ev0030/plane/ev0030_plane_2_uvs12.inc"
};
static NJS_COLOR ev0030_plane_2_uvs13[25] = {
#include "ev0030/plane/ev0030_plane_2_uvs13.inc"
};
static NJS_COLOR ev0030_plane_2_uvs14[8] = {
#include "ev0030/plane/ev0030_plane_2_uvs14.inc"
};
static NJS_COLOR ev0030_plane_2_uvs15[8] = {
#include "ev0030/plane/ev0030_plane_2_uvs15.inc"
};
static NJS_COLOR ev0030_plane_2_uvs16[4] = {
#include "ev0030/plane/ev0030_plane_2_uvs16.inc"
};
static NJS_COLOR ev0030_plane_2_uvs17[74] = {
#include "ev0030/plane/ev0030_plane_2_uvs17.inc"
};
static NJS_MESHSET ev0030_plane_2_meshsets[18] = {
#include "ev0030/plane/ev0030_plane_2_meshsets.inc"
};
static NJS_POINT3 ev0030_plane_2_points[211] = {
#include "ev0030/plane/ev0030_plane_2_points.inc"
};
static NJS_VECTOR ev0030_plane_2_normals[211] = {
#include "ev0030/plane/ev0030_plane_2_normals.inc"
};
static NJS_MODEL ev0030_plane_2_model = {
#include "ev0030/plane/ev0030_plane_2_model.inc"
};
static NJS_OBJECT ev0030_plane_2 = {
#include "ev0030/plane/ev0030_plane_2.inc"
};
static NJS_OBJECT ev0030_plane_1 = {
#include "ev0030/plane/ev0030_plane_1.inc"
};
static NJS_OBJECT ev0030_plane_object = {
#include "ev0030/plane/ev0030_plane_object.inc"
};
static NJS_MATERIAL ev0030_head_materials[8] = {
#include "ev0030/plane/ev0030_head_materials.inc"
};
static Sint16 ev0030_head_meshes0[68] = {
#include "ev0030/plane/ev0030_head_meshes0.inc"
};
static Sint16 ev0030_head_meshes1[40] = {
#include "ev0030/plane/ev0030_head_meshes1.inc"
};
static Sint16 ev0030_head_meshes2[18] = {
#include "ev0030/plane/ev0030_head_meshes2.inc"
};
static Sint16 ev0030_head_meshes3[465] = {
#include "ev0030/plane/ev0030_head_meshes3.inc"
};
static Sint16 ev0030_head_meshes4[119] = {
#include "ev0030/plane/ev0030_head_meshes4.inc"
};
static Sint16 ev0030_head_meshes5[40] = {
#include "ev0030/plane/ev0030_head_meshes5.inc"
};
static Sint16 ev0030_head_meshes6[168] = {
#include "ev0030/plane/ev0030_head_meshes6.inc"
};
static Sint16 ev0030_head_meshes7[12] = {
#include "ev0030/plane/ev0030_head_meshes7.inc"
};
static NJS_COLOR ev0030_head_uvs0[60] = {
#include "ev0030/plane/ev0030_head_uvs0.inc"
};
static NJS_COLOR ev0030_head_uvs1[36] = {
#include "ev0030/plane/ev0030_head_uvs1.inc"
};
static NJS_COLOR ev0030_head_uvs2[16] = {
#include "ev0030/plane/ev0030_head_uvs2.inc"
};
static NJS_COLOR ev0030_head_uvs3[407] = {
#include "ev0030/plane/ev0030_head_uvs3.inc"
};
static NJS_COLOR ev0030_head_uvs4[110] = {
#include "ev0030/plane/ev0030_head_uvs4.inc"
};
static NJS_COLOR ev0030_head_uvs5[32] = {
#include "ev0030/plane/ev0030_head_uvs5.inc"
};
static NJS_COLOR ev0030_head_uvs6[144] = {
#include "ev0030/plane/ev0030_head_uvs6.inc"
};
static NJS_COLOR ev0030_head_uvs7[10] = {
#include "ev0030/plane/ev0030_head_uvs7.inc"
};
static NJS_MESHSET ev0030_head_meshsets[8] = {
#include "ev0030/plane/ev0030_head_meshsets.inc"
};
static NJS_POINT3 ev0030_head_points[366] = {
#include "ev0030/plane/ev0030_head_points.inc"
};
static NJS_VECTOR ev0030_head_normals[366] = {
#include "ev0030/plane/ev0030_head_normals.inc"
};
static NJS_MODEL ev0030_head_model = {
#include "ev0030/plane/ev0030_head_model.inc"
};
static NJS_OBJECT ev0030_head_object = {
#include "ev0030/plane/ev0030_head_object.inc"
};
static NJS_MKEY_F ev0030_plane_motion_0_pos1[44] = {
#include "ev0030/plane/ev0030_plane_motion_0_pos1.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang4[44] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang4.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang6[44] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang6.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang10[44] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang10.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang15[44] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang15.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang16[43] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang16.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang17[41] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang17.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang18[31] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang18.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang26[44] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang26.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang27[43] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang27.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang28[41] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang28.inc"
};
static NJS_MKEY_A ev0030_plane_motion_0_ang29[29] = {
#include "ev0030/plane/ev0030_plane_motion_0_ang29.inc"
};
static NJS_MKEY_F ev0030_plane_motion_0_pos37[44] = {
#include "ev0030/plane/ev0030_plane_motion_0_pos37.inc"
};
static NJS_MKEY_F ev0030_plane_motion_0_pos38[43] = {
#include "ev0030/plane/ev0030_plane_motion_0_pos38.inc"
};
static NJS_MKEY_F ev0030_plane_motion_0_pos39[21] = {
#include "ev0030/plane/ev0030_plane_motion_0_pos39.inc"
};
static NJS_MKEY_F ev0030_plane_motion_0_pos40[44] = {
#include "ev0030/plane/ev0030_plane_motion_0_pos40.inc"
};
static NJS_MKEY_F ev0030_plane_motion_0_pos41[43] = {
#include "ev0030/plane/ev0030_plane_motion_0_pos41.inc"
};
static NJS_MKEY_F ev0030_plane_motion_0_pos42[25] = {
#include "ev0030/plane/ev0030_plane_motion_0_pos42.inc"
};
static NJS_MDATA2 ev0030_plane_motion_0_mdata[45] = {
#include "ev0030/plane/ev0030_plane_motion_0_mdata.inc"
};
static NJS_MOTION ev0030_plane_motion_0 = {
#include "ev0030/plane/ev0030_plane_motion_0.inc"
};
static NJS_ACTION ev0030_plane_action_0 = {
#include "ev0030/plane/ev0030_plane_action_0.inc"
};
static NJS_MKEY_F ev0030_plane_motion_1_pos1[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_pos1.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang4[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang4.inc"
};
static NJS_MKEY_F ev0030_plane_motion_1_pos5[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_pos5.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang6[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang6.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang10[32] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang10.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang15[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang15.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang16[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang16.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang17[38] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang17.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang18[34] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang18.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang26[43] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang26.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang27[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang27.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang28[42] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang28.inc"
};
static NJS_MKEY_A ev0030_plane_motion_1_ang29[24] = {
#include "ev0030/plane/ev0030_plane_motion_1_ang29.inc"
};
static NJS_MKEY_F ev0030_plane_motion_1_pos37[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_pos37.inc"
};
static NJS_MKEY_F ev0030_plane_motion_1_pos38[40] = {
#include "ev0030/plane/ev0030_plane_motion_1_pos38.inc"
};
static NJS_MKEY_F ev0030_plane_motion_1_pos39[2] = {
#include "ev0030/plane/ev0030_plane_motion_1_pos39.inc"
};
static NJS_MKEY_F ev0030_plane_motion_1_pos40[44] = {
#include "ev0030/plane/ev0030_plane_motion_1_pos40.inc"
};
static NJS_MKEY_F ev0030_plane_motion_1_pos41[40] = {
#include "ev0030/plane/ev0030_plane_motion_1_pos41.inc"
};
static NJS_MKEY_F ev0030_plane_motion_1_pos42[2] = {
#include "ev0030/plane/ev0030_plane_motion_1_pos42.inc"
};
static NJS_MDATA2 ev0030_plane_motion_1_mdata[45] = {
#include "ev0030/plane/ev0030_plane_motion_1_mdata.inc"
};
static NJS_MOTION ev0030_plane_motion_1 = {
#include "ev0030/plane/ev0030_plane_motion_1.inc"
};
static NJS_ACTION ev0030_plane_action_1 = {
#include "ev0030/plane/ev0030_plane_action_1.inc"
};
static NJS_MKEY_F ev0030_plane_motion_2_pos1[44] = {
#include "ev0030/plane/ev0030_plane_motion_2_pos1.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang4[44] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang4.inc"
};
static NJS_MKEY_F ev0030_plane_motion_2_pos5[43] = {
#include "ev0030/plane/ev0030_plane_motion_2_pos5.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang6[43] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang6.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang10[43] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang10.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang16[43] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang16.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang17[39] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang17.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang18[35] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang18.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang27[43] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang27.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang28[43] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang28.inc"
};
static NJS_MKEY_A ev0030_plane_motion_2_ang29[29] = {
#include "ev0030/plane/ev0030_plane_motion_2_ang29.inc"
};
static NJS_MKEY_F ev0030_plane_motion_2_pos37[44] = {
#include "ev0030/plane/ev0030_plane_motion_2_pos37.inc"
};
static NJS_MKEY_F ev0030_plane_motion_2_pos38[31] = {
#include "ev0030/plane/ev0030_plane_motion_2_pos38.inc"
};
static NJS_MKEY_F ev0030_plane_motion_2_pos39[2] = {
#include "ev0030/plane/ev0030_plane_motion_2_pos39.inc"
};
static NJS_MKEY_F ev0030_plane_motion_2_pos40[43] = {
#include "ev0030/plane/ev0030_plane_motion_2_pos40.inc"
};
static NJS_MKEY_F ev0030_plane_motion_2_pos41[31] = {
#include "ev0030/plane/ev0030_plane_motion_2_pos41.inc"
};
static NJS_MKEY_F ev0030_plane_motion_2_pos42[2] = {
#include "ev0030/plane/ev0030_plane_motion_2_pos42.inc"
};
static NJS_MDATA2 ev0030_plane_motion_2_mdata[45] = {
#include "ev0030/plane/ev0030_plane_motion_2_mdata.inc"
};
static NJS_MOTION ev0030_plane_motion_2 = {
#include "ev0030/plane/ev0030_plane_motion_2.inc"
};
static NJS_ACTION ev0030_plane_action_2 = {
#include "ev0030/plane/ev0030_plane_action_2.inc"
};
static NJS_MKEY_F ev0030_plane_motion_3_pos1[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_pos1.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang4[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang4.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang6[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang6.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang10[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang10.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang15[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang15.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang16[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang16.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang17[41] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang17.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang18[23] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang18.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang26[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang26.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang27[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang27.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang28[41] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang28.inc"
};
static NJS_MKEY_A ev0030_plane_motion_3_ang29[29] = {
#include "ev0030/plane/ev0030_plane_motion_3_ang29.inc"
};
static NJS_MKEY_F ev0030_plane_motion_3_pos37[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_pos37.inc"
};
static NJS_MKEY_F ev0030_plane_motion_3_pos38[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_pos38.inc"
};
static NJS_MKEY_F ev0030_plane_motion_3_pos39[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_pos39.inc"
};
static NJS_MKEY_F ev0030_plane_motion_3_pos40[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_pos40.inc"
};
static NJS_MKEY_F ev0030_plane_motion_3_pos41[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_pos41.inc"
};
static NJS_MKEY_F ev0030_plane_motion_3_pos42[44] = {
#include "ev0030/plane/ev0030_plane_motion_3_pos42.inc"
};
static NJS_MDATA2 ev0030_plane_motion_3_mdata[45] = {
#include "ev0030/plane/ev0030_plane_motion_3_mdata.inc"
};
static NJS_MOTION ev0030_plane_motion_3 = {
#include "ev0030/plane/ev0030_plane_motion_3.inc"
};
static NJS_ACTION ev0030_plane_action_3 = {
#include "ev0030/plane/ev0030_plane_action_3.inc"
};
static NJS_MDATA2 ev0030_head_motion_0_mdata[1] = {
#include "ev0030/plane/ev0030_head_motion_0_mdata.inc"
};
static NJS_MOTION ev0030_head_motion_0 = {
#include "ev0030/plane/ev0030_head_motion_0.inc"
};
static NJS_ACTION ev0030_head_action_0 = {
#include "ev0030/plane/ev0030_head_action_0.inc"
};
static NJS_MDATA2 ev0030_head_motion_1_mdata[1] = {
#include "ev0030/plane/ev0030_head_motion_1_mdata.inc"
};
static NJS_MOTION ev0030_head_motion_1 = {
#include "ev0030/plane/ev0030_head_motion_1.inc"
};
static NJS_ACTION ev0030_head_action_1 = {
#include "ev0030/plane/ev0030_head_action_1.inc"
};
static NJS_MDATA2 ev0030_head_motion_2_mdata[1] = {
#include "ev0030/plane/ev0030_head_motion_2_mdata.inc"
};
static NJS_MOTION ev0030_head_motion_2 = {
#include "ev0030/plane/ev0030_head_motion_2.inc"
};
static NJS_ACTION ev0030_head_action_2 = {
#include "ev0030/plane/ev0030_head_action_2.inc"
};
static NJS_POINT3 ev0030_head_shape_0_points0[366] = {
#include "ev0030/plane/ev0030_head_shape_0_points0.inc"
};
static NJS_POINT3 ev0030_head_shape_0_normals0[366] = {
#include "ev0030/plane/ev0030_head_shape_0_normals0.inc"
};
static NJS_POINT3 ev0030_head_shape_0_points1[366] = {
#include "ev0030/plane/ev0030_head_shape_0_points1.inc"
};
static NJS_POINT3 ev0030_head_shape_0_normals1[366] = {
#include "ev0030/plane/ev0030_head_shape_0_normals1.inc"
};
static NJS_POINT3 ev0030_head_shape_0_points2[366] = {
#include "ev0030/plane/ev0030_head_shape_0_points2.inc"
};
static NJS_POINT3 ev0030_head_shape_0_normals2[366] = {
#include "ev0030/plane/ev0030_head_shape_0_normals2.inc"
};
static NJS_MKEY_P ev0030_head_shape_0_points[3] = {
#include "ev0030/plane/ev0030_head_shape_0_points.inc"
};
static NJS_MKEY_P ev0030_head_shape_0_normals[3] = {
#include "ev0030/plane/ev0030_head_shape_0_normals.inc"
};
static NJS_MDATA2 ev0030_head_shape_0_mdata[1] = {
#include "ev0030/plane/ev0030_head_shape_0_mdata.inc"
};
static NJS_MOTION ev0030_head_shape_0 = {
#include "ev0030/plane/ev0030_head_shape_0.inc"
};
static NJS_POINT3 ev0030_head_shape_1_points0[366] = {
#include "ev0030/plane/ev0030_head_shape_1_points0.inc"
};
static NJS_POINT3 ev0030_head_shape_1_normals0[366] = {
#include "ev0030/plane/ev0030_head_shape_1_normals0.inc"
};
static NJS_POINT3 ev0030_head_shape_1_points1[366] = {
#include "ev0030/plane/ev0030_head_shape_1_points1.inc"
};
static NJS_POINT3 ev0030_head_shape_1_normals1[366] = {
#include "ev0030/plane/ev0030_head_shape_1_normals1.inc"
};
static NJS_POINT3 ev0030_head_shape_1_points2[366] = {
#include "ev0030/plane/ev0030_head_shape_1_points2.inc"
};
static NJS_POINT3 ev0030_head_shape_1_normals2[366] = {
#include "ev0030/plane/ev0030_head_shape_1_normals2.inc"
};
static NJS_POINT3 ev0030_head_shape_1_points3[366] = {
#include "ev0030/plane/ev0030_head_shape_1_points3.inc"
};
static NJS_POINT3 ev0030_head_shape_1_normals3[366] = {
#include "ev0030/plane/ev0030_head_shape_1_normals3.inc"
};
static NJS_MKEY_P ev0030_head_shape_1_points[4] = {
#include "ev0030/plane/ev0030_head_shape_1_points.inc"
};
static NJS_MKEY_P ev0030_head_shape_1_normals[4] = {
#include "ev0030/plane/ev0030_head_shape_1_normals.inc"
};
static NJS_MDATA2 ev0030_head_shape_1_mdata[1] = {
#include "ev0030/plane/ev0030_head_shape_1_mdata.inc"
};
static NJS_MOTION ev0030_head_shape_1 = {
#include "ev0030/plane/ev0030_head_shape_1.inc"
};
static NJS_POINT3 ev0030_head_shape_2_points0[366] = {
#include "ev0030/plane/ev0030_head_shape_2_points0.inc"
};
static NJS_POINT3 ev0030_head_shape_2_normals0[366] = {
#include "ev0030/plane/ev0030_head_shape_2_normals0.inc"
};
static NJS_POINT3 ev0030_head_shape_2_points1[366] = {
#include "ev0030/plane/ev0030_head_shape_2_points1.inc"
};
static NJS_POINT3 ev0030_head_shape_2_normals1[366] = {
#include "ev0030/plane/ev0030_head_shape_2_normals1.inc"
};
static NJS_POINT3 ev0030_head_shape_2_points2[366] = {
#include "ev0030/plane/ev0030_head_shape_2_points2.inc"
};
static NJS_POINT3 ev0030_head_shape_2_normals2[366] = {
#include "ev0030/plane/ev0030_head_shape_2_normals2.inc"
};
static NJS_POINT3 ev0030_head_shape_2_points3[366] = {
#include "ev0030/plane/ev0030_head_shape_2_points3.inc"
};
static NJS_POINT3 ev0030_head_shape_2_normals3[366] = {
#include "ev0030/plane/ev0030_head_shape_2_normals3.inc"
};
static NJS_POINT3 ev0030_head_shape_2_points4[366] = {
#include "ev0030/plane/ev0030_head_shape_2_points4.inc"
};
static NJS_POINT3 ev0030_head_shape_2_normals4[366] = {
#include "ev0030/plane/ev0030_head_shape_2_normals4.inc"
};
static NJS_MKEY_P ev0030_head_shape_2_points[5] = {
#include "ev0030/plane/ev0030_head_shape_2_points.inc"
};
static NJS_MKEY_P ev0030_head_shape_2_normals[5] = {
#include "ev0030/plane/ev0030_head_shape_2_normals.inc"
};
static NJS_MDATA2 ev0030_head_shape_2_mdata[1] = {
#include "ev0030/plane/ev0030_head_shape_2_mdata.inc"
};
static NJS_MOTION ev0030_head_shape_2 = {
#include "ev0030/plane/ev0030_head_shape_2.inc"
};

/* indexed by anim. 0 plays as the plane flies. ev0030_script starts 1 as the
 * camera shakes, and 3 as the plane stalls */
static struct plane_anim ev0030_plane_anims[4] = {
    {&ev0030_plane_action_0, &ev0030_head_motion_1, &ev0030_head_shape_1, 0},
    {&ev0030_plane_action_1, &ev0030_head_motion_2, &ev0030_head_shape_2, 2},
    {&ev0030_plane_action_2, &ev0030_head_motion_2, &ev0030_head_shape_2, 2},
    {&ev0030_plane_action_3, &ev0030_head_motion_0, &ev0030_head_shape_0, 3},
};

static struct task *ev0030_plane_task = NULL;

static void ev0030_plane_destroy(struct task *task)
{
}

static void ev0030_plane_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MATRIX head;
    Angle ang;

    draw_set_palette(DRAW_PLAYER_PALETTE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&ev0030_plane_texlist);
    draw_action(ev0030_plane_anims[VARS(work)->anim].action, VARS(work)->frame);
    pose_action(&_unit_matrix_, ev0030_plane_anims[VARS(work)->anim].action,
                VARS(work)->frame, PLANE_HEAD, &head);
    njMultiMatrix(NULL, &head);
    njSetTexture(&ev0030_head_texlist);
    draw_shape_motion(
        &ev0030_head_object, ev0030_plane_anims[VARS(work)->anim].head_motion,
        ev0030_plane_anims[VARS(work)->anim].head_shape, VARS(work)->frame);
    njPopMatrix(1);
    draw_set_palette(0);
}

static void ev0030_plane_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame += VARS(work)->speed;
    if (VARS(work)->frame >=
        (Float)(ev0030_plane_anims[VARS(work)->anim].action->motion->nbFrame -
                1)) {
        VARS(work)->anim = ev0030_plane_anims[VARS(work)->anim].next;
        VARS(work)->frame = 0.0F;
    }
    if (work->event->paths != NULL) {
        event_paths_step(work);
    }
    ev0030_plane_draw(task);
}

static void ev0030_plane_init(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->speed = 0.5F;
    VARS(work)->frame = 0.0F;
    VARS(work)->anim = 0;
    work->scl.x = 0.0F;
    work->scl.y = 0.0F;
    work->scl.z = 0.0F;
    event_set_flags(task, EVENT_IGNORE_GROUND);
    task->step = ev0030_plane_step;
    task->draw = ev0030_plane_draw;
    task->destructor = ev0030_plane_destroy;
}

struct task *ev0030_plane_create(void)
{
    struct task *task;

    task = task_create(TASK_WORK, 5, ev0030_plane_init);
    if (task != NULL) {
        ev0030_plane_task = task;
        event_alloc(task->work);
    }
    return task;
}

void ev0030_plane_play(int anim, Float speed)
{
    if (ev0030_plane_task != NULL) {
        VARS(ev0030_plane_task->work)->anim = anim;
        VARS(ev0030_plane_task->work)->frame = 0.0F;
        VARS(ev0030_plane_task->work)->speed = speed;
    }
}
