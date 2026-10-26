/* an event's Zero, Amy and the bird, drawn together by one task where it
 * stands */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "event/event.h"
#include "shared/zero.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 13
#define TEXT_19_LENGTH 13
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 14
#define TEXT_25_LENGTH 14
#define TEXT_26_LENGTH 12
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 7
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 10
#define TEXT_32_LENGTH 7
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 9
#define TEXT_35_LENGTH 9
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 9
#define TEXT_38_LENGTH 7
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 7
#define TEXT_41_LENGTH 9
#define TEXT_42_LENGTH 11
#define TEXT_43_LENGTH 11
#define TEXT_44_LENGTH 11
#define TEXT_45_LENGTH 11
#define TEXT_46_LENGTH 11
#define TEXT_47_LENGTH 9
#define TEXT_48_LENGTH 10
#define TEXT_49_LENGTH 9
#define TEXT_50_LENGTH 10
#define TEXT_51_LENGTH 11
#define TEXT_52_LENGTH 10
#define TEXT_53_LENGTH 9
#define TEXT_54_LENGTH 9
#define TEXT_55_LENGTH 11
#define TEXT_56_LENGTH 11
#define TEXT_57_LENGTH 11
#define TEXT_58_LENGTH 8
#define TEXT_59_LENGTH 12
#include "shared/zero/text.inc"

/* the node of Amy's action, counted from 0, whose matrix
 * event_zero_amy_head_object draws in */
#define AMY_HEAD_NODE 12

/* an animation: three actions, and the motion and shape motion of Amy's
 * head. all five share the task's frame */
struct event_zero_anim {
    NJS_ACTION *zero;
    NJS_ACTION *amy;
    NJS_ACTION *bird;
    NJS_MOTION *head_motion;
    NJS_MOTION *head_shape;
    Uint8 unk_14[4];
};

struct zero_vars {
    Float frame;
    Sint32 anim; /* which of event_zero_anims plays */
    Float speed; /* frames the animation advances a step */
};

#define VARS(work) ((struct zero_vars *)&(work)->vars)

static NJS_TEXNAME event_zero_texnames[59] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0}, {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
    {TEXT_57, 0, 0}, {TEXT_58, 0, 0}, {TEXT_59, 0, 0},
};
NJS_TEXLIST event_zero_texlist = {
#include "shared/zero/event_zero_texlist.inc"
};
static NJS_MATERIAL event_zero_amy_76_materials[8] = {
#include "shared/zero/event_zero_amy_76_materials.inc"
};
static Sint16 event_zero_amy_76_meshes0[24] = {
#include "shared/zero/event_zero_amy_76_meshes0.inc"
};
static Sint16 event_zero_amy_76_meshes1[72] = {
#include "shared/zero/event_zero_amy_76_meshes1.inc"
};
static Sint16 event_zero_amy_76_meshes2[22] = {
#include "shared/zero/event_zero_amy_76_meshes2.inc"
};
static Sint16 event_zero_amy_76_meshes3[22] = {
#include "shared/zero/event_zero_amy_76_meshes3.inc"
};
static Sint16 event_zero_amy_76_meshes4[10] = {
#include "shared/zero/event_zero_amy_76_meshes4.inc"
};
static Sint16 event_zero_amy_76_meshes5[18] = {
#include "shared/zero/event_zero_amy_76_meshes5.inc"
};
static Sint16 event_zero_amy_76_meshes6[8] = {
#include "shared/zero/event_zero_amy_76_meshes6.inc"
};
static Sint16 event_zero_amy_76_meshes7[72] = {
#include "shared/zero/event_zero_amy_76_meshes7.inc"
};
static NJS_COLOR event_zero_amy_76_uvs0[20] = {
#include "shared/zero/event_zero_amy_76_uvs0.inc"
};
static NJS_COLOR event_zero_amy_76_uvs1[54] = {
#include "shared/zero/event_zero_amy_76_uvs1.inc"
};
static NJS_COLOR event_zero_amy_76_uvs2[18] = {
#include "shared/zero/event_zero_amy_76_uvs2.inc"
};
static NJS_COLOR event_zero_amy_76_uvs3[18] = {
#include "shared/zero/event_zero_amy_76_uvs3.inc"
};
static NJS_COLOR event_zero_amy_76_uvs4[8] = {
#include "shared/zero/event_zero_amy_76_uvs4.inc"
};
static NJS_COLOR event_zero_amy_76_uvs5[15] = {
#include "shared/zero/event_zero_amy_76_uvs5.inc"
};
static NJS_COLOR event_zero_amy_76_uvs6[7] = {
#include "shared/zero/event_zero_amy_76_uvs6.inc"
};
static NJS_COLOR event_zero_amy_76_uvs7[54] = {
#include "shared/zero/event_zero_amy_76_uvs7.inc"
};
static NJS_MESHSET event_zero_amy_76_meshsets[8] = {
#include "shared/zero/event_zero_amy_76_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_76_points[45] = {
#include "shared/zero/event_zero_amy_76_points.inc"
};
static NJS_VECTOR event_zero_amy_76_normals[45] = {
#include "shared/zero/event_zero_amy_76_normals.inc"
};
static NJS_MODEL event_zero_amy_76_model = {
#include "shared/zero/event_zero_amy_76_model.inc"
};
static NJS_OBJECT event_zero_amy_76 = {
#include "shared/zero/event_zero_amy_76.inc"
};
static NJS_OBJECT event_zero_amy_75 = {
#include "shared/zero/event_zero_amy_75.inc"
};
static NJS_OBJECT event_zero_amy_74 = {
#include "shared/zero/event_zero_amy_74.inc"
};
static NJS_OBJECT event_zero_amy_73 = {
#include "shared/zero/event_zero_amy_73.inc"
};
static NJS_MATERIAL event_zero_amy_72_materials[1] = {
#include "shared/zero/event_zero_amy_72_materials.inc"
};
static Sint16 event_zero_amy_72_meshes0[15] = {
#include "shared/zero/event_zero_amy_72_meshes0.inc"
};
static NJS_COLOR event_zero_amy_72_uvs0[13] = {
#include "shared/zero/event_zero_amy_72_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_72_meshsets[1] = {
#include "shared/zero/event_zero_amy_72_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_72_points[7] = {
#include "shared/zero/event_zero_amy_72_points.inc"
};
static NJS_VECTOR event_zero_amy_72_normals[7] = {
#include "shared/zero/event_zero_amy_72_normals.inc"
};
static NJS_MODEL event_zero_amy_72_model = {
#include "shared/zero/event_zero_amy_72_model.inc"
};
static NJS_OBJECT event_zero_amy_72 = {
#include "shared/zero/event_zero_amy_72.inc"
};
static NJS_OBJECT event_zero_amy_71 = {
#include "shared/zero/event_zero_amy_71.inc"
};
static NJS_OBJECT event_zero_amy_70 = {
#include "shared/zero/event_zero_amy_70.inc"
};
static NJS_OBJECT event_zero_amy_69 = {
#include "shared/zero/event_zero_amy_69.inc"
};
static NJS_MATERIAL event_zero_amy_68_materials[1] = {
#include "shared/zero/event_zero_amy_68_materials.inc"
};
static Sint16 event_zero_amy_68_meshes0[11] = {
#include "shared/zero/event_zero_amy_68_meshes0.inc"
};
static NJS_COLOR event_zero_amy_68_uvs0[10] = {
#include "shared/zero/event_zero_amy_68_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_68_meshsets[1] = {
#include "shared/zero/event_zero_amy_68_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_68_points[8] = {
#include "shared/zero/event_zero_amy_68_points.inc"
};
static NJS_VECTOR event_zero_amy_68_normals[8] = {
#include "shared/zero/event_zero_amy_68_normals.inc"
};
static NJS_MODEL event_zero_amy_68_model = {
#include "shared/zero/event_zero_amy_68_model.inc"
};
static NJS_OBJECT event_zero_amy_68 = {
#include "shared/zero/event_zero_amy_68.inc"
};
static NJS_MATERIAL event_zero_amy_67_materials[1] = {
#include "shared/zero/event_zero_amy_67_materials.inc"
};
static Sint16 event_zero_amy_67_meshes0[11] = {
#include "shared/zero/event_zero_amy_67_meshes0.inc"
};
static NJS_COLOR event_zero_amy_67_uvs0[10] = {
#include "shared/zero/event_zero_amy_67_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_67_meshsets[1] = {
#include "shared/zero/event_zero_amy_67_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_67_points[8] = {
#include "shared/zero/event_zero_amy_67_points.inc"
};
static NJS_VECTOR event_zero_amy_67_normals[8] = {
#include "shared/zero/event_zero_amy_67_normals.inc"
};
static NJS_MODEL event_zero_amy_67_model = {
#include "shared/zero/event_zero_amy_67_model.inc"
};
static NJS_OBJECT event_zero_amy_67 = {
#include "shared/zero/event_zero_amy_67.inc"
};
static NJS_MATERIAL event_zero_amy_66_materials[3] = {
#include "shared/zero/event_zero_amy_66_materials.inc"
};
static Sint16 event_zero_amy_66_meshes0[10] = {
#include "shared/zero/event_zero_amy_66_meshes0.inc"
};
static Sint16 event_zero_amy_66_meshes1[7] = {
#include "shared/zero/event_zero_amy_66_meshes1.inc"
};
static Sint16 event_zero_amy_66_meshes2[5] = {
#include "shared/zero/event_zero_amy_66_meshes2.inc"
};
static NJS_COLOR event_zero_amy_66_uvs0[8] = {
#include "shared/zero/event_zero_amy_66_uvs0.inc"
};
static NJS_COLOR event_zero_amy_66_uvs1[6] = {
#include "shared/zero/event_zero_amy_66_uvs1.inc"
};
static NJS_COLOR event_zero_amy_66_uvs2[4] = {
#include "shared/zero/event_zero_amy_66_uvs2.inc"
};
static NJS_MESHSET event_zero_amy_66_meshsets[3] = {
#include "shared/zero/event_zero_amy_66_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_66_points[8] = {
#include "shared/zero/event_zero_amy_66_points.inc"
};
static NJS_VECTOR event_zero_amy_66_normals[8] = {
#include "shared/zero/event_zero_amy_66_normals.inc"
};
static NJS_MODEL event_zero_amy_66_model = {
#include "shared/zero/event_zero_amy_66_model.inc"
};
static NJS_OBJECT event_zero_amy_66 = {
#include "shared/zero/event_zero_amy_66.inc"
};
static NJS_MATERIAL event_zero_amy_65_materials[4] = {
#include "shared/zero/event_zero_amy_65_materials.inc"
};
static Sint16 event_zero_amy_65_meshes0[18] = {
#include "shared/zero/event_zero_amy_65_meshes0.inc"
};
static Sint16 event_zero_amy_65_meshes1[10] = {
#include "shared/zero/event_zero_amy_65_meshes1.inc"
};
static Sint16 event_zero_amy_65_meshes2[10] = {
#include "shared/zero/event_zero_amy_65_meshes2.inc"
};
static Sint16 event_zero_amy_65_meshes3[5] = {
#include "shared/zero/event_zero_amy_65_meshes3.inc"
};
static NJS_COLOR event_zero_amy_65_uvs0[15] = {
#include "shared/zero/event_zero_amy_65_uvs0.inc"
};
static NJS_COLOR event_zero_amy_65_uvs1[8] = {
#include "shared/zero/event_zero_amy_65_uvs1.inc"
};
static NJS_COLOR event_zero_amy_65_uvs2[9] = {
#include "shared/zero/event_zero_amy_65_uvs2.inc"
};
static NJS_COLOR event_zero_amy_65_uvs3[4] = {
#include "shared/zero/event_zero_amy_65_uvs3.inc"
};
static NJS_MESHSET event_zero_amy_65_meshsets[4] = {
#include "shared/zero/event_zero_amy_65_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_65_points[14] = {
#include "shared/zero/event_zero_amy_65_points.inc"
};
static NJS_VECTOR event_zero_amy_65_normals[14] = {
#include "shared/zero/event_zero_amy_65_normals.inc"
};
static NJS_MODEL event_zero_amy_65_model = {
#include "shared/zero/event_zero_amy_65_model.inc"
};
static NJS_OBJECT event_zero_amy_65 = {
#include "shared/zero/event_zero_amy_65.inc"
};
static NJS_MATERIAL event_zero_amy_64_materials[4] = {
#include "shared/zero/event_zero_amy_64_materials.inc"
};
static Sint16 event_zero_amy_64_meshes0[11] = {
#include "shared/zero/event_zero_amy_64_meshes0.inc"
};
static Sint16 event_zero_amy_64_meshes1[15] = {
#include "shared/zero/event_zero_amy_64_meshes1.inc"
};
static Sint16 event_zero_amy_64_meshes2[7] = {
#include "shared/zero/event_zero_amy_64_meshes2.inc"
};
static Sint16 event_zero_amy_64_meshes3[6] = {
#include "shared/zero/event_zero_amy_64_meshes3.inc"
};
static NJS_COLOR event_zero_amy_64_uvs0[10] = {
#include "shared/zero/event_zero_amy_64_uvs0.inc"
};
static NJS_COLOR event_zero_amy_64_uvs1[13] = {
#include "shared/zero/event_zero_amy_64_uvs1.inc"
};
static NJS_COLOR event_zero_amy_64_uvs2[6] = {
#include "shared/zero/event_zero_amy_64_uvs2.inc"
};
static NJS_COLOR event_zero_amy_64_uvs3[5] = {
#include "shared/zero/event_zero_amy_64_uvs3.inc"
};
static NJS_MESHSET event_zero_amy_64_meshsets[4] = {
#include "shared/zero/event_zero_amy_64_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_64_points[18] = {
#include "shared/zero/event_zero_amy_64_points.inc"
};
static NJS_VECTOR event_zero_amy_64_normals[18] = {
#include "shared/zero/event_zero_amy_64_normals.inc"
};
static NJS_MODEL event_zero_amy_64_model = {
#include "shared/zero/event_zero_amy_64_model.inc"
};
static NJS_OBJECT event_zero_amy_64 = {
#include "shared/zero/event_zero_amy_64.inc"
};
static NJS_OBJECT event_zero_amy_63 = {
#include "shared/zero/event_zero_amy_63.inc"
};
static NJS_OBJECT event_zero_amy_62 = {
#include "shared/zero/event_zero_amy_62.inc"
};
static NJS_OBJECT event_zero_amy_61 = {
#include "shared/zero/event_zero_amy_61.inc"
};
static NJS_OBJECT event_zero_amy_60 = {
#include "shared/zero/event_zero_amy_60.inc"
};
static NJS_OBJECT event_zero_amy_59 = {
#include "shared/zero/event_zero_amy_59.inc"
};
static NJS_MATERIAL event_zero_amy_58_materials[1] = {
#include "shared/zero/event_zero_amy_58_materials.inc"
};
static Sint16 event_zero_amy_58_meshes0[11] = {
#include "shared/zero/event_zero_amy_58_meshes0.inc"
};
static NJS_COLOR event_zero_amy_58_uvs0[10] = {
#include "shared/zero/event_zero_amy_58_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_58_meshsets[1] = {
#include "shared/zero/event_zero_amy_58_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_58_points[8] = {
#include "shared/zero/event_zero_amy_58_points.inc"
};
static NJS_VECTOR event_zero_amy_58_normals[8] = {
#include "shared/zero/event_zero_amy_58_normals.inc"
};
static NJS_MODEL event_zero_amy_58_model = {
#include "shared/zero/event_zero_amy_58_model.inc"
};
static NJS_OBJECT event_zero_amy_58 = {
#include "shared/zero/event_zero_amy_58.inc"
};
static NJS_MATERIAL event_zero_amy_57_materials[1] = {
#include "shared/zero/event_zero_amy_57_materials.inc"
};
static Sint16 event_zero_amy_57_meshes0[11] = {
#include "shared/zero/event_zero_amy_57_meshes0.inc"
};
static NJS_COLOR event_zero_amy_57_uvs0[10] = {
#include "shared/zero/event_zero_amy_57_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_57_meshsets[1] = {
#include "shared/zero/event_zero_amy_57_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_57_points[8] = {
#include "shared/zero/event_zero_amy_57_points.inc"
};
static NJS_VECTOR event_zero_amy_57_normals[8] = {
#include "shared/zero/event_zero_amy_57_normals.inc"
};
static NJS_MODEL event_zero_amy_57_model = {
#include "shared/zero/event_zero_amy_57_model.inc"
};
static NJS_OBJECT event_zero_amy_57 = {
#include "shared/zero/event_zero_amy_57.inc"
};
static NJS_MATERIAL event_zero_amy_56_materials[3] = {
#include "shared/zero/event_zero_amy_56_materials.inc"
};
static Sint16 event_zero_amy_56_meshes0[10] = {
#include "shared/zero/event_zero_amy_56_meshes0.inc"
};
static Sint16 event_zero_amy_56_meshes1[7] = {
#include "shared/zero/event_zero_amy_56_meshes1.inc"
};
static Sint16 event_zero_amy_56_meshes2[5] = {
#include "shared/zero/event_zero_amy_56_meshes2.inc"
};
static NJS_COLOR event_zero_amy_56_uvs0[8] = {
#include "shared/zero/event_zero_amy_56_uvs0.inc"
};
static NJS_COLOR event_zero_amy_56_uvs1[6] = {
#include "shared/zero/event_zero_amy_56_uvs1.inc"
};
static NJS_COLOR event_zero_amy_56_uvs2[4] = {
#include "shared/zero/event_zero_amy_56_uvs2.inc"
};
static NJS_MESHSET event_zero_amy_56_meshsets[3] = {
#include "shared/zero/event_zero_amy_56_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_56_points[8] = {
#include "shared/zero/event_zero_amy_56_points.inc"
};
static NJS_VECTOR event_zero_amy_56_normals[8] = {
#include "shared/zero/event_zero_amy_56_normals.inc"
};
static NJS_MODEL event_zero_amy_56_model = {
#include "shared/zero/event_zero_amy_56_model.inc"
};
static NJS_OBJECT event_zero_amy_56 = {
#include "shared/zero/event_zero_amy_56.inc"
};
static NJS_MATERIAL event_zero_amy_55_materials[4] = {
#include "shared/zero/event_zero_amy_55_materials.inc"
};
static Sint16 event_zero_amy_55_meshes0[18] = {
#include "shared/zero/event_zero_amy_55_meshes0.inc"
};
static Sint16 event_zero_amy_55_meshes1[10] = {
#include "shared/zero/event_zero_amy_55_meshes1.inc"
};
static Sint16 event_zero_amy_55_meshes2[10] = {
#include "shared/zero/event_zero_amy_55_meshes2.inc"
};
static Sint16 event_zero_amy_55_meshes3[5] = {
#include "shared/zero/event_zero_amy_55_meshes3.inc"
};
static NJS_COLOR event_zero_amy_55_uvs0[15] = {
#include "shared/zero/event_zero_amy_55_uvs0.inc"
};
static NJS_COLOR event_zero_amy_55_uvs1[8] = {
#include "shared/zero/event_zero_amy_55_uvs1.inc"
};
static NJS_COLOR event_zero_amy_55_uvs2[9] = {
#include "shared/zero/event_zero_amy_55_uvs2.inc"
};
static NJS_COLOR event_zero_amy_55_uvs3[4] = {
#include "shared/zero/event_zero_amy_55_uvs3.inc"
};
static NJS_MESHSET event_zero_amy_55_meshsets[4] = {
#include "shared/zero/event_zero_amy_55_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_55_points[14] = {
#include "shared/zero/event_zero_amy_55_points.inc"
};
static NJS_VECTOR event_zero_amy_55_normals[14] = {
#include "shared/zero/event_zero_amy_55_normals.inc"
};
static NJS_MODEL event_zero_amy_55_model = {
#include "shared/zero/event_zero_amy_55_model.inc"
};
static NJS_OBJECT event_zero_amy_55 = {
#include "shared/zero/event_zero_amy_55.inc"
};
static NJS_MATERIAL event_zero_amy_54_materials[4] = {
#include "shared/zero/event_zero_amy_54_materials.inc"
};
static Sint16 event_zero_amy_54_meshes0[11] = {
#include "shared/zero/event_zero_amy_54_meshes0.inc"
};
static Sint16 event_zero_amy_54_meshes1[18] = {
#include "shared/zero/event_zero_amy_54_meshes1.inc"
};
static Sint16 event_zero_amy_54_meshes2[7] = {
#include "shared/zero/event_zero_amy_54_meshes2.inc"
};
static Sint16 event_zero_amy_54_meshes3[6] = {
#include "shared/zero/event_zero_amy_54_meshes3.inc"
};
static NJS_COLOR event_zero_amy_54_uvs0[10] = {
#include "shared/zero/event_zero_amy_54_uvs0.inc"
};
static NJS_COLOR event_zero_amy_54_uvs1[15] = {
#include "shared/zero/event_zero_amy_54_uvs1.inc"
};
static NJS_COLOR event_zero_amy_54_uvs2[6] = {
#include "shared/zero/event_zero_amy_54_uvs2.inc"
};
static NJS_COLOR event_zero_amy_54_uvs3[5] = {
#include "shared/zero/event_zero_amy_54_uvs3.inc"
};
static NJS_MESHSET event_zero_amy_54_meshsets[4] = {
#include "shared/zero/event_zero_amy_54_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_54_points[18] = {
#include "shared/zero/event_zero_amy_54_points.inc"
};
static NJS_VECTOR event_zero_amy_54_normals[18] = {
#include "shared/zero/event_zero_amy_54_normals.inc"
};
static NJS_MODEL event_zero_amy_54_model = {
#include "shared/zero/event_zero_amy_54_model.inc"
};
static NJS_OBJECT event_zero_amy_54 = {
#include "shared/zero/event_zero_amy_54.inc"
};
static NJS_OBJECT event_zero_amy_53 = {
#include "shared/zero/event_zero_amy_53.inc"
};
static NJS_OBJECT event_zero_amy_52 = {
#include "shared/zero/event_zero_amy_52.inc"
};
static NJS_OBJECT event_zero_amy_51 = {
#include "shared/zero/event_zero_amy_51.inc"
};
static NJS_OBJECT event_zero_amy_50 = {
#include "shared/zero/event_zero_amy_50.inc"
};
static NJS_OBJECT event_zero_amy_49 = {
#include "shared/zero/event_zero_amy_49.inc"
};
static NJS_MATERIAL event_zero_amy_48_materials[1] = {
#include "shared/zero/event_zero_amy_48_materials.inc"
};
static Sint16 event_zero_amy_48_meshes0[16] = {
#include "shared/zero/event_zero_amy_48_meshes0.inc"
};
static NJS_COLOR event_zero_amy_48_uvs0[14] = {
#include "shared/zero/event_zero_amy_48_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_48_meshsets[1] = {
#include "shared/zero/event_zero_amy_48_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_48_points[8] = {
#include "shared/zero/event_zero_amy_48_points.inc"
};
static NJS_VECTOR event_zero_amy_48_normals[8] = {
#include "shared/zero/event_zero_amy_48_normals.inc"
};
static NJS_MODEL event_zero_amy_48_model = {
#include "shared/zero/event_zero_amy_48_model.inc"
};
static NJS_OBJECT event_zero_amy_48 = {
#include "shared/zero/event_zero_amy_48.inc"
};
static NJS_MATERIAL event_zero_amy_47_materials[1] = {
#include "shared/zero/event_zero_amy_47_materials.inc"
};
static Sint16 event_zero_amy_47_meshes0[11] = {
#include "shared/zero/event_zero_amy_47_meshes0.inc"
};
static NJS_COLOR event_zero_amy_47_uvs0[10] = {
#include "shared/zero/event_zero_amy_47_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_47_meshsets[1] = {
#include "shared/zero/event_zero_amy_47_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_47_points[8] = {
#include "shared/zero/event_zero_amy_47_points.inc"
};
static NJS_VECTOR event_zero_amy_47_normals[8] = {
#include "shared/zero/event_zero_amy_47_normals.inc"
};
static NJS_MODEL event_zero_amy_47_model = {
#include "shared/zero/event_zero_amy_47_model.inc"
};
static NJS_OBJECT event_zero_amy_47 = {
#include "shared/zero/event_zero_amy_47.inc"
};
static NJS_MATERIAL event_zero_amy_46_materials[2] = {
#include "shared/zero/event_zero_amy_46_materials.inc"
};
static Sint16 event_zero_amy_46_meshes0[41] = {
#include "shared/zero/event_zero_amy_46_meshes0.inc"
};
static Sint16 event_zero_amy_46_meshes1[11] = {
#include "shared/zero/event_zero_amy_46_meshes1.inc"
};
static NJS_COLOR event_zero_amy_46_uvs0[36] = {
#include "shared/zero/event_zero_amy_46_uvs0.inc"
};
static NJS_COLOR event_zero_amy_46_uvs1[10] = {
#include "shared/zero/event_zero_amy_46_uvs1.inc"
};
static NJS_MESHSET event_zero_amy_46_meshsets[2] = {
#include "shared/zero/event_zero_amy_46_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_46_points[23] = {
#include "shared/zero/event_zero_amy_46_points.inc"
};
static NJS_VECTOR event_zero_amy_46_normals[23] = {
#include "shared/zero/event_zero_amy_46_normals.inc"
};
static NJS_MODEL event_zero_amy_46_model = {
#include "shared/zero/event_zero_amy_46_model.inc"
};
static NJS_OBJECT event_zero_amy_46 = {
#include "shared/zero/event_zero_amy_46.inc"
};
static NJS_MATERIAL event_zero_amy_45_materials[1] = {
#include "shared/zero/event_zero_amy_45_materials.inc"
};
static Sint16 event_zero_amy_45_meshes0[35] = {
#include "shared/zero/event_zero_amy_45_meshes0.inc"
};
static NJS_COLOR event_zero_amy_45_uvs0[30] = {
#include "shared/zero/event_zero_amy_45_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_45_meshsets[1] = {
#include "shared/zero/event_zero_amy_45_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_45_points[12] = {
#include "shared/zero/event_zero_amy_45_points.inc"
};
static NJS_VECTOR event_zero_amy_45_normals[12] = {
#include "shared/zero/event_zero_amy_45_normals.inc"
};
static NJS_MODEL event_zero_amy_45_model = {
#include "shared/zero/event_zero_amy_45_model.inc"
};
static NJS_OBJECT event_zero_amy_45 = {
#include "shared/zero/event_zero_amy_45.inc"
};
static NJS_MATERIAL event_zero_amy_44_materials[2] = {
#include "shared/zero/event_zero_amy_44_materials.inc"
};
static Sint16 event_zero_amy_44_meshes0[39] = {
#include "shared/zero/event_zero_amy_44_meshes0.inc"
};
static Sint16 event_zero_amy_44_meshes1[7] = {
#include "shared/zero/event_zero_amy_44_meshes1.inc"
};
static NJS_COLOR event_zero_amy_44_uvs0[32] = {
#include "shared/zero/event_zero_amy_44_uvs0.inc"
};
static NJS_COLOR event_zero_amy_44_uvs1[6] = {
#include "shared/zero/event_zero_amy_44_uvs1.inc"
};
static NJS_MESHSET event_zero_amy_44_meshsets[2] = {
#include "shared/zero/event_zero_amy_44_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_44_points[14] = {
#include "shared/zero/event_zero_amy_44_points.inc"
};
static NJS_VECTOR event_zero_amy_44_normals[14] = {
#include "shared/zero/event_zero_amy_44_normals.inc"
};
static NJS_MODEL event_zero_amy_44_model = {
#include "shared/zero/event_zero_amy_44_model.inc"
};
static NJS_OBJECT event_zero_amy_44 = {
#include "shared/zero/event_zero_amy_44.inc"
};
static NJS_MATERIAL event_zero_amy_43_materials[2] = {
#include "shared/zero/event_zero_amy_43_materials.inc"
};
static Sint16 event_zero_amy_43_meshes0[19] = {
#include "shared/zero/event_zero_amy_43_meshes0.inc"
};
static Sint16 event_zero_amy_43_meshes1[5] = {
#include "shared/zero/event_zero_amy_43_meshes1.inc"
};
static NJS_COLOR event_zero_amy_43_uvs0[16] = {
#include "shared/zero/event_zero_amy_43_uvs0.inc"
};
static NJS_COLOR event_zero_amy_43_uvs1[4] = {
#include "shared/zero/event_zero_amy_43_uvs1.inc"
};
static NJS_MESHSET event_zero_amy_43_meshsets[2] = {
#include "shared/zero/event_zero_amy_43_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_43_points[9] = {
#include "shared/zero/event_zero_amy_43_points.inc"
};
static NJS_VECTOR event_zero_amy_43_normals[9] = {
#include "shared/zero/event_zero_amy_43_normals.inc"
};
static NJS_MODEL event_zero_amy_43_model = {
#include "shared/zero/event_zero_amy_43_model.inc"
};
static NJS_OBJECT event_zero_amy_43 = {
#include "shared/zero/event_zero_amy_43.inc"
};
static NJS_OBJECT event_zero_amy_42 = {
#include "shared/zero/event_zero_amy_42.inc"
};
static NJS_OBJECT event_zero_amy_41 = {
#include "shared/zero/event_zero_amy_41.inc"
};
static NJS_OBJECT event_zero_amy_40 = {
#include "shared/zero/event_zero_amy_40.inc"
};
static NJS_OBJECT event_zero_amy_39 = {
#include "shared/zero/event_zero_amy_39.inc"
};
static NJS_OBJECT event_zero_amy_38 = {
#include "shared/zero/event_zero_amy_38.inc"
};
static NJS_MATERIAL event_zero_amy_37_materials[1] = {
#include "shared/zero/event_zero_amy_37_materials.inc"
};
static Sint16 event_zero_amy_37_meshes0[16] = {
#include "shared/zero/event_zero_amy_37_meshes0.inc"
};
static NJS_COLOR event_zero_amy_37_uvs0[14] = {
#include "shared/zero/event_zero_amy_37_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_37_meshsets[1] = {
#include "shared/zero/event_zero_amy_37_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_37_points[8] = {
#include "shared/zero/event_zero_amy_37_points.inc"
};
static NJS_VECTOR event_zero_amy_37_normals[8] = {
#include "shared/zero/event_zero_amy_37_normals.inc"
};
static NJS_MODEL event_zero_amy_37_model = {
#include "shared/zero/event_zero_amy_37_model.inc"
};
static NJS_OBJECT event_zero_amy_37 = {
#include "shared/zero/event_zero_amy_37.inc"
};
static NJS_MATERIAL event_zero_amy_36_materials[1] = {
#include "shared/zero/event_zero_amy_36_materials.inc"
};
static Sint16 event_zero_amy_36_meshes0[11] = {
#include "shared/zero/event_zero_amy_36_meshes0.inc"
};
static NJS_COLOR event_zero_amy_36_uvs0[10] = {
#include "shared/zero/event_zero_amy_36_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_36_meshsets[1] = {
#include "shared/zero/event_zero_amy_36_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_36_points[8] = {
#include "shared/zero/event_zero_amy_36_points.inc"
};
static NJS_VECTOR event_zero_amy_36_normals[8] = {
#include "shared/zero/event_zero_amy_36_normals.inc"
};
static NJS_MODEL event_zero_amy_36_model = {
#include "shared/zero/event_zero_amy_36_model.inc"
};
static NJS_OBJECT event_zero_amy_36 = {
#include "shared/zero/event_zero_amy_36.inc"
};
static NJS_MATERIAL event_zero_amy_35_materials[1] = {
#include "shared/zero/event_zero_amy_35_materials.inc"
};
static Sint16 event_zero_amy_35_meshes0[35] = {
#include "shared/zero/event_zero_amy_35_meshes0.inc"
};
static NJS_COLOR event_zero_amy_35_uvs0[30] = {
#include "shared/zero/event_zero_amy_35_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_35_meshsets[1] = {
#include "shared/zero/event_zero_amy_35_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_35_points[12] = {
#include "shared/zero/event_zero_amy_35_points.inc"
};
static NJS_VECTOR event_zero_amy_35_normals[12] = {
#include "shared/zero/event_zero_amy_35_normals.inc"
};
static NJS_MODEL event_zero_amy_35_model = {
#include "shared/zero/event_zero_amy_35_model.inc"
};
static NJS_OBJECT event_zero_amy_35 = {
#include "shared/zero/event_zero_amy_35.inc"
};
static NJS_MATERIAL event_zero_amy_34_materials[2] = {
#include "shared/zero/event_zero_amy_34_materials.inc"
};
static Sint16 event_zero_amy_34_meshes0[36] = {
#include "shared/zero/event_zero_amy_34_meshes0.inc"
};
static Sint16 event_zero_amy_34_meshes1[7] = {
#include "shared/zero/event_zero_amy_34_meshes1.inc"
};
static NJS_COLOR event_zero_amy_34_uvs0[30] = {
#include "shared/zero/event_zero_amy_34_uvs0.inc"
};
static NJS_COLOR event_zero_amy_34_uvs1[6] = {
#include "shared/zero/event_zero_amy_34_uvs1.inc"
};
static NJS_MESHSET event_zero_amy_34_meshsets[2] = {
#include "shared/zero/event_zero_amy_34_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_34_points[14] = {
#include "shared/zero/event_zero_amy_34_points.inc"
};
static NJS_VECTOR event_zero_amy_34_normals[14] = {
#include "shared/zero/event_zero_amy_34_normals.inc"
};
static NJS_MODEL event_zero_amy_34_model = {
#include "shared/zero/event_zero_amy_34_model.inc"
};
static NJS_OBJECT event_zero_amy_34 = {
#include "shared/zero/event_zero_amy_34.inc"
};
static NJS_MATERIAL event_zero_amy_33_materials[2] = {
#include "shared/zero/event_zero_amy_33_materials.inc"
};
static Sint16 event_zero_amy_33_meshes0[19] = {
#include "shared/zero/event_zero_amy_33_meshes0.inc"
};
static Sint16 event_zero_amy_33_meshes1[5] = {
#include "shared/zero/event_zero_amy_33_meshes1.inc"
};
static NJS_COLOR event_zero_amy_33_uvs0[16] = {
#include "shared/zero/event_zero_amy_33_uvs0.inc"
};
static NJS_COLOR event_zero_amy_33_uvs1[4] = {
#include "shared/zero/event_zero_amy_33_uvs1.inc"
};
static NJS_MESHSET event_zero_amy_33_meshsets[2] = {
#include "shared/zero/event_zero_amy_33_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_33_points[9] = {
#include "shared/zero/event_zero_amy_33_points.inc"
};
static NJS_VECTOR event_zero_amy_33_normals[9] = {
#include "shared/zero/event_zero_amy_33_normals.inc"
};
static NJS_MODEL event_zero_amy_33_model = {
#include "shared/zero/event_zero_amy_33_model.inc"
};
static NJS_OBJECT event_zero_amy_33 = {
#include "shared/zero/event_zero_amy_33.inc"
};
static NJS_MATERIAL event_zero_amy_32_materials[2] = {
#include "shared/zero/event_zero_amy_32_materials.inc"
};
static Sint16 event_zero_amy_32_meshes0[38] = {
#include "shared/zero/event_zero_amy_32_meshes0.inc"
};
static Sint16 event_zero_amy_32_meshes1[11] = {
#include "shared/zero/event_zero_amy_32_meshes1.inc"
};
static NJS_COLOR event_zero_amy_32_uvs0[34] = {
#include "shared/zero/event_zero_amy_32_uvs0.inc"
};
static NJS_COLOR event_zero_amy_32_uvs1[10] = {
#include "shared/zero/event_zero_amy_32_uvs1.inc"
};
static NJS_MESHSET event_zero_amy_32_meshsets[2] = {
#include "shared/zero/event_zero_amy_32_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_32_points[23] = {
#include "shared/zero/event_zero_amy_32_points.inc"
};
static NJS_VECTOR event_zero_amy_32_normals[23] = {
#include "shared/zero/event_zero_amy_32_normals.inc"
};
static NJS_MODEL event_zero_amy_32_model = {
#include "shared/zero/event_zero_amy_32_model.inc"
};
static NJS_OBJECT event_zero_amy_32 = {
#include "shared/zero/event_zero_amy_32.inc"
};
static NJS_OBJECT event_zero_amy_31 = {
#include "shared/zero/event_zero_amy_31.inc"
};
static NJS_OBJECT event_zero_amy_30 = {
#include "shared/zero/event_zero_amy_30.inc"
};
static NJS_OBJECT event_zero_amy_29 = {
#include "shared/zero/event_zero_amy_29.inc"
};
static NJS_OBJECT event_zero_amy_28 = {
#include "shared/zero/event_zero_amy_28.inc"
};
static NJS_OBJECT event_zero_amy_27 = {
#include "shared/zero/event_zero_amy_27.inc"
};
static NJS_OBJECT event_zero_amy_26 = {
#include "shared/zero/event_zero_amy_26.inc"
};
static NJS_OBJECT event_zero_amy_25 = {
#include "shared/zero/event_zero_amy_25.inc"
};
static NJS_MATERIAL event_zero_amy_24_materials[1] = {
#include "shared/zero/event_zero_amy_24_materials.inc"
};
static Sint16 event_zero_amy_24_meshes0[99] = {
#include "shared/zero/event_zero_amy_24_meshes0.inc"
};
static NJS_COLOR event_zero_amy_24_uvs0[90] = {
#include "shared/zero/event_zero_amy_24_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_24_meshsets[1] = {
#include "shared/zero/event_zero_amy_24_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_24_points[49] = {
#include "shared/zero/event_zero_amy_24_points.inc"
};
static NJS_VECTOR event_zero_amy_24_normals[49] = {
#include "shared/zero/event_zero_amy_24_normals.inc"
};
static NJS_MODEL event_zero_amy_24_model = {
#include "shared/zero/event_zero_amy_24_model.inc"
};
static NJS_OBJECT event_zero_amy_24 = {
#include "shared/zero/event_zero_amy_24.inc"
};
static NJS_MATERIAL event_zero_amy_23_materials[1] = {
#include "shared/zero/event_zero_amy_23_materials.inc"
};
static Sint16 event_zero_amy_23_meshes0[21] = {
#include "shared/zero/event_zero_amy_23_meshes0.inc"
};
static NJS_COLOR event_zero_amy_23_uvs0[20] = {
#include "shared/zero/event_zero_amy_23_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_23_meshsets[1] = {
#include "shared/zero/event_zero_amy_23_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_23_points[20] = {
#include "shared/zero/event_zero_amy_23_points.inc"
};
static NJS_VECTOR event_zero_amy_23_normals[20] = {
#include "shared/zero/event_zero_amy_23_normals.inc"
};
static NJS_MODEL event_zero_amy_23_model = {
#include "shared/zero/event_zero_amy_23_model.inc"
};
static NJS_OBJECT event_zero_amy_23 = {
#include "shared/zero/event_zero_amy_23.inc"
};
static NJS_OBJECT event_zero_amy_22 = {
#include "shared/zero/event_zero_amy_22.inc"
};
static NJS_MATERIAL event_zero_amy_21_materials[1] = {
#include "shared/zero/event_zero_amy_21_materials.inc"
};
static Sint16 event_zero_amy_21_meshes0[18] = {
#include "shared/zero/event_zero_amy_21_meshes0.inc"
};
static NJS_COLOR event_zero_amy_21_uvs0[16] = {
#include "shared/zero/event_zero_amy_21_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_21_meshsets[1] = {
#include "shared/zero/event_zero_amy_21_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_21_points[11] = {
#include "shared/zero/event_zero_amy_21_points.inc"
};
static NJS_VECTOR event_zero_amy_21_normals[11] = {
#include "shared/zero/event_zero_amy_21_normals.inc"
};
static NJS_MODEL event_zero_amy_21_model = {
#include "shared/zero/event_zero_amy_21_model.inc"
};
static NJS_OBJECT event_zero_amy_21 = {
#include "shared/zero/event_zero_amy_21.inc"
};
static NJS_OBJECT event_zero_amy_20 = {
#include "shared/zero/event_zero_amy_20.inc"
};
static NJS_OBJECT event_zero_amy_19 = {
#include "shared/zero/event_zero_amy_19.inc"
};
static NJS_OBJECT event_zero_amy_18 = {
#include "shared/zero/event_zero_amy_18.inc"
};
static NJS_MATERIAL event_zero_amy_17_materials[1] = {
#include "shared/zero/event_zero_amy_17_materials.inc"
};
static Sint16 event_zero_amy_17_meshes0[18] = {
#include "shared/zero/event_zero_amy_17_meshes0.inc"
};
static NJS_COLOR event_zero_amy_17_uvs0[16] = {
#include "shared/zero/event_zero_amy_17_uvs0.inc"
};
static NJS_MESHSET event_zero_amy_17_meshsets[1] = {
#include "shared/zero/event_zero_amy_17_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_17_points[11] = {
#include "shared/zero/event_zero_amy_17_points.inc"
};
static NJS_VECTOR event_zero_amy_17_normals[11] = {
#include "shared/zero/event_zero_amy_17_normals.inc"
};
static NJS_MODEL event_zero_amy_17_model = {
#include "shared/zero/event_zero_amy_17_model.inc"
};
static NJS_OBJECT event_zero_amy_17 = {
#include "shared/zero/event_zero_amy_17.inc"
};
static NJS_OBJECT event_zero_amy_16 = {
#include "shared/zero/event_zero_amy_16.inc"
};
static NJS_OBJECT event_zero_amy_15 = {
#include "shared/zero/event_zero_amy_15.inc"
};
static NJS_OBJECT event_zero_amy_14 = {
#include "shared/zero/event_zero_amy_14.inc"
};
static NJS_OBJECT event_zero_amy_13 = {
#include "shared/zero/event_zero_amy_13.inc"
};
static NJS_OBJECT event_zero_amy_12 = {
#include "shared/zero/event_zero_amy_12.inc"
};
static NJS_OBJECT event_zero_amy_11 = {
#include "shared/zero/event_zero_amy_11.inc"
};
static NJS_OBJECT event_zero_amy_10 = {
#include "shared/zero/event_zero_amy_10.inc"
};
static NJS_OBJECT event_zero_amy_9 = {
#include "shared/zero/event_zero_amy_9.inc"
};
static NJS_OBJECT event_zero_amy_8 = {
#include "shared/zero/event_zero_amy_8.inc"
};
static NJS_OBJECT event_zero_amy_7 = {
#include "shared/zero/event_zero_amy_7.inc"
};
static NJS_OBJECT event_zero_amy_6 = {
#include "shared/zero/event_zero_amy_6.inc"
};
static NJS_OBJECT event_zero_amy_5 = {
#include "shared/zero/event_zero_amy_5.inc"
};
static NJS_OBJECT event_zero_amy_4 = {
#include "shared/zero/event_zero_amy_4.inc"
};
static NJS_OBJECT event_zero_amy_3 = {
#include "shared/zero/event_zero_amy_3.inc"
};
static NJS_OBJECT event_zero_amy_2 = {
#include "shared/zero/event_zero_amy_2.inc"
};
static NJS_OBJECT event_zero_amy_1 = {
#include "shared/zero/event_zero_amy_1.inc"
};
static NJS_OBJECT event_zero_amy_object = {
#include "shared/zero/event_zero_amy_object.inc"
};
static NJS_MATERIAL event_zero_amy_head_materials[6] = {
#include "shared/zero/event_zero_amy_head_materials.inc"
};
static Sint16 event_zero_amy_head_meshes0[49] = {
#include "shared/zero/event_zero_amy_head_meshes0.inc"
};
static Sint16 event_zero_amy_head_meshes1[53] = {
#include "shared/zero/event_zero_amy_head_meshes1.inc"
};
static Sint16 event_zero_amy_head_meshes2[15] = {
#include "shared/zero/event_zero_amy_head_meshes2.inc"
};
static Sint16 event_zero_amy_head_meshes3[223] = {
#include "shared/zero/event_zero_amy_head_meshes3.inc"
};
static Sint16 event_zero_amy_head_meshes4[497] = {
#include "shared/zero/event_zero_amy_head_meshes4.inc"
};
static Sint16 event_zero_amy_head_meshes5[19] = {
#include "shared/zero/event_zero_amy_head_meshes5.inc"
};
static NJS_COLOR event_zero_amy_head_uvs0[46] = {
#include "shared/zero/event_zero_amy_head_uvs0.inc"
};
static NJS_COLOR event_zero_amy_head_uvs1[50] = {
#include "shared/zero/event_zero_amy_head_uvs1.inc"
};
static NJS_COLOR event_zero_amy_head_uvs2[14] = {
#include "shared/zero/event_zero_amy_head_uvs2.inc"
};
static NJS_COLOR event_zero_amy_head_uvs3[206] = {
#include "shared/zero/event_zero_amy_head_uvs3.inc"
};
static NJS_COLOR event_zero_amy_head_uvs4[431] = {
#include "shared/zero/event_zero_amy_head_uvs4.inc"
};
static NJS_COLOR event_zero_amy_head_uvs5[16] = {
#include "shared/zero/event_zero_amy_head_uvs5.inc"
};
static NJS_MESHSET event_zero_amy_head_meshsets[6] = {
#include "shared/zero/event_zero_amy_head_meshsets.inc"
};
static NJS_POINT3 event_zero_amy_head_points[330] = {
#include "shared/zero/event_zero_amy_head_points.inc"
};
static NJS_VECTOR event_zero_amy_head_normals[330] = {
#include "shared/zero/event_zero_amy_head_normals.inc"
};
static NJS_MODEL event_zero_amy_head_model = {
#include "shared/zero/event_zero_amy_head_model.inc"
};
static NJS_OBJECT event_zero_amy_head_object = {
#include "shared/zero/event_zero_amy_head_object.inc"
};
static NJS_OBJECT event_zero_47 = {
#include "shared/zero/event_zero_47.inc"
};
static NJS_OBJECT event_zero_46 = {
#include "shared/zero/event_zero_46.inc"
};
static NJS_OBJECT event_zero_45 = {
#include "shared/zero/event_zero_45.inc"
};
static NJS_OBJECT event_zero_44 = {
#include "shared/zero/event_zero_44.inc"
};
static NJS_OBJECT event_zero_43 = {
#include "shared/zero/event_zero_43.inc"
};
static NJS_OBJECT event_zero_42 = {
#include "shared/zero/event_zero_42.inc"
};
static NJS_MATERIAL event_zero_41_materials[2] = {
#include "shared/zero/event_zero_41_materials.inc"
};
static Sint16 event_zero_41_meshes0[15] = {
#include "shared/zero/event_zero_41_meshes0.inc"
};
static Sint16 event_zero_41_meshes1[10] = {
#include "shared/zero/event_zero_41_meshes1.inc"
};
static NJS_COLOR event_zero_41_uvs0[14] = {
#include "shared/zero/event_zero_41_uvs0.inc"
};
static NJS_COLOR event_zero_41_uvs1[8] = {
#include "shared/zero/event_zero_41_uvs1.inc"
};
static NJS_MESHSET event_zero_41_meshsets[2] = {
#include "shared/zero/event_zero_41_meshsets.inc"
};
static NJS_POINT3 event_zero_41_points[12] = {
#include "shared/zero/event_zero_41_points.inc"
};
static NJS_VECTOR event_zero_41_normals[12] = {
#include "shared/zero/event_zero_41_normals.inc"
};
static NJS_MODEL event_zero_41_model = {
#include "shared/zero/event_zero_41_model.inc"
};
static NJS_OBJECT event_zero_41 = {
#include "shared/zero/event_zero_41.inc"
};
static NJS_MATERIAL event_zero_40_materials[2] = {
#include "shared/zero/event_zero_40_materials.inc"
};
static Sint16 event_zero_40_meshes0[15] = {
#include "shared/zero/event_zero_40_meshes0.inc"
};
static Sint16 event_zero_40_meshes1[10] = {
#include "shared/zero/event_zero_40_meshes1.inc"
};
static NJS_COLOR event_zero_40_uvs0[14] = {
#include "shared/zero/event_zero_40_uvs0.inc"
};
static NJS_COLOR event_zero_40_uvs1[8] = {
#include "shared/zero/event_zero_40_uvs1.inc"
};
static NJS_MESHSET event_zero_40_meshsets[2] = {
#include "shared/zero/event_zero_40_meshsets.inc"
};
static NJS_POINT3 event_zero_40_points[12] = {
#include "shared/zero/event_zero_40_points.inc"
};
static NJS_VECTOR event_zero_40_normals[12] = {
#include "shared/zero/event_zero_40_normals.inc"
};
static NJS_MODEL event_zero_40_model = {
#include "shared/zero/event_zero_40_model.inc"
};
static NJS_OBJECT event_zero_40 = {
#include "shared/zero/event_zero_40.inc"
};
static NJS_MATERIAL event_zero_39_materials[3] = {
#include "shared/zero/event_zero_39_materials.inc"
};
static Sint16 event_zero_39_meshes0[20] = {
#include "shared/zero/event_zero_39_meshes0.inc"
};
static Sint16 event_zero_39_meshes1[25] = {
#include "shared/zero/event_zero_39_meshes1.inc"
};
static Sint16 event_zero_39_meshes2[11] = {
#include "shared/zero/event_zero_39_meshes2.inc"
};
static NJS_COLOR event_zero_39_uvs0[15] = {
#include "shared/zero/event_zero_39_uvs0.inc"
};
static NJS_COLOR event_zero_39_uvs1[20] = {
#include "shared/zero/event_zero_39_uvs1.inc"
};
static NJS_COLOR event_zero_39_uvs2[9] = {
#include "shared/zero/event_zero_39_uvs2.inc"
};
static NJS_MESHSET event_zero_39_meshsets[3] = {
#include "shared/zero/event_zero_39_meshsets.inc"
};
static NJS_POINT3 event_zero_39_points[12] = {
#include "shared/zero/event_zero_39_points.inc"
};
static NJS_VECTOR event_zero_39_normals[12] = {
#include "shared/zero/event_zero_39_normals.inc"
};
static NJS_MODEL event_zero_39_model = {
#include "shared/zero/event_zero_39_model.inc"
};
static NJS_OBJECT event_zero_39 = {
#include "shared/zero/event_zero_39.inc"
};
static NJS_MATERIAL event_zero_38_materials[3] = {
#include "shared/zero/event_zero_38_materials.inc"
};
static Sint16 event_zero_38_meshes0[20] = {
#include "shared/zero/event_zero_38_meshes0.inc"
};
static Sint16 event_zero_38_meshes1[25] = {
#include "shared/zero/event_zero_38_meshes1.inc"
};
static Sint16 event_zero_38_meshes2[11] = {
#include "shared/zero/event_zero_38_meshes2.inc"
};
static NJS_COLOR event_zero_38_uvs0[15] = {
#include "shared/zero/event_zero_38_uvs0.inc"
};
static NJS_COLOR event_zero_38_uvs1[20] = {
#include "shared/zero/event_zero_38_uvs1.inc"
};
static NJS_COLOR event_zero_38_uvs2[9] = {
#include "shared/zero/event_zero_38_uvs2.inc"
};
static NJS_MESHSET event_zero_38_meshsets[3] = {
#include "shared/zero/event_zero_38_meshsets.inc"
};
static NJS_POINT3 event_zero_38_points[12] = {
#include "shared/zero/event_zero_38_points.inc"
};
static NJS_VECTOR event_zero_38_normals[12] = {
#include "shared/zero/event_zero_38_normals.inc"
};
static NJS_MODEL event_zero_38_model = {
#include "shared/zero/event_zero_38_model.inc"
};
static NJS_OBJECT event_zero_38 = {
#include "shared/zero/event_zero_38.inc"
};
static NJS_MATERIAL event_zero_37_materials[5] = {
#include "shared/zero/event_zero_37_materials.inc"
};
static Sint16 event_zero_37_meshes0[30] = {
#include "shared/zero/event_zero_37_meshes0.inc"
};
static Sint16 event_zero_37_meshes1[10] = {
#include "shared/zero/event_zero_37_meshes1.inc"
};
static Sint16 event_zero_37_meshes2[20] = {
#include "shared/zero/event_zero_37_meshes2.inc"
};
static Sint16 event_zero_37_meshes3[10] = {
#include "shared/zero/event_zero_37_meshes3.inc"
};
static Sint16 event_zero_37_meshes4[27] = {
#include "shared/zero/event_zero_37_meshes4.inc"
};
static NJS_COLOR event_zero_37_uvs0[24] = {
#include "shared/zero/event_zero_37_uvs0.inc"
};
static NJS_COLOR event_zero_37_uvs1[8] = {
#include "shared/zero/event_zero_37_uvs1.inc"
};
static NJS_COLOR event_zero_37_uvs2[16] = {
#include "shared/zero/event_zero_37_uvs2.inc"
};
static NJS_COLOR event_zero_37_uvs3[8] = {
#include "shared/zero/event_zero_37_uvs3.inc"
};
static NJS_COLOR event_zero_37_uvs4[24] = {
#include "shared/zero/event_zero_37_uvs4.inc"
};
static NJS_MESHSET event_zero_37_meshsets[5] = {
#include "shared/zero/event_zero_37_meshsets.inc"
};
static NJS_POINT3 event_zero_37_points[33] = {
#include "shared/zero/event_zero_37_points.inc"
};
static NJS_VECTOR event_zero_37_normals[33] = {
#include "shared/zero/event_zero_37_normals.inc"
};
static NJS_MODEL event_zero_37_model = {
#include "shared/zero/event_zero_37_model.inc"
};
static NJS_OBJECT event_zero_37 = {
#include "shared/zero/event_zero_37.inc"
};
static NJS_MATERIAL event_zero_36_materials[1] = {
#include "shared/zero/event_zero_36_materials.inc"
};
static Sint16 event_zero_36_meshes0[16] = {
#include "shared/zero/event_zero_36_meshes0.inc"
};
static NJS_COLOR event_zero_36_uvs0[14] = {
#include "shared/zero/event_zero_36_uvs0.inc"
};
static NJS_MESHSET event_zero_36_meshsets[1] = {
#include "shared/zero/event_zero_36_meshsets.inc"
};
static NJS_POINT3 event_zero_36_points[8] = {
#include "shared/zero/event_zero_36_points.inc"
};
static NJS_VECTOR event_zero_36_normals[8] = {
#include "shared/zero/event_zero_36_normals.inc"
};
static NJS_MODEL event_zero_36_model = {
#include "shared/zero/event_zero_36_model.inc"
};
static NJS_OBJECT event_zero_36 = {
#include "shared/zero/event_zero_36.inc"
};
static NJS_MATERIAL event_zero_35_materials[1] = {
#include "shared/zero/event_zero_35_materials.inc"
};
static Sint16 event_zero_35_meshes0[30] = {
#include "shared/zero/event_zero_35_meshes0.inc"
};
static NJS_COLOR event_zero_35_uvs0[24] = {
#include "shared/zero/event_zero_35_uvs0.inc"
};
static NJS_MESHSET event_zero_35_meshsets[1] = {
#include "shared/zero/event_zero_35_meshsets.inc"
};
static NJS_POINT3 event_zero_35_points[8] = {
#include "shared/zero/event_zero_35_points.inc"
};
static NJS_VECTOR event_zero_35_normals[8] = {
#include "shared/zero/event_zero_35_normals.inc"
};
static NJS_MODEL event_zero_35_model = {
#include "shared/zero/event_zero_35_model.inc"
};
static NJS_OBJECT event_zero_35 = {
#include "shared/zero/event_zero_35.inc"
};
static NJS_MATERIAL event_zero_34_materials[3] = {
#include "shared/zero/event_zero_34_materials.inc"
};
static Sint16 event_zero_34_meshes0[36] = {
#include "shared/zero/event_zero_34_meshes0.inc"
};
static Sint16 event_zero_34_meshes1[25] = {
#include "shared/zero/event_zero_34_meshes1.inc"
};
static Sint16 event_zero_34_meshes2[6] = {
#include "shared/zero/event_zero_34_meshes2.inc"
};
static NJS_COLOR event_zero_34_uvs0[29] = {
#include "shared/zero/event_zero_34_uvs0.inc"
};
static NJS_COLOR event_zero_34_uvs1[20] = {
#include "shared/zero/event_zero_34_uvs1.inc"
};
static NJS_COLOR event_zero_34_uvs2[5] = {
#include "shared/zero/event_zero_34_uvs2.inc"
};
static NJS_MESHSET event_zero_34_meshsets[3] = {
#include "shared/zero/event_zero_34_meshsets.inc"
};
static NJS_POINT3 event_zero_34_points[16] = {
#include "shared/zero/event_zero_34_points.inc"
};
static NJS_VECTOR event_zero_34_normals[16] = {
#include "shared/zero/event_zero_34_normals.inc"
};
static NJS_MODEL event_zero_34_model = {
#include "shared/zero/event_zero_34_model.inc"
};
static NJS_OBJECT event_zero_34 = {
#include "shared/zero/event_zero_34.inc"
};
static NJS_MATERIAL event_zero_33_materials[3] = {
#include "shared/zero/event_zero_33_materials.inc"
};
static Sint16 event_zero_33_meshes0[17] = {
#include "shared/zero/event_zero_33_meshes0.inc"
};
static Sint16 event_zero_33_meshes1[16] = {
#include "shared/zero/event_zero_33_meshes1.inc"
};
static Sint16 event_zero_33_meshes2[5] = {
#include "shared/zero/event_zero_33_meshes2.inc"
};
static NJS_COLOR event_zero_33_uvs0[14] = {
#include "shared/zero/event_zero_33_uvs0.inc"
};
static NJS_COLOR event_zero_33_uvs1[14] = {
#include "shared/zero/event_zero_33_uvs1.inc"
};
static NJS_COLOR event_zero_33_uvs2[4] = {
#include "shared/zero/event_zero_33_uvs2.inc"
};
static NJS_MESHSET event_zero_33_meshsets[3] = {
#include "shared/zero/event_zero_33_meshsets.inc"
};
static NJS_POINT3 event_zero_33_points[14] = {
#include "shared/zero/event_zero_33_points.inc"
};
static NJS_VECTOR event_zero_33_normals[14] = {
#include "shared/zero/event_zero_33_normals.inc"
};
static NJS_MODEL event_zero_33_model = {
#include "shared/zero/event_zero_33_model.inc"
};
static NJS_OBJECT event_zero_33 = {
#include "shared/zero/event_zero_33.inc"
};
static NJS_MATERIAL event_zero_32_materials[3] = {
#include "shared/zero/event_zero_32_materials.inc"
};
static Sint16 event_zero_32_meshes0[19] = {
#include "shared/zero/event_zero_32_meshes0.inc"
};
static Sint16 event_zero_32_meshes1[5] = {
#include "shared/zero/event_zero_32_meshes1.inc"
};
static Sint16 event_zero_32_meshes2[17] = {
#include "shared/zero/event_zero_32_meshes2.inc"
};
static NJS_COLOR event_zero_32_uvs0[16] = {
#include "shared/zero/event_zero_32_uvs0.inc"
};
static NJS_COLOR event_zero_32_uvs1[4] = {
#include "shared/zero/event_zero_32_uvs1.inc"
};
static NJS_COLOR event_zero_32_uvs2[14] = {
#include "shared/zero/event_zero_32_uvs2.inc"
};
static NJS_MESHSET event_zero_32_meshsets[3] = {
#include "shared/zero/event_zero_32_meshsets.inc"
};
static NJS_POINT3 event_zero_32_points[14] = {
#include "shared/zero/event_zero_32_points.inc"
};
static NJS_VECTOR event_zero_32_normals[14] = {
#include "shared/zero/event_zero_32_normals.inc"
};
static NJS_MODEL event_zero_32_model = {
#include "shared/zero/event_zero_32_model.inc"
};
static NJS_OBJECT event_zero_32 = {
#include "shared/zero/event_zero_32.inc"
};
static NJS_MATERIAL event_zero_31_materials[2] = {
#include "shared/zero/event_zero_31_materials.inc"
};
static Sint16 event_zero_31_meshes0[28] = {
#include "shared/zero/event_zero_31_meshes0.inc"
};
static Sint16 event_zero_31_meshes1[7] = {
#include "shared/zero/event_zero_31_meshes1.inc"
};
static NJS_COLOR event_zero_31_uvs0[24] = {
#include "shared/zero/event_zero_31_uvs0.inc"
};
static NJS_COLOR event_zero_31_uvs1[6] = {
#include "shared/zero/event_zero_31_uvs1.inc"
};
static NJS_MESHSET event_zero_31_meshsets[2] = {
#include "shared/zero/event_zero_31_meshsets.inc"
};
static NJS_POINT3 event_zero_31_points[12] = {
#include "shared/zero/event_zero_31_points.inc"
};
static NJS_VECTOR event_zero_31_normals[12] = {
#include "shared/zero/event_zero_31_normals.inc"
};
static NJS_MODEL event_zero_31_model = {
#include "shared/zero/event_zero_31_model.inc"
};
static NJS_OBJECT event_zero_31 = {
#include "shared/zero/event_zero_31.inc"
};
static NJS_MATERIAL event_zero_30_materials[2] = {
#include "shared/zero/event_zero_30_materials.inc"
};
static Sint16 event_zero_30_meshes0[21] = {
#include "shared/zero/event_zero_30_meshes0.inc"
};
static Sint16 event_zero_30_meshes1[17] = {
#include "shared/zero/event_zero_30_meshes1.inc"
};
static NJS_COLOR event_zero_30_uvs0[18] = {
#include "shared/zero/event_zero_30_uvs0.inc"
};
static NJS_COLOR event_zero_30_uvs1[14] = {
#include "shared/zero/event_zero_30_uvs1.inc"
};
static NJS_MESHSET event_zero_30_meshsets[2] = {
#include "shared/zero/event_zero_30_meshsets.inc"
};
static NJS_POINT3 event_zero_30_points[14] = {
#include "shared/zero/event_zero_30_points.inc"
};
static NJS_VECTOR event_zero_30_normals[14] = {
#include "shared/zero/event_zero_30_normals.inc"
};
static NJS_MODEL event_zero_30_model = {
#include "shared/zero/event_zero_30_model.inc"
};
static NJS_OBJECT event_zero_30 = {
#include "shared/zero/event_zero_30.inc"
};
static NJS_OBJECT event_zero_29 = {
#include "shared/zero/event_zero_29.inc"
};
static NJS_OBJECT event_zero_28 = {
#include "shared/zero/event_zero_28.inc"
};
static NJS_OBJECT event_zero_27 = {
#include "shared/zero/event_zero_27.inc"
};
static NJS_OBJECT event_zero_26 = {
#include "shared/zero/event_zero_26.inc"
};
static NJS_OBJECT event_zero_25 = {
#include "shared/zero/event_zero_25.inc"
};
static NJS_MATERIAL event_zero_24_materials[1] = {
#include "shared/zero/event_zero_24_materials.inc"
};
static Sint16 event_zero_24_meshes0[16] = {
#include "shared/zero/event_zero_24_meshes0.inc"
};
static NJS_COLOR event_zero_24_uvs0[14] = {
#include "shared/zero/event_zero_24_uvs0.inc"
};
static NJS_MESHSET event_zero_24_meshsets[1] = {
#include "shared/zero/event_zero_24_meshsets.inc"
};
static NJS_POINT3 event_zero_24_points[8] = {
#include "shared/zero/event_zero_24_points.inc"
};
static NJS_VECTOR event_zero_24_normals[8] = {
#include "shared/zero/event_zero_24_normals.inc"
};
static NJS_MODEL event_zero_24_model = {
#include "shared/zero/event_zero_24_model.inc"
};
static NJS_OBJECT event_zero_24 = {
#include "shared/zero/event_zero_24.inc"
};
static NJS_MATERIAL event_zero_23_materials[1] = {
#include "shared/zero/event_zero_23_materials.inc"
};
static Sint16 event_zero_23_meshes0[30] = {
#include "shared/zero/event_zero_23_meshes0.inc"
};
static NJS_COLOR event_zero_23_uvs0[24] = {
#include "shared/zero/event_zero_23_uvs0.inc"
};
static NJS_MESHSET event_zero_23_meshsets[1] = {
#include "shared/zero/event_zero_23_meshsets.inc"
};
static NJS_POINT3 event_zero_23_points[8] = {
#include "shared/zero/event_zero_23_points.inc"
};
static NJS_VECTOR event_zero_23_normals[8] = {
#include "shared/zero/event_zero_23_normals.inc"
};
static NJS_MODEL event_zero_23_model = {
#include "shared/zero/event_zero_23_model.inc"
};
static NJS_OBJECT event_zero_23 = {
#include "shared/zero/event_zero_23.inc"
};
static NJS_MATERIAL event_zero_22_materials[3] = {
#include "shared/zero/event_zero_22_materials.inc"
};
static Sint16 event_zero_22_meshes0[36] = {
#include "shared/zero/event_zero_22_meshes0.inc"
};
static Sint16 event_zero_22_meshes1[25] = {
#include "shared/zero/event_zero_22_meshes1.inc"
};
static Sint16 event_zero_22_meshes2[6] = {
#include "shared/zero/event_zero_22_meshes2.inc"
};
static NJS_COLOR event_zero_22_uvs0[29] = {
#include "shared/zero/event_zero_22_uvs0.inc"
};
static NJS_COLOR event_zero_22_uvs1[20] = {
#include "shared/zero/event_zero_22_uvs1.inc"
};
static NJS_COLOR event_zero_22_uvs2[5] = {
#include "shared/zero/event_zero_22_uvs2.inc"
};
static NJS_MESHSET event_zero_22_meshsets[3] = {
#include "shared/zero/event_zero_22_meshsets.inc"
};
static NJS_POINT3 event_zero_22_points[16] = {
#include "shared/zero/event_zero_22_points.inc"
};
static NJS_VECTOR event_zero_22_normals[16] = {
#include "shared/zero/event_zero_22_normals.inc"
};
static NJS_MODEL event_zero_22_model = {
#include "shared/zero/event_zero_22_model.inc"
};
static NJS_OBJECT event_zero_22 = {
#include "shared/zero/event_zero_22.inc"
};
static NJS_MATERIAL event_zero_21_materials[3] = {
#include "shared/zero/event_zero_21_materials.inc"
};
static Sint16 event_zero_21_meshes0[17] = {
#include "shared/zero/event_zero_21_meshes0.inc"
};
static Sint16 event_zero_21_meshes1[16] = {
#include "shared/zero/event_zero_21_meshes1.inc"
};
static Sint16 event_zero_21_meshes2[5] = {
#include "shared/zero/event_zero_21_meshes2.inc"
};
static NJS_COLOR event_zero_21_uvs0[14] = {
#include "shared/zero/event_zero_21_uvs0.inc"
};
static NJS_COLOR event_zero_21_uvs1[14] = {
#include "shared/zero/event_zero_21_uvs1.inc"
};
static NJS_COLOR event_zero_21_uvs2[4] = {
#include "shared/zero/event_zero_21_uvs2.inc"
};
static NJS_MESHSET event_zero_21_meshsets[3] = {
#include "shared/zero/event_zero_21_meshsets.inc"
};
static NJS_POINT3 event_zero_21_points[14] = {
#include "shared/zero/event_zero_21_points.inc"
};
static NJS_VECTOR event_zero_21_normals[14] = {
#include "shared/zero/event_zero_21_normals.inc"
};
static NJS_MODEL event_zero_21_model = {
#include "shared/zero/event_zero_21_model.inc"
};
static NJS_OBJECT event_zero_21 = {
#include "shared/zero/event_zero_21.inc"
};
static NJS_MATERIAL event_zero_20_materials[3] = {
#include "shared/zero/event_zero_20_materials.inc"
};
static Sint16 event_zero_20_meshes0[19] = {
#include "shared/zero/event_zero_20_meshes0.inc"
};
static Sint16 event_zero_20_meshes1[5] = {
#include "shared/zero/event_zero_20_meshes1.inc"
};
static Sint16 event_zero_20_meshes2[17] = {
#include "shared/zero/event_zero_20_meshes2.inc"
};
static NJS_COLOR event_zero_20_uvs0[16] = {
#include "shared/zero/event_zero_20_uvs0.inc"
};
static NJS_COLOR event_zero_20_uvs1[4] = {
#include "shared/zero/event_zero_20_uvs1.inc"
};
static NJS_COLOR event_zero_20_uvs2[14] = {
#include "shared/zero/event_zero_20_uvs2.inc"
};
static NJS_MESHSET event_zero_20_meshsets[3] = {
#include "shared/zero/event_zero_20_meshsets.inc"
};
static NJS_POINT3 event_zero_20_points[14] = {
#include "shared/zero/event_zero_20_points.inc"
};
static NJS_VECTOR event_zero_20_normals[14] = {
#include "shared/zero/event_zero_20_normals.inc"
};
static NJS_MODEL event_zero_20_model = {
#include "shared/zero/event_zero_20_model.inc"
};
static NJS_OBJECT event_zero_20 = {
#include "shared/zero/event_zero_20.inc"
};
static NJS_MATERIAL event_zero_19_materials[2] = {
#include "shared/zero/event_zero_19_materials.inc"
};
static Sint16 event_zero_19_meshes0[28] = {
#include "shared/zero/event_zero_19_meshes0.inc"
};
static Sint16 event_zero_19_meshes1[7] = {
#include "shared/zero/event_zero_19_meshes1.inc"
};
static NJS_COLOR event_zero_19_uvs0[24] = {
#include "shared/zero/event_zero_19_uvs0.inc"
};
static NJS_COLOR event_zero_19_uvs1[6] = {
#include "shared/zero/event_zero_19_uvs1.inc"
};
static NJS_MESHSET event_zero_19_meshsets[2] = {
#include "shared/zero/event_zero_19_meshsets.inc"
};
static NJS_POINT3 event_zero_19_points[12] = {
#include "shared/zero/event_zero_19_points.inc"
};
static NJS_VECTOR event_zero_19_normals[12] = {
#include "shared/zero/event_zero_19_normals.inc"
};
static NJS_MODEL event_zero_19_model = {
#include "shared/zero/event_zero_19_model.inc"
};
static NJS_OBJECT event_zero_19 = {
#include "shared/zero/event_zero_19.inc"
};
static NJS_MATERIAL event_zero_18_materials[2] = {
#include "shared/zero/event_zero_18_materials.inc"
};
static Sint16 event_zero_18_meshes0[21] = {
#include "shared/zero/event_zero_18_meshes0.inc"
};
static Sint16 event_zero_18_meshes1[17] = {
#include "shared/zero/event_zero_18_meshes1.inc"
};
static NJS_COLOR event_zero_18_uvs0[18] = {
#include "shared/zero/event_zero_18_uvs0.inc"
};
static NJS_COLOR event_zero_18_uvs1[14] = {
#include "shared/zero/event_zero_18_uvs1.inc"
};
static NJS_MESHSET event_zero_18_meshsets[2] = {
#include "shared/zero/event_zero_18_meshsets.inc"
};
static NJS_POINT3 event_zero_18_points[14] = {
#include "shared/zero/event_zero_18_points.inc"
};
static NJS_VECTOR event_zero_18_normals[14] = {
#include "shared/zero/event_zero_18_normals.inc"
};
static NJS_MODEL event_zero_18_model = {
#include "shared/zero/event_zero_18_model.inc"
};
static NJS_OBJECT event_zero_18 = {
#include "shared/zero/event_zero_18.inc"
};
static NJS_OBJECT event_zero_17 = {
#include "shared/zero/event_zero_17.inc"
};
static NJS_OBJECT event_zero_16 = {
#include "shared/zero/event_zero_16.inc"
};
static NJS_OBJECT event_zero_15 = {
#include "shared/zero/event_zero_15.inc"
};
static NJS_OBJECT event_zero_14 = {
#include "shared/zero/event_zero_14.inc"
};
static NJS_OBJECT event_zero_13 = {
#include "shared/zero/event_zero_13.inc"
};
static NJS_MATERIAL event_zero_12_materials[16] = {
#include "shared/zero/event_zero_12_materials.inc"
};
static Sint16 event_zero_12_meshes0[71] = {
#include "shared/zero/event_zero_12_meshes0.inc"
};
static Sint16 event_zero_12_meshes1[7] = {
#include "shared/zero/event_zero_12_meshes1.inc"
};
static Sint16 event_zero_12_meshes2[30] = {
#include "shared/zero/event_zero_12_meshes2.inc"
};
static Sint16 event_zero_12_meshes3[38] = {
#include "shared/zero/event_zero_12_meshes3.inc"
};
static Sint16 event_zero_12_meshes4[10] = {
#include "shared/zero/event_zero_12_meshes4.inc"
};
static Sint16 event_zero_12_meshes5[83] = {
#include "shared/zero/event_zero_12_meshes5.inc"
};
static Sint16 event_zero_12_meshes6[11] = {
#include "shared/zero/event_zero_12_meshes6.inc"
};
static Sint16 event_zero_12_meshes7[40] = {
#include "shared/zero/event_zero_12_meshes7.inc"
};
static Sint16 event_zero_12_meshes8[17] = {
#include "shared/zero/event_zero_12_meshes8.inc"
};
static Sint16 event_zero_12_meshes9[10] = {
#include "shared/zero/event_zero_12_meshes9.inc"
};
static Sint16 event_zero_12_meshes10[11] = {
#include "shared/zero/event_zero_12_meshes10.inc"
};
static Sint16 event_zero_12_meshes11[28] = {
#include "shared/zero/event_zero_12_meshes11.inc"
};
static Sint16 event_zero_12_meshes12[50] = {
#include "shared/zero/event_zero_12_meshes12.inc"
};
static Sint16 event_zero_12_meshes13[10] = {
#include "shared/zero/event_zero_12_meshes13.inc"
};
static Sint16 event_zero_12_meshes14[70] = {
#include "shared/zero/event_zero_12_meshes14.inc"
};
static Sint16 event_zero_12_meshes15[25] = {
#include "shared/zero/event_zero_12_meshes15.inc"
};
static NJS_COLOR event_zero_12_uvs0[62] = {
#include "shared/zero/event_zero_12_uvs0.inc"
};
static NJS_COLOR event_zero_12_uvs2[24] = {
#include "shared/zero/event_zero_12_uvs2.inc"
};
static NJS_COLOR event_zero_12_uvs3[30] = {
#include "shared/zero/event_zero_12_uvs3.inc"
};
static NJS_COLOR event_zero_12_uvs4[8] = {
#include "shared/zero/event_zero_12_uvs4.inc"
};
static NJS_COLOR event_zero_12_uvs5[70] = {
#include "shared/zero/event_zero_12_uvs5.inc"
};
static NJS_COLOR event_zero_12_uvs6[10] = {
#include "shared/zero/event_zero_12_uvs6.inc"
};
static NJS_COLOR event_zero_12_uvs7[32] = {
#include "shared/zero/event_zero_12_uvs7.inc"
};
static NJS_COLOR event_zero_12_uvs8[14] = {
#include "shared/zero/event_zero_12_uvs8.inc"
};
static NJS_COLOR event_zero_12_uvs9[8] = {
#include "shared/zero/event_zero_12_uvs9.inc"
};
static NJS_COLOR event_zero_12_uvs10[9] = {
#include "shared/zero/event_zero_12_uvs10.inc"
};
static NJS_COLOR event_zero_12_uvs12[40] = {
#include "shared/zero/event_zero_12_uvs12.inc"
};
static NJS_COLOR event_zero_12_uvs13[8] = {
#include "shared/zero/event_zero_12_uvs13.inc"
};
static NJS_COLOR event_zero_12_uvs14[56] = {
#include "shared/zero/event_zero_12_uvs14.inc"
};
static NJS_COLOR event_zero_12_uvs15[20] = {
#include "shared/zero/event_zero_12_uvs15.inc"
};
static NJS_MESHSET event_zero_12_meshsets[16] = {
#include "shared/zero/event_zero_12_meshsets.inc"
};
static NJS_POINT3 event_zero_12_points[146] = {
#include "shared/zero/event_zero_12_points.inc"
};
static NJS_VECTOR event_zero_12_normals[146] = {
#include "shared/zero/event_zero_12_normals.inc"
};
static NJS_MODEL event_zero_12_model = {
#include "shared/zero/event_zero_12_model.inc"
};
static NJS_OBJECT event_zero_12 = {
#include "shared/zero/event_zero_12.inc"
};
static NJS_OBJECT event_zero_11 = {
#include "shared/zero/event_zero_11.inc"
};
static NJS_OBJECT event_zero_10 = {
#include "shared/zero/event_zero_10.inc"
};
static NJS_OBJECT event_zero_9 = {
#include "shared/zero/event_zero_9.inc"
};
static NJS_MATERIAL event_zero_8_materials[6] = {
#include "shared/zero/event_zero_8_materials.inc"
};
static Sint16 event_zero_8_meshes0[5] = {
#include "shared/zero/event_zero_8_meshes0.inc"
};
static Sint16 event_zero_8_meshes1[31] = {
#include "shared/zero/event_zero_8_meshes1.inc"
};
static Sint16 event_zero_8_meshes2[20] = {
#include "shared/zero/event_zero_8_meshes2.inc"
};
static Sint16 event_zero_8_meshes3[20] = {
#include "shared/zero/event_zero_8_meshes3.inc"
};
static Sint16 event_zero_8_meshes4[12] = {
#include "shared/zero/event_zero_8_meshes4.inc"
};
static Sint16 event_zero_8_meshes5[5] = {
#include "shared/zero/event_zero_8_meshes5.inc"
};
static NJS_COLOR event_zero_8_uvs0[4] = {
#include "shared/zero/event_zero_8_uvs0.inc"
};
static NJS_COLOR event_zero_8_uvs1[28] = {
#include "shared/zero/event_zero_8_uvs1.inc"
};
static NJS_COLOR event_zero_8_uvs2[16] = {
#include "shared/zero/event_zero_8_uvs2.inc"
};
static NJS_COLOR event_zero_8_uvs3[16] = {
#include "shared/zero/event_zero_8_uvs3.inc"
};
static NJS_COLOR event_zero_8_uvs4[10] = {
#include "shared/zero/event_zero_8_uvs4.inc"
};
static NJS_COLOR event_zero_8_uvs5[4] = {
#include "shared/zero/event_zero_8_uvs5.inc"
};
static NJS_MESHSET event_zero_8_meshsets[6] = {
#include "shared/zero/event_zero_8_meshsets.inc"
};
static NJS_POINT3 event_zero_8_points[38] = {
#include "shared/zero/event_zero_8_points.inc"
};
static NJS_VECTOR event_zero_8_normals[38] = {
#include "shared/zero/event_zero_8_normals.inc"
};
static NJS_MODEL event_zero_8_model = {
#include "shared/zero/event_zero_8_model.inc"
};
static NJS_OBJECT event_zero_8 = {
#include "shared/zero/event_zero_8.inc"
};
static NJS_MATERIAL event_zero_7_materials[6] = {
#include "shared/zero/event_zero_7_materials.inc"
};
static Sint16 event_zero_7_meshes0[5] = {
#include "shared/zero/event_zero_7_meshes0.inc"
};
static Sint16 event_zero_7_meshes1[34] = {
#include "shared/zero/event_zero_7_meshes1.inc"
};
static Sint16 event_zero_7_meshes2[20] = {
#include "shared/zero/event_zero_7_meshes2.inc"
};
static Sint16 event_zero_7_meshes3[20] = {
#include "shared/zero/event_zero_7_meshes3.inc"
};
static Sint16 event_zero_7_meshes4[12] = {
#include "shared/zero/event_zero_7_meshes4.inc"
};
static Sint16 event_zero_7_meshes5[5] = {
#include "shared/zero/event_zero_7_meshes5.inc"
};
static NJS_COLOR event_zero_7_uvs0[4] = {
#include "shared/zero/event_zero_7_uvs0.inc"
};
static NJS_COLOR event_zero_7_uvs1[30] = {
#include "shared/zero/event_zero_7_uvs1.inc"
};
static NJS_COLOR event_zero_7_uvs2[16] = {
#include "shared/zero/event_zero_7_uvs2.inc"
};
static NJS_COLOR event_zero_7_uvs3[16] = {
#include "shared/zero/event_zero_7_uvs3.inc"
};
static NJS_COLOR event_zero_7_uvs4[10] = {
#include "shared/zero/event_zero_7_uvs4.inc"
};
static NJS_COLOR event_zero_7_uvs5[4] = {
#include "shared/zero/event_zero_7_uvs5.inc"
};
static NJS_MESHSET event_zero_7_meshsets[6] = {
#include "shared/zero/event_zero_7_meshsets.inc"
};
static NJS_POINT3 event_zero_7_points[38] = {
#include "shared/zero/event_zero_7_points.inc"
};
static NJS_VECTOR event_zero_7_normals[38] = {
#include "shared/zero/event_zero_7_normals.inc"
};
static NJS_MODEL event_zero_7_model = {
#include "shared/zero/event_zero_7_model.inc"
};
static NJS_OBJECT event_zero_7 = {
#include "shared/zero/event_zero_7.inc"
};
static NJS_MATERIAL event_zero_6_materials[2] = {
#include "shared/zero/event_zero_6_materials.inc"
};
static Sint16 event_zero_6_meshes0[80] = {
#include "shared/zero/event_zero_6_meshes0.inc"
};
static Sint16 event_zero_6_meshes1[35] = {
#include "shared/zero/event_zero_6_meshes1.inc"
};
static NJS_COLOR event_zero_6_uvs0[72] = {
#include "shared/zero/event_zero_6_uvs0.inc"
};
static NJS_COLOR event_zero_6_uvs1[30] = {
#include "shared/zero/event_zero_6_uvs1.inc"
};
static NJS_MESHSET event_zero_6_meshsets[2] = {
#include "shared/zero/event_zero_6_meshsets.inc"
};
static NJS_POINT3 event_zero_6_points[48] = {
#include "shared/zero/event_zero_6_points.inc"
};
static NJS_VECTOR event_zero_6_normals[48] = {
#include "shared/zero/event_zero_6_normals.inc"
};
static NJS_MODEL event_zero_6_model = {
#include "shared/zero/event_zero_6_model.inc"
};
static NJS_OBJECT event_zero_6 = {
#include "shared/zero/event_zero_6.inc"
};
static NJS_MATERIAL event_zero_5_materials[6] = {
#include "shared/zero/event_zero_5_materials.inc"
};
static Sint16 event_zero_5_meshes0[12] = {
#include "shared/zero/event_zero_5_meshes0.inc"
};
static Sint16 event_zero_5_meshes1[17] = {
#include "shared/zero/event_zero_5_meshes1.inc"
};
static Sint16 event_zero_5_meshes2[25] = {
#include "shared/zero/event_zero_5_meshes2.inc"
};
static Sint16 event_zero_5_meshes3[62] = {
#include "shared/zero/event_zero_5_meshes3.inc"
};
static Sint16 event_zero_5_meshes4[15] = {
#include "shared/zero/event_zero_5_meshes4.inc"
};
static Sint16 event_zero_5_meshes5[5] = {
#include "shared/zero/event_zero_5_meshes5.inc"
};
static NJS_COLOR event_zero_5_uvs1[14] = {
#include "shared/zero/event_zero_5_uvs1.inc"
};
static NJS_COLOR event_zero_5_uvs2[20] = {
#include "shared/zero/event_zero_5_uvs2.inc"
};
static NJS_COLOR event_zero_5_uvs3[48] = {
#include "shared/zero/event_zero_5_uvs3.inc"
};
static NJS_COLOR event_zero_5_uvs4[12] = {
#include "shared/zero/event_zero_5_uvs4.inc"
};
static NJS_COLOR event_zero_5_uvs5[4] = {
#include "shared/zero/event_zero_5_uvs5.inc"
};
static NJS_MESHSET event_zero_5_meshsets[6] = {
#include "shared/zero/event_zero_5_meshsets.inc"
};
static NJS_POINT3 event_zero_5_points[36] = {
#include "shared/zero/event_zero_5_points.inc"
};
static NJS_VECTOR event_zero_5_normals[36] = {
#include "shared/zero/event_zero_5_normals.inc"
};
static NJS_MODEL event_zero_5_model = {
#include "shared/zero/event_zero_5_model.inc"
};
static NJS_OBJECT event_zero_5 = {
#include "shared/zero/event_zero_5.inc"
};
static NJS_MATERIAL event_zero_4_materials[5] = {
#include "shared/zero/event_zero_4_materials.inc"
};
static Sint16 event_zero_4_meshes0[20] = {
#include "shared/zero/event_zero_4_meshes0.inc"
};
static Sint16 event_zero_4_meshes1[10] = {
#include "shared/zero/event_zero_4_meshes1.inc"
};
static Sint16 event_zero_4_meshes2[5] = {
#include "shared/zero/event_zero_4_meshes2.inc"
};
static Sint16 event_zero_4_meshes3[22] = {
#include "shared/zero/event_zero_4_meshes3.inc"
};
static Sint16 event_zero_4_meshes4[5] = {
#include "shared/zero/event_zero_4_meshes4.inc"
};
static NJS_COLOR event_zero_4_uvs0[16] = {
#include "shared/zero/event_zero_4_uvs0.inc"
};
static NJS_COLOR event_zero_4_uvs1[8] = {
#include "shared/zero/event_zero_4_uvs1.inc"
};
static NJS_COLOR event_zero_4_uvs2[4] = {
#include "shared/zero/event_zero_4_uvs2.inc"
};
static NJS_COLOR event_zero_4_uvs3[18] = {
#include "shared/zero/event_zero_4_uvs3.inc"
};
static NJS_COLOR event_zero_4_uvs4[4] = {
#include "shared/zero/event_zero_4_uvs4.inc"
};
static NJS_MESHSET event_zero_4_meshsets[5] = {
#include "shared/zero/event_zero_4_meshsets.inc"
};
static NJS_POINT3 event_zero_4_points[16] = {
#include "shared/zero/event_zero_4_points.inc"
};
static NJS_VECTOR event_zero_4_normals[16] = {
#include "shared/zero/event_zero_4_normals.inc"
};
static NJS_MODEL event_zero_4_model = {
#include "shared/zero/event_zero_4_model.inc"
};
static NJS_OBJECT event_zero_4 = {
#include "shared/zero/event_zero_4.inc"
};
static NJS_MATERIAL event_zero_3_materials[2] = {
#include "shared/zero/event_zero_3_materials.inc"
};
static Sint16 event_zero_3_meshes0[25] = {
#include "shared/zero/event_zero_3_meshes0.inc"
};
static Sint16 event_zero_3_meshes1[5] = {
#include "shared/zero/event_zero_3_meshes1.inc"
};
static NJS_COLOR event_zero_3_uvs0[20] = {
#include "shared/zero/event_zero_3_uvs0.inc"
};
static NJS_COLOR event_zero_3_uvs1[4] = {
#include "shared/zero/event_zero_3_uvs1.inc"
};
static NJS_MESHSET event_zero_3_meshsets[2] = {
#include "shared/zero/event_zero_3_meshsets.inc"
};
static NJS_POINT3 event_zero_3_points[8] = {
#include "shared/zero/event_zero_3_points.inc"
};
static NJS_VECTOR event_zero_3_normals[8] = {
#include "shared/zero/event_zero_3_normals.inc"
};
static NJS_MODEL event_zero_3_model = {
#include "shared/zero/event_zero_3_model.inc"
};
static NJS_OBJECT event_zero_3 = {
#include "shared/zero/event_zero_3.inc"
};
static NJS_OBJECT event_zero_2 = {
#include "shared/zero/event_zero_2.inc"
};
static NJS_OBJECT event_zero_1 = {
#include "shared/zero/event_zero_1.inc"
};
static NJS_OBJECT event_zero_object = {
#include "shared/zero/event_zero_object.inc"
};
static NJS_MATERIAL event_zero_bird_56_materials[1] = {
#include "shared/zero/event_zero_bird_56_materials.inc"
};
static Sint16 event_zero_bird_56_meshes0[5] = {
#include "shared/zero/event_zero_bird_56_meshes0.inc"
};
static NJS_COLOR event_zero_bird_56_uvs0[4] = {
#include "shared/zero/event_zero_bird_56_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_56_meshsets[1] = {
#include "shared/zero/event_zero_bird_56_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_56_points[4] = {
#include "shared/zero/event_zero_bird_56_points.inc"
};
static NJS_VECTOR event_zero_bird_56_normals[4] = {
#include "shared/zero/event_zero_bird_56_normals.inc"
};
static NJS_MODEL event_zero_bird_56_model = {
#include "shared/zero/event_zero_bird_56_model.inc"
};
static NJS_OBJECT event_zero_bird_56 = {
#include "shared/zero/event_zero_bird_56.inc"
};
static NJS_MATERIAL event_zero_bird_55_materials[1] = {
#include "shared/zero/event_zero_bird_55_materials.inc"
};
static Sint16 event_zero_bird_55_meshes0[11] = {
#include "shared/zero/event_zero_bird_55_meshes0.inc"
};
static NJS_COLOR event_zero_bird_55_uvs0[9] = {
#include "shared/zero/event_zero_bird_55_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_55_meshsets[1] = {
#include "shared/zero/event_zero_bird_55_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_55_points[7] = {
#include "shared/zero/event_zero_bird_55_points.inc"
};
static NJS_VECTOR event_zero_bird_55_normals[7] = {
#include "shared/zero/event_zero_bird_55_normals.inc"
};
static NJS_MODEL event_zero_bird_55_model = {
#include "shared/zero/event_zero_bird_55_model.inc"
};
static NJS_OBJECT event_zero_bird_55 = {
#include "shared/zero/event_zero_bird_55.inc"
};
static NJS_OBJECT event_zero_bird_54 = {
#include "shared/zero/event_zero_bird_54.inc"
};
static NJS_OBJECT event_zero_bird_53 = {
#include "shared/zero/event_zero_bird_53.inc"
};
static NJS_OBJECT event_zero_bird_52 = {
#include "shared/zero/event_zero_bird_52.inc"
};
static NJS_OBJECT event_zero_bird_51 = {
#include "shared/zero/event_zero_bird_51.inc"
};
static NJS_OBJECT event_zero_bird_50 = {
#include "shared/zero/event_zero_bird_50.inc"
};
static NJS_OBJECT event_zero_bird_49 = {
#include "shared/zero/event_zero_bird_49.inc"
};
static NJS_MATERIAL event_zero_bird_48_materials[1] = {
#include "shared/zero/event_zero_bird_48_materials.inc"
};
static Sint16 event_zero_bird_48_meshes0[5] = {
#include "shared/zero/event_zero_bird_48_meshes0.inc"
};
static NJS_COLOR event_zero_bird_48_uvs0[4] = {
#include "shared/zero/event_zero_bird_48_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_48_meshsets[1] = {
#include "shared/zero/event_zero_bird_48_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_48_points[4] = {
#include "shared/zero/event_zero_bird_48_points.inc"
};
static NJS_VECTOR event_zero_bird_48_normals[4] = {
#include "shared/zero/event_zero_bird_48_normals.inc"
};
static NJS_MODEL event_zero_bird_48_model = {
#include "shared/zero/event_zero_bird_48_model.inc"
};
static NJS_OBJECT event_zero_bird_48 = {
#include "shared/zero/event_zero_bird_48.inc"
};
static NJS_MATERIAL event_zero_bird_47_materials[1] = {
#include "shared/zero/event_zero_bird_47_materials.inc"
};
static Sint16 event_zero_bird_47_meshes0[11] = {
#include "shared/zero/event_zero_bird_47_meshes0.inc"
};
static NJS_COLOR event_zero_bird_47_uvs0[9] = {
#include "shared/zero/event_zero_bird_47_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_47_meshsets[1] = {
#include "shared/zero/event_zero_bird_47_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_47_points[7] = {
#include "shared/zero/event_zero_bird_47_points.inc"
};
static NJS_VECTOR event_zero_bird_47_normals[7] = {
#include "shared/zero/event_zero_bird_47_normals.inc"
};
static NJS_MODEL event_zero_bird_47_model = {
#include "shared/zero/event_zero_bird_47_model.inc"
};
static NJS_OBJECT event_zero_bird_47 = {
#include "shared/zero/event_zero_bird_47.inc"
};
static NJS_OBJECT event_zero_bird_46 = {
#include "shared/zero/event_zero_bird_46.inc"
};
static NJS_OBJECT event_zero_bird_45 = {
#include "shared/zero/event_zero_bird_45.inc"
};
static NJS_OBJECT event_zero_bird_44 = {
#include "shared/zero/event_zero_bird_44.inc"
};
static NJS_OBJECT event_zero_bird_43 = {
#include "shared/zero/event_zero_bird_43.inc"
};
static NJS_OBJECT event_zero_bird_42 = {
#include "shared/zero/event_zero_bird_42.inc"
};
static NJS_OBJECT event_zero_bird_41 = {
#include "shared/zero/event_zero_bird_41.inc"
};
static NJS_OBJECT event_zero_bird_40 = {
#include "shared/zero/event_zero_bird_40.inc"
};
static NJS_OBJECT event_zero_bird_39 = {
#include "shared/zero/event_zero_bird_39.inc"
};
static NJS_OBJECT event_zero_bird_38 = {
#include "shared/zero/event_zero_bird_38.inc"
};
static NJS_OBJECT event_zero_bird_37 = {
#include "shared/zero/event_zero_bird_37.inc"
};
static NJS_OBJECT event_zero_bird_36 = {
#include "shared/zero/event_zero_bird_36.inc"
};
static NJS_MATERIAL event_zero_bird_35_materials[1] = {
#include "shared/zero/event_zero_bird_35_materials.inc"
};
static Sint16 event_zero_bird_35_meshes0[18] = {
#include "shared/zero/event_zero_bird_35_meshes0.inc"
};
static NJS_COLOR event_zero_bird_35_uvs0[15] = {
#include "shared/zero/event_zero_bird_35_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_35_meshsets[1] = {
#include "shared/zero/event_zero_bird_35_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_35_points[7] = {
#include "shared/zero/event_zero_bird_35_points.inc"
};
static NJS_VECTOR event_zero_bird_35_normals[7] = {
#include "shared/zero/event_zero_bird_35_normals.inc"
};
static NJS_MODEL event_zero_bird_35_model = {
#include "shared/zero/event_zero_bird_35_model.inc"
};
static NJS_OBJECT event_zero_bird_35 = {
#include "shared/zero/event_zero_bird_35.inc"
};
static NJS_OBJECT event_zero_bird_34 = {
#include "shared/zero/event_zero_bird_34.inc"
};
static NJS_OBJECT event_zero_bird_33 = {
#include "shared/zero/event_zero_bird_33.inc"
};
static NJS_OBJECT event_zero_bird_32 = {
#include "shared/zero/event_zero_bird_32.inc"
};
static NJS_MATERIAL event_zero_bird_31_materials[1] = {
#include "shared/zero/event_zero_bird_31_materials.inc"
};
static Sint16 event_zero_bird_31_meshes0[9] = {
#include "shared/zero/event_zero_bird_31_meshes0.inc"
};
static NJS_COLOR event_zero_bird_31_uvs0[8] = {
#include "shared/zero/event_zero_bird_31_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_31_meshsets[1] = {
#include "shared/zero/event_zero_bird_31_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_31_points[8] = {
#include "shared/zero/event_zero_bird_31_points.inc"
};
static NJS_VECTOR event_zero_bird_31_normals[8] = {
#include "shared/zero/event_zero_bird_31_normals.inc"
};
static NJS_MODEL event_zero_bird_31_model = {
#include "shared/zero/event_zero_bird_31_model.inc"
};
static NJS_OBJECT event_zero_bird_31 = {
#include "shared/zero/event_zero_bird_31.inc"
};
static NJS_MATERIAL event_zero_bird_30_materials[1] = {
#include "shared/zero/event_zero_bird_30_materials.inc"
};
static Sint16 event_zero_bird_30_meshes0[9] = {
#include "shared/zero/event_zero_bird_30_meshes0.inc"
};
static NJS_COLOR event_zero_bird_30_uvs0[8] = {
#include "shared/zero/event_zero_bird_30_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_30_meshsets[1] = {
#include "shared/zero/event_zero_bird_30_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_30_points[8] = {
#include "shared/zero/event_zero_bird_30_points.inc"
};
static NJS_VECTOR event_zero_bird_30_normals[8] = {
#include "shared/zero/event_zero_bird_30_normals.inc"
};
static NJS_MODEL event_zero_bird_30_model = {
#include "shared/zero/event_zero_bird_30_model.inc"
};
static NJS_OBJECT event_zero_bird_30 = {
#include "shared/zero/event_zero_bird_30.inc"
};
static NJS_MATERIAL event_zero_bird_29_materials[1] = {
#include "shared/zero/event_zero_bird_29_materials.inc"
};
static Sint16 event_zero_bird_29_meshes0[15] = {
#include "shared/zero/event_zero_bird_29_meshes0.inc"
};
static NJS_COLOR event_zero_bird_29_uvs0[12] = {
#include "shared/zero/event_zero_bird_29_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_29_meshsets[1] = {
#include "shared/zero/event_zero_bird_29_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_29_points[12] = {
#include "shared/zero/event_zero_bird_29_points.inc"
};
static NJS_VECTOR event_zero_bird_29_normals[12] = {
#include "shared/zero/event_zero_bird_29_normals.inc"
};
static NJS_MODEL event_zero_bird_29_model = {
#include "shared/zero/event_zero_bird_29_model.inc"
};
static NJS_OBJECT event_zero_bird_29 = {
#include "shared/zero/event_zero_bird_29.inc"
};
static NJS_MATERIAL event_zero_bird_28_materials[1] = {
#include "shared/zero/event_zero_bird_28_materials.inc"
};
static Sint16 event_zero_bird_28_meshes0[15] = {
#include "shared/zero/event_zero_bird_28_meshes0.inc"
};
static NJS_COLOR event_zero_bird_28_uvs0[12] = {
#include "shared/zero/event_zero_bird_28_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_28_meshsets[1] = {
#include "shared/zero/event_zero_bird_28_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_28_points[12] = {
#include "shared/zero/event_zero_bird_28_points.inc"
};
static NJS_VECTOR event_zero_bird_28_normals[12] = {
#include "shared/zero/event_zero_bird_28_normals.inc"
};
static NJS_MODEL event_zero_bird_28_model = {
#include "shared/zero/event_zero_bird_28_model.inc"
};
static NJS_OBJECT event_zero_bird_28 = {
#include "shared/zero/event_zero_bird_28.inc"
};
static NJS_MATERIAL event_zero_bird_27_materials[4] = {
#include "shared/zero/event_zero_bird_27_materials.inc"
};
static Sint16 event_zero_bird_27_meshes0[239] = {
#include "shared/zero/event_zero_bird_27_meshes0.inc"
};
static Sint16 event_zero_bird_27_meshes1[22] = {
#include "shared/zero/event_zero_bird_27_meshes1.inc"
};
static Sint16 event_zero_bird_27_meshes2[21] = {
#include "shared/zero/event_zero_bird_27_meshes2.inc"
};
static Sint16 event_zero_bird_27_meshes3[21] = {
#include "shared/zero/event_zero_bird_27_meshes3.inc"
};
static NJS_COLOR event_zero_bird_27_uvs0[209] = {
#include "shared/zero/event_zero_bird_27_uvs0.inc"
};
static NJS_COLOR event_zero_bird_27_uvs1[20] = {
#include "shared/zero/event_zero_bird_27_uvs1.inc"
};
static NJS_COLOR event_zero_bird_27_uvs2[16] = {
#include "shared/zero/event_zero_bird_27_uvs2.inc"
};
static NJS_COLOR event_zero_bird_27_uvs3[18] = {
#include "shared/zero/event_zero_bird_27_uvs3.inc"
};
static NJS_MESHSET event_zero_bird_27_meshsets[4] = {
#include "shared/zero/event_zero_bird_27_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_27_points[127] = {
#include "shared/zero/event_zero_bird_27_points.inc"
};
static NJS_VECTOR event_zero_bird_27_normals[127] = {
#include "shared/zero/event_zero_bird_27_normals.inc"
};
static NJS_MODEL event_zero_bird_27_model = {
#include "shared/zero/event_zero_bird_27_model.inc"
};
static NJS_OBJECT event_zero_bird_27 = {
#include "shared/zero/event_zero_bird_27.inc"
};
static NJS_OBJECT event_zero_bird_26 = {
#include "shared/zero/event_zero_bird_26.inc"
};
static NJS_OBJECT event_zero_bird_25 = {
#include "shared/zero/event_zero_bird_25.inc"
};
static NJS_OBJECT event_zero_bird_24 = {
#include "shared/zero/event_zero_bird_24.inc"
};
static NJS_MATERIAL event_zero_bird_23_materials[1] = {
#include "shared/zero/event_zero_bird_23_materials.inc"
};
static Sint16 event_zero_bird_23_meshes0[8] = {
#include "shared/zero/event_zero_bird_23_meshes0.inc"
};
static NJS_COLOR event_zero_bird_23_uvs0[6] = {
#include "shared/zero/event_zero_bird_23_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_23_meshsets[1] = {
#include "shared/zero/event_zero_bird_23_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_23_points[6] = {
#include "shared/zero/event_zero_bird_23_points.inc"
};
static NJS_VECTOR event_zero_bird_23_normals[6] = {
#include "shared/zero/event_zero_bird_23_normals.inc"
};
static NJS_MODEL event_zero_bird_23_model = {
#include "shared/zero/event_zero_bird_23_model.inc"
};
static NJS_OBJECT event_zero_bird_23 = {
#include "shared/zero/event_zero_bird_23.inc"
};
static NJS_MATERIAL event_zero_bird_22_materials[1] = {
#include "shared/zero/event_zero_bird_22_materials.inc"
};
static Sint16 event_zero_bird_22_meshes0[21] = {
#include "shared/zero/event_zero_bird_22_meshes0.inc"
};
static NJS_COLOR event_zero_bird_22_uvs0[16] = {
#include "shared/zero/event_zero_bird_22_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_22_meshsets[1] = {
#include "shared/zero/event_zero_bird_22_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_22_points[5] = {
#include "shared/zero/event_zero_bird_22_points.inc"
};
static NJS_VECTOR event_zero_bird_22_normals[5] = {
#include "shared/zero/event_zero_bird_22_normals.inc"
};
static NJS_MODEL event_zero_bird_22_model = {
#include "shared/zero/event_zero_bird_22_model.inc"
};
static NJS_OBJECT event_zero_bird_22 = {
#include "shared/zero/event_zero_bird_22.inc"
};
static NJS_MATERIAL event_zero_bird_21_materials[1] = {
#include "shared/zero/event_zero_bird_21_materials.inc"
};
static Sint16 event_zero_bird_21_meshes0[30] = {
#include "shared/zero/event_zero_bird_21_meshes0.inc"
};
static NJS_COLOR event_zero_bird_21_uvs0[28] = {
#include "shared/zero/event_zero_bird_21_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_21_meshsets[1] = {
#include "shared/zero/event_zero_bird_21_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_21_points[18] = {
#include "shared/zero/event_zero_bird_21_points.inc"
};
static NJS_VECTOR event_zero_bird_21_normals[18] = {
#include "shared/zero/event_zero_bird_21_normals.inc"
};
static NJS_MODEL event_zero_bird_21_model = {
#include "shared/zero/event_zero_bird_21_model.inc"
};
static NJS_OBJECT event_zero_bird_21 = {
#include "shared/zero/event_zero_bird_21.inc"
};
static NJS_MATERIAL event_zero_bird_20_materials[1] = {
#include "shared/zero/event_zero_bird_20_materials.inc"
};
static Sint16 event_zero_bird_20_meshes0[20] = {
#include "shared/zero/event_zero_bird_20_meshes0.inc"
};
static NJS_COLOR event_zero_bird_20_uvs0[16] = {
#include "shared/zero/event_zero_bird_20_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_20_meshsets[1] = {
#include "shared/zero/event_zero_bird_20_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_20_points[12] = {
#include "shared/zero/event_zero_bird_20_points.inc"
};
static NJS_VECTOR event_zero_bird_20_normals[12] = {
#include "shared/zero/event_zero_bird_20_normals.inc"
};
static NJS_MODEL event_zero_bird_20_model = {
#include "shared/zero/event_zero_bird_20_model.inc"
};
static NJS_OBJECT event_zero_bird_20 = {
#include "shared/zero/event_zero_bird_20.inc"
};
static NJS_MATERIAL event_zero_bird_19_materials[1] = {
#include "shared/zero/event_zero_bird_19_materials.inc"
};
static Sint16 event_zero_bird_19_meshes0[10] = {
#include "shared/zero/event_zero_bird_19_meshes0.inc"
};
static NJS_COLOR event_zero_bird_19_uvs0[8] = {
#include "shared/zero/event_zero_bird_19_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_19_meshsets[1] = {
#include "shared/zero/event_zero_bird_19_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_19_points[5] = {
#include "shared/zero/event_zero_bird_19_points.inc"
};
static NJS_VECTOR event_zero_bird_19_normals[5] = {
#include "shared/zero/event_zero_bird_19_normals.inc"
};
static NJS_MODEL event_zero_bird_19_model = {
#include "shared/zero/event_zero_bird_19_model.inc"
};
static NJS_OBJECT event_zero_bird_19 = {
#include "shared/zero/event_zero_bird_19.inc"
};
static NJS_MATERIAL event_zero_bird_18_materials[1] = {
#include "shared/zero/event_zero_bird_18_materials.inc"
};
static Sint16 event_zero_bird_18_meshes0[10] = {
#include "shared/zero/event_zero_bird_18_meshes0.inc"
};
static NJS_COLOR event_zero_bird_18_uvs0[8] = {
#include "shared/zero/event_zero_bird_18_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_18_meshsets[1] = {
#include "shared/zero/event_zero_bird_18_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_18_points[5] = {
#include "shared/zero/event_zero_bird_18_points.inc"
};
static NJS_VECTOR event_zero_bird_18_normals[5] = {
#include "shared/zero/event_zero_bird_18_normals.inc"
};
static NJS_MODEL event_zero_bird_18_model = {
#include "shared/zero/event_zero_bird_18_model.inc"
};
static NJS_OBJECT event_zero_bird_18 = {
#include "shared/zero/event_zero_bird_18.inc"
};
static NJS_MATERIAL event_zero_bird_17_materials[1] = {
#include "shared/zero/event_zero_bird_17_materials.inc"
};
static Sint16 event_zero_bird_17_meshes0[23] = {
#include "shared/zero/event_zero_bird_17_meshes0.inc"
};
static NJS_COLOR event_zero_bird_17_uvs0[20] = {
#include "shared/zero/event_zero_bird_17_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_17_meshsets[1] = {
#include "shared/zero/event_zero_bird_17_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_17_points[12] = {
#include "shared/zero/event_zero_bird_17_points.inc"
};
static NJS_VECTOR event_zero_bird_17_normals[12] = {
#include "shared/zero/event_zero_bird_17_normals.inc"
};
static NJS_MODEL event_zero_bird_17_model = {
#include "shared/zero/event_zero_bird_17_model.inc"
};
static NJS_OBJECT event_zero_bird_17 = {
#include "shared/zero/event_zero_bird_17.inc"
};
static NJS_MATERIAL event_zero_bird_16_materials[1] = {
#include "shared/zero/event_zero_bird_16_materials.inc"
};
static Sint16 event_zero_bird_16_meshes0[35] = {
#include "shared/zero/event_zero_bird_16_meshes0.inc"
};
static NJS_COLOR event_zero_bird_16_uvs0[30] = {
#include "shared/zero/event_zero_bird_16_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_16_meshsets[1] = {
#include "shared/zero/event_zero_bird_16_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_16_points[12] = {
#include "shared/zero/event_zero_bird_16_points.inc"
};
static NJS_VECTOR event_zero_bird_16_normals[12] = {
#include "shared/zero/event_zero_bird_16_normals.inc"
};
static NJS_MODEL event_zero_bird_16_model = {
#include "shared/zero/event_zero_bird_16_model.inc"
};
static NJS_OBJECT event_zero_bird_16 = {
#include "shared/zero/event_zero_bird_16.inc"
};
static NJS_OBJECT event_zero_bird_15 = {
#include "shared/zero/event_zero_bird_15.inc"
};
static NJS_OBJECT event_zero_bird_14 = {
#include "shared/zero/event_zero_bird_14.inc"
};
static NJS_OBJECT event_zero_bird_13 = {
#include "shared/zero/event_zero_bird_13.inc"
};
static NJS_MATERIAL event_zero_bird_12_materials[1] = {
#include "shared/zero/event_zero_bird_12_materials.inc"
};
static Sint16 event_zero_bird_12_meshes0[47] = {
#include "shared/zero/event_zero_bird_12_meshes0.inc"
};
static NJS_COLOR event_zero_bird_12_uvs0[38] = {
#include "shared/zero/event_zero_bird_12_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_12_meshsets[1] = {
#include "shared/zero/event_zero_bird_12_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_12_points[12] = {
#include "shared/zero/event_zero_bird_12_points.inc"
};
static NJS_VECTOR event_zero_bird_12_normals[12] = {
#include "shared/zero/event_zero_bird_12_normals.inc"
};
static NJS_MODEL event_zero_bird_12_model = {
#include "shared/zero/event_zero_bird_12_model.inc"
};
static NJS_OBJECT event_zero_bird_12 = {
#include "shared/zero/event_zero_bird_12.inc"
};
static NJS_OBJECT event_zero_bird_11 = {
#include "shared/zero/event_zero_bird_11.inc"
};
static NJS_OBJECT event_zero_bird_10 = {
#include "shared/zero/event_zero_bird_10.inc"
};
static NJS_OBJECT event_zero_bird_9 = {
#include "shared/zero/event_zero_bird_9.inc"
};
static NJS_MATERIAL event_zero_bird_8_materials[1] = {
#include "shared/zero/event_zero_bird_8_materials.inc"
};
static Sint16 event_zero_bird_8_meshes0[47] = {
#include "shared/zero/event_zero_bird_8_meshes0.inc"
};
static NJS_COLOR event_zero_bird_8_uvs0[38] = {
#include "shared/zero/event_zero_bird_8_uvs0.inc"
};
static NJS_MESHSET event_zero_bird_8_meshsets[1] = {
#include "shared/zero/event_zero_bird_8_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_8_points[12] = {
#include "shared/zero/event_zero_bird_8_points.inc"
};
static NJS_VECTOR event_zero_bird_8_normals[12] = {
#include "shared/zero/event_zero_bird_8_normals.inc"
};
static NJS_MODEL event_zero_bird_8_model = {
#include "shared/zero/event_zero_bird_8_model.inc"
};
static NJS_OBJECT event_zero_bird_8 = {
#include "shared/zero/event_zero_bird_8.inc"
};
static NJS_OBJECT event_zero_bird_7 = {
#include "shared/zero/event_zero_bird_7.inc"
};
static NJS_OBJECT event_zero_bird_6 = {
#include "shared/zero/event_zero_bird_6.inc"
};
static NJS_OBJECT event_zero_bird_5 = {
#include "shared/zero/event_zero_bird_5.inc"
};
static NJS_MATERIAL event_zero_bird_4_materials[2] = {
#include "shared/zero/event_zero_bird_4_materials.inc"
};
static Sint16 event_zero_bird_4_meshes0[54] = {
#include "shared/zero/event_zero_bird_4_meshes0.inc"
};
static Sint16 event_zero_bird_4_meshes1[21] = {
#include "shared/zero/event_zero_bird_4_meshes1.inc"
};
static NJS_COLOR event_zero_bird_4_uvs0[48] = {
#include "shared/zero/event_zero_bird_4_uvs0.inc"
};
static NJS_COLOR event_zero_bird_4_uvs1[18] = {
#include "shared/zero/event_zero_bird_4_uvs1.inc"
};
static NJS_MESHSET event_zero_bird_4_meshsets[2] = {
#include "shared/zero/event_zero_bird_4_meshsets.inc"
};
static NJS_POINT3 event_zero_bird_4_points[26] = {
#include "shared/zero/event_zero_bird_4_points.inc"
};
static NJS_VECTOR event_zero_bird_4_normals[26] = {
#include "shared/zero/event_zero_bird_4_normals.inc"
};
static NJS_MODEL event_zero_bird_4_model = {
#include "shared/zero/event_zero_bird_4_model.inc"
};
static NJS_OBJECT event_zero_bird_4 = {
#include "shared/zero/event_zero_bird_4.inc"
};
static NJS_OBJECT event_zero_bird_3 = {
#include "shared/zero/event_zero_bird_3.inc"
};
static NJS_OBJECT event_zero_bird_2 = {
#include "shared/zero/event_zero_bird_2.inc"
};
static NJS_OBJECT event_zero_bird_1 = {
#include "shared/zero/event_zero_bird_1.inc"
};
static NJS_OBJECT event_zero_bird_object = {
#include "shared/zero/event_zero_bird_object.inc"
};
static NJS_MKEY_F event_zero_amy_motion_pos1[33] = {
#include "shared/zero/event_zero_amy_motion_pos1.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang1[33] = {
#include "shared/zero/event_zero_amy_motion_ang1.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang12[33] = {
#include "shared/zero/event_zero_amy_motion_ang12.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang38[33] = {
#include "shared/zero/event_zero_amy_motion_ang38.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang39[33] = {
#include "shared/zero/event_zero_amy_motion_ang39.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang40[33] = {
#include "shared/zero/event_zero_amy_motion_ang40.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang41[33] = {
#include "shared/zero/event_zero_amy_motion_ang41.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang50[33] = {
#include "shared/zero/event_zero_amy_motion_ang50.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang55[33] = {
#include "shared/zero/event_zero_amy_motion_ang55.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang60[33] = {
#include "shared/zero/event_zero_amy_motion_ang60.inc"
};
static NJS_MKEY_A event_zero_amy_motion_ang65[33] = {
#include "shared/zero/event_zero_amy_motion_ang65.inc"
};
static NJS_MKEY_F event_zero_amy_motion_pos73[33] = {
#include "shared/zero/event_zero_amy_motion_pos73.inc"
};
static NJS_MKEY_F event_zero_amy_motion_pos74[33] = {
#include "shared/zero/event_zero_amy_motion_pos74.inc"
};
static NJS_MKEY_F event_zero_amy_motion_pos75[33] = {
#include "shared/zero/event_zero_amy_motion_pos75.inc"
};
static NJS_MDATA2 event_zero_amy_motion_mdata[77] = {
#include "shared/zero/event_zero_amy_motion_mdata.inc"
};
static NJS_MOTION event_zero_amy_motion = {
#include "shared/zero/event_zero_amy_motion.inc"
};
static NJS_ACTION event_zero_amy_action = {
#include "shared/zero/event_zero_amy_action.inc"
};
static NJS_MDATA2 event_zero_amy_head_motion_mdata[1] = {
#include "shared/zero/event_zero_amy_head_motion_mdata.inc"
};
static NJS_MOTION event_zero_amy_head_motion = {
#include "shared/zero/event_zero_amy_head_motion.inc"
};
static NJS_ACTION event_zero_amy_head_action = {
#include "shared/zero/event_zero_amy_head_action.inc"
};
static NJS_MKEY_A event_zero_motion_ang1[32] = {
#include "shared/zero/event_zero_motion_ang1.inc"
};
static NJS_MKEY_A event_zero_motion_ang4[32] = {
#include "shared/zero/event_zero_motion_ang4.inc"
};
static NJS_MKEY_A event_zero_motion_ang6[32] = {
#include "shared/zero/event_zero_motion_ang6.inc"
};
static NJS_MKEY_A event_zero_motion_ang14[32] = {
#include "shared/zero/event_zero_motion_ang14.inc"
};
static NJS_MKEY_A event_zero_motion_ang15[32] = {
#include "shared/zero/event_zero_motion_ang15.inc"
};
static NJS_MKEY_A event_zero_motion_ang16[32] = {
#include "shared/zero/event_zero_motion_ang16.inc"
};
static NJS_MKEY_A event_zero_motion_ang18[32] = {
#include "shared/zero/event_zero_motion_ang18.inc"
};
static NJS_MKEY_A event_zero_motion_ang19[32] = {
#include "shared/zero/event_zero_motion_ang19.inc"
};
static NJS_MKEY_A event_zero_motion_ang20[32] = {
#include "shared/zero/event_zero_motion_ang20.inc"
};
static NJS_MKEY_A event_zero_motion_ang21[32] = {
#include "shared/zero/event_zero_motion_ang21.inc"
};
static NJS_MKEY_A event_zero_motion_ang26[32] = {
#include "shared/zero/event_zero_motion_ang26.inc"
};
static NJS_MKEY_A event_zero_motion_ang27[32] = {
#include "shared/zero/event_zero_motion_ang27.inc"
};
static NJS_MKEY_A event_zero_motion_ang28[32] = {
#include "shared/zero/event_zero_motion_ang28.inc"
};
static NJS_MKEY_F event_zero_motion_pos42[32] = {
#include "shared/zero/event_zero_motion_pos42.inc"
};
static NJS_MKEY_F event_zero_motion_pos44[32] = {
#include "shared/zero/event_zero_motion_pos44.inc"
};
static NJS_MKEY_F event_zero_motion_pos45[32] = {
#include "shared/zero/event_zero_motion_pos45.inc"
};
static NJS_MKEY_F event_zero_motion_pos46[32] = {
#include "shared/zero/event_zero_motion_pos46.inc"
};
static NJS_MDATA2 event_zero_motion_mdata[48] = {
#include "shared/zero/event_zero_motion_mdata.inc"
};
static NJS_MOTION event_zero_motion = {
#include "shared/zero/event_zero_motion.inc"
};
static NJS_ACTION event_zero_action = {
#include "shared/zero/event_zero_action.inc"
};
static NJS_MKEY_F event_zero_bird_motion_pos1[33] = {
#include "shared/zero/event_zero_bird_motion_pos1.inc"
};
static NJS_MKEY_A event_zero_bird_motion_ang1[33] = {
#include "shared/zero/event_zero_bird_motion_ang1.inc"
};
static NJS_MDATA3 event_zero_bird_motion_mdata[57] = {
#include "shared/zero/event_zero_bird_motion_mdata.inc"
};
static NJS_MOTION event_zero_bird_motion = {
#include "shared/zero/event_zero_bird_motion.inc"
};
static NJS_ACTION event_zero_bird_action = {
#include "shared/zero/event_zero_bird_action.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points0[330] = {
#include "shared/zero/event_zero_amy_head_shape_points0.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals0[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals0.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points1[330] = {
#include "shared/zero/event_zero_amy_head_shape_points1.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals1[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals1.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points2[330] = {
#include "shared/zero/event_zero_amy_head_shape_points2.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals2[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals2.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points3[330] = {
#include "shared/zero/event_zero_amy_head_shape_points3.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals3[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals3.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points4[330] = {
#include "shared/zero/event_zero_amy_head_shape_points4.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals4[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals4.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points5[330] = {
#include "shared/zero/event_zero_amy_head_shape_points5.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals5[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals5.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points6[330] = {
#include "shared/zero/event_zero_amy_head_shape_points6.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals6[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals6.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points7[330] = {
#include "shared/zero/event_zero_amy_head_shape_points7.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals7[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals7.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_points8[330] = {
#include "shared/zero/event_zero_amy_head_shape_points8.inc"
};
static NJS_POINT3 event_zero_amy_head_shape_normals8[330] = {
#include "shared/zero/event_zero_amy_head_shape_normals8.inc"
};
static NJS_MKEY_P event_zero_amy_head_shape_points[9] = {
#include "shared/zero/event_zero_amy_head_shape_points.inc"
};
static NJS_MKEY_P event_zero_amy_head_shape_normals[9] = {
#include "shared/zero/event_zero_amy_head_shape_normals.inc"
};
static NJS_MDATA2 event_zero_amy_head_shape_mdata[1] = {
#include "shared/zero/event_zero_amy_head_shape_mdata.inc"
};
static NJS_MOTION event_zero_amy_head_shape = {
#include "shared/zero/event_zero_amy_head_shape.inc"
};

static struct event_zero_anim event_zero_anims[1] = {
    {&event_zero_action,
     &event_zero_amy_action,
     &event_zero_bird_action,
     &event_zero_amy_head_motion,
     &event_zero_amy_head_shape,
     {0}},
};

static struct task *event_zero = NULL;

static void event_zero_destroy(struct task *task)
{
}

static void event_zero_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    NJS_MATRIX head;

    draw_set_palette(DRAW_PLAYER_PALETTE);
    njSetTexture(&event_zero_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(event_zero_anims[VARS(work)->anim].zero, VARS(work)->frame);
    draw_action(event_zero_anims[VARS(work)->anim].bird, VARS(work)->frame);
    draw_action(event_zero_anims[VARS(work)->anim].amy, VARS(work)->frame);
    pose_action(&_unit_matrix_, event_zero_anims[VARS(work)->anim].amy,
                VARS(work)->frame, AMY_HEAD_NODE, &head);
    njMultiMatrix(NULL, &head);
    draw_shape_motion(&event_zero_amy_head_object,
                      event_zero_anims[VARS(work)->anim].head_motion,
                      event_zero_anims[VARS(work)->anim].head_shape,
                      VARS(work)->frame);
    njPopMatrix(1);
    draw_set_palette(0);
}

static void event_zero_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    NJS_POINT3 under;
    struct angles slope;
    Float x;
    Float y;
    Float z;

    if (VARS(work)->frame >=
        (Float)(event_zero_anims[VARS(work)->anim].zero->motion->nbFrame - 1)) {
        VARS(work)->frame = 0.0F;
    }
    VARS(work)->frame += VARS(work)->speed;
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    under.y = ground_below(x, y, z, &slope);
    if (event->flags & EVENT_GROUND) {
        work->pos.y = under.y;
    }
    if (event->flags & EVENT_SLOPE) {
        work->ang.x = slope.x;
        work->ang.z = slope.z;
    }
    if (event->shadow > 0.0F) {
        under.x = x;
        under.y += SHADOW_LIFT;
        under.z = z;
        /* @hack spelling: the shadow's size reads work->event again, not the
         * local event. the image loads the pointer anew at 0x0cb81030 in
         * EV0016. through event, the step is a word shorter, and 26 of 106
         * words differ */
        shadow_draw_at(&slope, &under, work->event->shadow);
    }
    event_zero_draw(task);
}

static void event_zero_init(struct task *task)
{
    VARS(task->work)->frame = 0.0F;
    VARS(task->work)->speed = 1.0F;
    VARS(task->work)->anim = 0;
    event_alloc(task->work);
    task->step = event_zero_step;
    task->draw = event_zero_draw;
    task->destructor = event_zero_destroy;
}

struct task *event_zero_create(void)
{
    struct task *task;

    task = task_create(TASK_WORK, 3, event_zero_init);
    if (task != NULL) {
        event_zero = task;
    }
    return task;
}

void event_zero_set_speed(Float speed)
{
    struct task *task = event_zero;

    if (task != NULL) {
        VARS(task->work)->speed = speed;
    }
}

struct task *event_zero_get(void)
{
    return event_zero;
}
