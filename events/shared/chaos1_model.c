/* Chaos 1's model: 72 nodes rooted at event_chaos1_object, 37 of which hold
 * a model */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/chaos1_model.h"

static NJS_MATERIAL event_chaos1_materials_0[1] = {
#include "shared/chaos1_model/event_chaos1_materials_0.inc"
};
static Sint16 event_chaos1_strips_0[14] = {
#include "shared/chaos1_model/event_chaos1_strips_0.inc"
};
static NJS_COLOR event_chaos1_colours_0[12] = {
#include "shared/chaos1_model/event_chaos1_colours_0.inc"
};
static NJS_MESHSET event_chaos1_meshsets_0[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_0.inc"
};
static NJS_POINT3 event_chaos1_points_0[10] = {
#include "shared/chaos1_model/event_chaos1_points_0.inc"
};
static NJS_VECTOR event_chaos1_normals_0[10] = {
#include "shared/chaos1_model/event_chaos1_normals_0.inc"
};
static NJS_MODEL event_chaos1_model_0 = {
#include "shared/chaos1_model/event_chaos1_model_0.inc"
};
NJS_OBJECT event_chaos1_thigh_plus_z = {
#include "shared/chaos1_model/event_chaos1_thigh_plus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_1[1] = {
#include "shared/chaos1_model/event_chaos1_materials_1.inc"
};
static Sint16 event_chaos1_strips_1[14] = {
#include "shared/chaos1_model/event_chaos1_strips_1.inc"
};
static NJS_COLOR event_chaos1_colours_1[12] = {
#include "shared/chaos1_model/event_chaos1_colours_1.inc"
};
static NJS_MESHSET event_chaos1_meshsets_1[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_1.inc"
};
static NJS_POINT3 event_chaos1_points_1[10] = {
#include "shared/chaos1_model/event_chaos1_points_1.inc"
};
static NJS_VECTOR event_chaos1_normals_1[10] = {
#include "shared/chaos1_model/event_chaos1_normals_1.inc"
};
static NJS_MODEL event_chaos1_model_1 = {
#include "shared/chaos1_model/event_chaos1_model_1.inc"
};
NJS_OBJECT event_chaos1_shin_plus_z = {
#include "shared/chaos1_model/event_chaos1_shin_plus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_2[1] = {
#include "shared/chaos1_model/event_chaos1_materials_2.inc"
};
static Sint16 event_chaos1_strips_2[46] = {
#include "shared/chaos1_model/event_chaos1_strips_2.inc"
};
static NJS_COLOR event_chaos1_colours_2[40] = {
#include "shared/chaos1_model/event_chaos1_colours_2.inc"
};
static NJS_MESHSET event_chaos1_meshsets_2[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_2.inc"
};
static NJS_POINT3 event_chaos1_points_2[17] = {
#include "shared/chaos1_model/event_chaos1_points_2.inc"
};
static NJS_VECTOR event_chaos1_normals_2[17] = {
#include "shared/chaos1_model/event_chaos1_normals_2.inc"
};
static NJS_MODEL event_chaos1_model_2 = {
#include "shared/chaos1_model/event_chaos1_model_2.inc"
};
NJS_OBJECT event_chaos1_toe_plus_z = {
#include "shared/chaos1_model/event_chaos1_toe_plus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_3[1] = {
#include "shared/chaos1_model/event_chaos1_materials_3.inc"
};
static Sint16 event_chaos1_strips_3[20] = {
#include "shared/chaos1_model/event_chaos1_strips_3.inc"
};
static NJS_COLOR event_chaos1_colours_3[17] = {
#include "shared/chaos1_model/event_chaos1_colours_3.inc"
};
static NJS_MESHSET event_chaos1_meshsets_3[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_3.inc"
};
static NJS_POINT3 event_chaos1_points_3[10] = {
#include "shared/chaos1_model/event_chaos1_points_3.inc"
};
static NJS_VECTOR event_chaos1_normals_3[10] = {
#include "shared/chaos1_model/event_chaos1_normals_3.inc"
};
static NJS_MODEL event_chaos1_model_3 = {
#include "shared/chaos1_model/event_chaos1_model_3.inc"
};
NJS_OBJECT event_chaos1_foot_plus_z = {
#include "shared/chaos1_model/event_chaos1_foot_plus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_4[1] = {
#include "shared/chaos1_model/event_chaos1_materials_4.inc"
};
static Sint16 event_chaos1_strips_4[32] = {
#include "shared/chaos1_model/event_chaos1_strips_4.inc"
};
static NJS_COLOR event_chaos1_colours_4[28] = {
#include "shared/chaos1_model/event_chaos1_colours_4.inc"
};
static NJS_MESHSET event_chaos1_meshsets_4[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_4.inc"
};
static NJS_POINT3 event_chaos1_points_4[15] = {
#include "shared/chaos1_model/event_chaos1_points_4.inc"
};
static NJS_VECTOR event_chaos1_normals_4[15] = {
#include "shared/chaos1_model/event_chaos1_normals_4.inc"
};
static NJS_MODEL event_chaos1_model_4 = {
#include "shared/chaos1_model/event_chaos1_model_4.inc"
};
NJS_OBJECT event_chaos1_ankle_plus_z = {
#include "shared/chaos1_model/event_chaos1_ankle_plus_z.inc"
};
static NJS_OBJECT event_chaos1_node_0 = {
#include "shared/chaos1_model/event_chaos1_node_0.inc"
};
static NJS_OBJECT event_chaos1_node_1 = {
#include "shared/chaos1_model/event_chaos1_node_1.inc"
};
static NJS_OBJECT event_chaos1_node_2 = {
#include "shared/chaos1_model/event_chaos1_node_2.inc"
};
static NJS_OBJECT event_chaos1_node_3 = {
#include "shared/chaos1_model/event_chaos1_node_3.inc"
};
static NJS_OBJECT event_chaos1_node_4 = {
#include "shared/chaos1_model/event_chaos1_node_4.inc"
};
static NJS_MATERIAL event_chaos1_materials_5[1] = {
#include "shared/chaos1_model/event_chaos1_materials_5.inc"
};
static Sint16 event_chaos1_strips_5[14] = {
#include "shared/chaos1_model/event_chaos1_strips_5.inc"
};
static NJS_COLOR event_chaos1_colours_5[12] = {
#include "shared/chaos1_model/event_chaos1_colours_5.inc"
};
static NJS_MESHSET event_chaos1_meshsets_5[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_5.inc"
};
static NJS_POINT3 event_chaos1_points_5[10] = {
#include "shared/chaos1_model/event_chaos1_points_5.inc"
};
static NJS_VECTOR event_chaos1_normals_5[10] = {
#include "shared/chaos1_model/event_chaos1_normals_5.inc"
};
static NJS_MODEL event_chaos1_model_5 = {
#include "shared/chaos1_model/event_chaos1_model_5.inc"
};
NJS_OBJECT event_chaos1_thigh_minus_z = {
#include "shared/chaos1_model/event_chaos1_thigh_minus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_6[1] = {
#include "shared/chaos1_model/event_chaos1_materials_6.inc"
};
static Sint16 event_chaos1_strips_6[14] = {
#include "shared/chaos1_model/event_chaos1_strips_6.inc"
};
static NJS_COLOR event_chaos1_colours_6[12] = {
#include "shared/chaos1_model/event_chaos1_colours_6.inc"
};
static NJS_MESHSET event_chaos1_meshsets_6[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_6.inc"
};
static NJS_POINT3 event_chaos1_points_6[10] = {
#include "shared/chaos1_model/event_chaos1_points_6.inc"
};
static NJS_VECTOR event_chaos1_normals_6[10] = {
#include "shared/chaos1_model/event_chaos1_normals_6.inc"
};
static NJS_MODEL event_chaos1_model_6 = {
#include "shared/chaos1_model/event_chaos1_model_6.inc"
};
NJS_OBJECT event_chaos1_shin_minus_z = {
#include "shared/chaos1_model/event_chaos1_shin_minus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_7[1] = {
#include "shared/chaos1_model/event_chaos1_materials_7.inc"
};
static Sint16 event_chaos1_strips_7[44] = {
#include "shared/chaos1_model/event_chaos1_strips_7.inc"
};
static NJS_COLOR event_chaos1_colours_7[38] = {
#include "shared/chaos1_model/event_chaos1_colours_7.inc"
};
static NJS_MESHSET event_chaos1_meshsets_7[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_7.inc"
};
static NJS_POINT3 event_chaos1_points_7[17] = {
#include "shared/chaos1_model/event_chaos1_points_7.inc"
};
static NJS_VECTOR event_chaos1_normals_7[17] = {
#include "shared/chaos1_model/event_chaos1_normals_7.inc"
};
static NJS_MODEL event_chaos1_model_7 = {
#include "shared/chaos1_model/event_chaos1_model_7.inc"
};
NJS_OBJECT event_chaos1_toe_minus_z = {
#include "shared/chaos1_model/event_chaos1_toe_minus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_8[1] = {
#include "shared/chaos1_model/event_chaos1_materials_8.inc"
};
static Sint16 event_chaos1_strips_8[20] = {
#include "shared/chaos1_model/event_chaos1_strips_8.inc"
};
static NJS_COLOR event_chaos1_colours_8[17] = {
#include "shared/chaos1_model/event_chaos1_colours_8.inc"
};
static NJS_MESHSET event_chaos1_meshsets_8[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_8.inc"
};
static NJS_POINT3 event_chaos1_points_8[10] = {
#include "shared/chaos1_model/event_chaos1_points_8.inc"
};
static NJS_VECTOR event_chaos1_normals_8[10] = {
#include "shared/chaos1_model/event_chaos1_normals_8.inc"
};
static NJS_MODEL event_chaos1_model_8 = {
#include "shared/chaos1_model/event_chaos1_model_8.inc"
};
NJS_OBJECT event_chaos1_foot_minus_z = {
#include "shared/chaos1_model/event_chaos1_foot_minus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_9[1] = {
#include "shared/chaos1_model/event_chaos1_materials_9.inc"
};
static Sint16 event_chaos1_strips_9[32] = {
#include "shared/chaos1_model/event_chaos1_strips_9.inc"
};
static NJS_COLOR event_chaos1_colours_9[28] = {
#include "shared/chaos1_model/event_chaos1_colours_9.inc"
};
static NJS_MESHSET event_chaos1_meshsets_9[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_9.inc"
};
static NJS_POINT3 event_chaos1_points_9[15] = {
#include "shared/chaos1_model/event_chaos1_points_9.inc"
};
static NJS_VECTOR event_chaos1_normals_9[15] = {
#include "shared/chaos1_model/event_chaos1_normals_9.inc"
};
static NJS_MODEL event_chaos1_model_9 = {
#include "shared/chaos1_model/event_chaos1_model_9.inc"
};
NJS_OBJECT event_chaos1_ankle_minus_z = {
#include "shared/chaos1_model/event_chaos1_ankle_minus_z.inc"
};
static NJS_OBJECT event_chaos1_node_5 = {
#include "shared/chaos1_model/event_chaos1_node_5.inc"
};
static NJS_OBJECT event_chaos1_node_6 = {
#include "shared/chaos1_model/event_chaos1_node_6.inc"
};
static NJS_OBJECT event_chaos1_node_7 = {
#include "shared/chaos1_model/event_chaos1_node_7.inc"
};
static NJS_OBJECT event_chaos1_node_8 = {
#include "shared/chaos1_model/event_chaos1_node_8.inc"
};
static NJS_OBJECT event_chaos1_node_9 = {
#include "shared/chaos1_model/event_chaos1_node_9.inc"
};
static NJS_OBJECT event_chaos1_node_10 = {
#include "shared/chaos1_model/event_chaos1_node_10.inc"
};
static NJS_OBJECT event_chaos1_node_11 = {
#include "shared/chaos1_model/event_chaos1_node_11.inc"
};
static NJS_OBJECT event_chaos1_node_12 = {
#include "shared/chaos1_model/event_chaos1_node_12.inc"
};
static NJS_OBJECT event_chaos1_node_13 = {
#include "shared/chaos1_model/event_chaos1_node_13.inc"
};
static NJS_OBJECT event_chaos1_node_14 = {
#include "shared/chaos1_model/event_chaos1_node_14.inc"
};
static NJS_OBJECT event_chaos1_node_15 = {
#include "shared/chaos1_model/event_chaos1_node_15.inc"
};
static NJS_MATERIAL event_chaos1_materials_10[1] = {
#include "shared/chaos1_model/event_chaos1_materials_10.inc"
};
static Sint16 event_chaos1_strips_10[26] = {
#include "shared/chaos1_model/event_chaos1_strips_10.inc"
};
static NJS_COLOR event_chaos1_colours_10[24] = {
#include "shared/chaos1_model/event_chaos1_colours_10.inc"
};
static NJS_MESHSET event_chaos1_meshsets_10[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_10.inc"
};
static NJS_POINT3 event_chaos1_points_10[15] = {
#include "shared/chaos1_model/event_chaos1_points_10.inc"
};
static NJS_VECTOR event_chaos1_normals_10[15] = {
#include "shared/chaos1_model/event_chaos1_normals_10.inc"
};
static NJS_MODEL event_chaos1_model_10 = {
#include "shared/chaos1_model/event_chaos1_model_10.inc"
};
NJS_OBJECT event_chaos1_upper_arm_plus_z = {
#include "shared/chaos1_model/event_chaos1_upper_arm_plus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_11[1] = {
#include "shared/chaos1_model/event_chaos1_materials_11.inc"
};
static Sint16 event_chaos1_strips_11[14] = {
#include "shared/chaos1_model/event_chaos1_strips_11.inc"
};
static NJS_COLOR event_chaos1_colours_11[12] = {
#include "shared/chaos1_model/event_chaos1_colours_11.inc"
};
static NJS_MESHSET event_chaos1_meshsets_11[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_11.inc"
};
static NJS_POINT3 event_chaos1_points_11[10] = {
#include "shared/chaos1_model/event_chaos1_points_11.inc"
};
static NJS_VECTOR event_chaos1_normals_11[10] = {
#include "shared/chaos1_model/event_chaos1_normals_11.inc"
};
static NJS_MODEL event_chaos1_model_11 = {
#include "shared/chaos1_model/event_chaos1_model_11.inc"
};
NJS_OBJECT event_chaos1_forearm_plus_z = {
#include "shared/chaos1_model/event_chaos1_forearm_plus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_12[1] = {
#include "shared/chaos1_model/event_chaos1_materials_12.inc"
};
static Sint16 event_chaos1_strips_12[58] = {
#include "shared/chaos1_model/event_chaos1_strips_12.inc"
};
static NJS_COLOR event_chaos1_colours_12[51] = {
#include "shared/chaos1_model/event_chaos1_colours_12.inc"
};
static NJS_MESHSET event_chaos1_meshsets_12[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_12.inc"
};
static NJS_POINT3 event_chaos1_points_12[22] = {
#include "shared/chaos1_model/event_chaos1_points_12.inc"
};
static NJS_VECTOR event_chaos1_normals_12[22] = {
#include "shared/chaos1_model/event_chaos1_normals_12.inc"
};
static NJS_MODEL event_chaos1_model_12 = {
#include "shared/chaos1_model/event_chaos1_model_12.inc"
};
NJS_OBJECT event_chaos1_fingers_plus_z = {
#include "shared/chaos1_model/event_chaos1_fingers_plus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_13[1] = {
#include "shared/chaos1_model/event_chaos1_materials_13.inc"
};
static Sint16 event_chaos1_strips_13[24] = {
#include "shared/chaos1_model/event_chaos1_strips_13.inc"
};
static NJS_COLOR event_chaos1_colours_13[21] = {
#include "shared/chaos1_model/event_chaos1_colours_13.inc"
};
static NJS_MESHSET event_chaos1_meshsets_13[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_13.inc"
};
static NJS_POINT3 event_chaos1_points_13[10] = {
#include "shared/chaos1_model/event_chaos1_points_13.inc"
};
static NJS_VECTOR event_chaos1_normals_13[10] = {
#include "shared/chaos1_model/event_chaos1_normals_13.inc"
};
static NJS_MODEL event_chaos1_model_13 = {
#include "shared/chaos1_model/event_chaos1_model_13.inc"
};
static NJS_OBJECT event_chaos1_node_16 = {
#include "shared/chaos1_model/event_chaos1_node_16.inc"
};
static NJS_MATERIAL event_chaos1_materials_14[1] = {
#include "shared/chaos1_model/event_chaos1_materials_14.inc"
};
static Sint16 event_chaos1_strips_14[14] = {
#include "shared/chaos1_model/event_chaos1_strips_14.inc"
};
static NJS_COLOR event_chaos1_colours_14[12] = {
#include "shared/chaos1_model/event_chaos1_colours_14.inc"
};
static NJS_MESHSET event_chaos1_meshsets_14[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_14.inc"
};
static NJS_POINT3 event_chaos1_points_14[10] = {
#include "shared/chaos1_model/event_chaos1_points_14.inc"
};
static NJS_VECTOR event_chaos1_normals_14[10] = {
#include "shared/chaos1_model/event_chaos1_normals_14.inc"
};
static NJS_MODEL event_chaos1_model_14 = {
#include "shared/chaos1_model/event_chaos1_model_14.inc"
};
NJS_OBJECT event_chaos1_hand_plus_z = {
#include "shared/chaos1_model/event_chaos1_hand_plus_z.inc"
};
static NJS_OBJECT event_chaos1_node_17 = {
#include "shared/chaos1_model/event_chaos1_node_17.inc"
};
static NJS_OBJECT event_chaos1_node_18 = {
#include "shared/chaos1_model/event_chaos1_node_18.inc"
};
static NJS_OBJECT event_chaos1_node_19 = {
#include "shared/chaos1_model/event_chaos1_node_19.inc"
};
static NJS_OBJECT event_chaos1_node_20 = {
#include "shared/chaos1_model/event_chaos1_node_20.inc"
};
static NJS_OBJECT event_chaos1_node_21 = {
#include "shared/chaos1_model/event_chaos1_node_21.inc"
};
static NJS_MATERIAL event_chaos1_materials_15[1] = {
#include "shared/chaos1_model/event_chaos1_materials_15.inc"
};
static Sint16 event_chaos1_strips_15[48] = {
#include "shared/chaos1_model/event_chaos1_strips_15.inc"
};
static NJS_COLOR event_chaos1_colours_15[42] = {
#include "shared/chaos1_model/event_chaos1_colours_15.inc"
};
static NJS_MESHSET event_chaos1_meshsets_15[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_15.inc"
};
static NJS_POINT3 event_chaos1_points_15[21] = {
#include "shared/chaos1_model/event_chaos1_points_15.inc"
};
static NJS_VECTOR event_chaos1_normals_15[21] = {
#include "shared/chaos1_model/event_chaos1_normals_15.inc"
};
static NJS_MODEL event_chaos1_model_15 = {
#include "shared/chaos1_model/event_chaos1_model_15.inc"
};
NJS_OBJECT event_chaos1_upper_arm_minus_z = {
#include "shared/chaos1_model/event_chaos1_upper_arm_minus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_16[1] = {
#include "shared/chaos1_model/event_chaos1_materials_16.inc"
};
static Sint16 event_chaos1_strips_16[110] = {
#include "shared/chaos1_model/event_chaos1_strips_16.inc"
};
static NJS_COLOR event_chaos1_colours_16[88] = {
#include "shared/chaos1_model/event_chaos1_colours_16.inc"
};
static NJS_MESHSET event_chaos1_meshsets_16[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_16.inc"
};
static NJS_POINT3 event_chaos1_points_16[24] = {
#include "shared/chaos1_model/event_chaos1_points_16.inc"
};
static NJS_VECTOR event_chaos1_normals_16[24] = {
#include "shared/chaos1_model/event_chaos1_normals_16.inc"
};
static NJS_MODEL event_chaos1_model_16 = {
#include "shared/chaos1_model/event_chaos1_model_16.inc"
};
static NJS_OBJECT event_chaos1_node_22 = {
#include "shared/chaos1_model/event_chaos1_node_22.inc"
};
static NJS_MATERIAL event_chaos1_materials_17[1] = {
#include "shared/chaos1_model/event_chaos1_materials_17.inc"
};
static Sint16 event_chaos1_strips_17[40] = {
#include "shared/chaos1_model/event_chaos1_strips_17.inc"
};
static NJS_COLOR event_chaos1_colours_17[36] = {
#include "shared/chaos1_model/event_chaos1_colours_17.inc"
};
static NJS_MESHSET event_chaos1_meshsets_17[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_17.inc"
};
static NJS_POINT3 event_chaos1_points_17[20] = {
#include "shared/chaos1_model/event_chaos1_points_17.inc"
};
static NJS_VECTOR event_chaos1_normals_17[20] = {
#include "shared/chaos1_model/event_chaos1_normals_17.inc"
};
static NJS_MODEL event_chaos1_model_17 = {
#include "shared/chaos1_model/event_chaos1_model_17.inc"
};
NJS_OBJECT event_chaos1_forearm_minus_z = {
#include "shared/chaos1_model/event_chaos1_forearm_minus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_18[1] = {
#include "shared/chaos1_model/event_chaos1_materials_18.inc"
};
static Sint16 event_chaos1_strips_18[110] = {
#include "shared/chaos1_model/event_chaos1_strips_18.inc"
};
static NJS_COLOR event_chaos1_colours_18[88] = {
#include "shared/chaos1_model/event_chaos1_colours_18.inc"
};
static NJS_MESHSET event_chaos1_meshsets_18[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_18.inc"
};
static NJS_POINT3 event_chaos1_points_18[24] = {
#include "shared/chaos1_model/event_chaos1_points_18.inc"
};
static NJS_VECTOR event_chaos1_normals_18[24] = {
#include "shared/chaos1_model/event_chaos1_normals_18.inc"
};
static NJS_MODEL event_chaos1_model_18 = {
#include "shared/chaos1_model/event_chaos1_model_18.inc"
};
static NJS_OBJECT event_chaos1_node_23 = {
#include "shared/chaos1_model/event_chaos1_node_23.inc"
};
static NJS_MATERIAL event_chaos1_materials_19[1] = {
#include "shared/chaos1_model/event_chaos1_materials_19.inc"
};
static Sint16 event_chaos1_strips_19[198] = {
#include "shared/chaos1_model/event_chaos1_strips_19.inc"
};
static NJS_COLOR event_chaos1_colours_19[158] = {
#include "shared/chaos1_model/event_chaos1_colours_19.inc"
};
static NJS_MESHSET event_chaos1_meshsets_19[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_19.inc"
};
static NJS_POINT3 event_chaos1_points_19[39] = {
#include "shared/chaos1_model/event_chaos1_points_19.inc"
};
static NJS_VECTOR event_chaos1_normals_19[39] = {
#include "shared/chaos1_model/event_chaos1_normals_19.inc"
};
static NJS_MODEL event_chaos1_model_19 = {
#include "shared/chaos1_model/event_chaos1_model_19.inc"
};
static NJS_OBJECT event_chaos1_node_24 = {
#include "shared/chaos1_model/event_chaos1_node_24.inc"
};
static NJS_MATERIAL event_chaos1_materials_20[1] = {
#include "shared/chaos1_model/event_chaos1_materials_20.inc"
};
static Sint16 event_chaos1_strips_20[18] = {
#include "shared/chaos1_model/event_chaos1_strips_20.inc"
};
static NJS_COLOR event_chaos1_colours_20[14] = {
#include "shared/chaos1_model/event_chaos1_colours_20.inc"
};
static NJS_MESHSET event_chaos1_meshsets_20[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_20.inc"
};
static NJS_POINT3 event_chaos1_points_20[6] = {
#include "shared/chaos1_model/event_chaos1_points_20.inc"
};
static NJS_VECTOR event_chaos1_normals_20[6] = {
#include "shared/chaos1_model/event_chaos1_normals_20.inc"
};
static NJS_MODEL event_chaos1_model_20 = {
#include "shared/chaos1_model/event_chaos1_model_20.inc"
};
static NJS_OBJECT event_chaos1_node_25 = {
#include "shared/chaos1_model/event_chaos1_node_25.inc"
};
static NJS_MATERIAL event_chaos1_materials_21[1] = {
#include "shared/chaos1_model/event_chaos1_materials_21.inc"
};
static Sint16 event_chaos1_strips_21[18] = {
#include "shared/chaos1_model/event_chaos1_strips_21.inc"
};
static NJS_COLOR event_chaos1_colours_21[14] = {
#include "shared/chaos1_model/event_chaos1_colours_21.inc"
};
static NJS_MESHSET event_chaos1_meshsets_21[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_21.inc"
};
static NJS_POINT3 event_chaos1_points_21[6] = {
#include "shared/chaos1_model/event_chaos1_points_21.inc"
};
static NJS_VECTOR event_chaos1_normals_21[6] = {
#include "shared/chaos1_model/event_chaos1_normals_21.inc"
};
static NJS_MODEL event_chaos1_model_21 = {
#include "shared/chaos1_model/event_chaos1_model_21.inc"
};
static NJS_OBJECT event_chaos1_node_26 = {
#include "shared/chaos1_model/event_chaos1_node_26.inc"
};
static NJS_MATERIAL event_chaos1_materials_22[1] = {
#include "shared/chaos1_model/event_chaos1_materials_22.inc"
};
static Sint16 event_chaos1_strips_22[110] = {
#include "shared/chaos1_model/event_chaos1_strips_22.inc"
};
static NJS_COLOR event_chaos1_colours_22[88] = {
#include "shared/chaos1_model/event_chaos1_colours_22.inc"
};
static NJS_MESHSET event_chaos1_meshsets_22[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_22.inc"
};
static NJS_POINT3 event_chaos1_points_22[24] = {
#include "shared/chaos1_model/event_chaos1_points_22.inc"
};
static NJS_VECTOR event_chaos1_normals_22[24] = {
#include "shared/chaos1_model/event_chaos1_normals_22.inc"
};
static NJS_MODEL event_chaos1_model_22 = {
#include "shared/chaos1_model/event_chaos1_model_22.inc"
};
static NJS_OBJECT event_chaos1_node_27 = {
#include "shared/chaos1_model/event_chaos1_node_27.inc"
};
static NJS_MATERIAL event_chaos1_materials_23[1] = {
#include "shared/chaos1_model/event_chaos1_materials_23.inc"
};
static Sint16 event_chaos1_strips_23[110] = {
#include "shared/chaos1_model/event_chaos1_strips_23.inc"
};
static NJS_COLOR event_chaos1_colours_23[88] = {
#include "shared/chaos1_model/event_chaos1_colours_23.inc"
};
static NJS_MESHSET event_chaos1_meshsets_23[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_23.inc"
};
static NJS_POINT3 event_chaos1_points_23[24] = {
#include "shared/chaos1_model/event_chaos1_points_23.inc"
};
static NJS_VECTOR event_chaos1_normals_23[24] = {
#include "shared/chaos1_model/event_chaos1_normals_23.inc"
};
static NJS_MODEL event_chaos1_model_23 = {
#include "shared/chaos1_model/event_chaos1_model_23.inc"
};
static NJS_OBJECT event_chaos1_node_28 = {
#include "shared/chaos1_model/event_chaos1_node_28.inc"
};
static NJS_MATERIAL event_chaos1_materials_24[1] = {
#include "shared/chaos1_model/event_chaos1_materials_24.inc"
};
static Sint16 event_chaos1_strips_24[110] = {
#include "shared/chaos1_model/event_chaos1_strips_24.inc"
};
static NJS_COLOR event_chaos1_colours_24[88] = {
#include "shared/chaos1_model/event_chaos1_colours_24.inc"
};
static NJS_MESHSET event_chaos1_meshsets_24[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_24.inc"
};
static NJS_POINT3 event_chaos1_points_24[24] = {
#include "shared/chaos1_model/event_chaos1_points_24.inc"
};
static NJS_VECTOR event_chaos1_normals_24[24] = {
#include "shared/chaos1_model/event_chaos1_normals_24.inc"
};
static NJS_MODEL event_chaos1_model_24 = {
#include "shared/chaos1_model/event_chaos1_model_24.inc"
};
static NJS_OBJECT event_chaos1_node_29 = {
#include "shared/chaos1_model/event_chaos1_node_29.inc"
};
static NJS_MATERIAL event_chaos1_materials_25[1] = {
#include "shared/chaos1_model/event_chaos1_materials_25.inc"
};
static Sint16 event_chaos1_strips_25[110] = {
#include "shared/chaos1_model/event_chaos1_strips_25.inc"
};
static NJS_COLOR event_chaos1_colours_25[88] = {
#include "shared/chaos1_model/event_chaos1_colours_25.inc"
};
static NJS_MESHSET event_chaos1_meshsets_25[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_25.inc"
};
static NJS_POINT3 event_chaos1_points_25[24] = {
#include "shared/chaos1_model/event_chaos1_points_25.inc"
};
static NJS_VECTOR event_chaos1_normals_25[24] = {
#include "shared/chaos1_model/event_chaos1_normals_25.inc"
};
static NJS_MODEL event_chaos1_model_25 = {
#include "shared/chaos1_model/event_chaos1_model_25.inc"
};
static NJS_OBJECT event_chaos1_node_30 = {
#include "shared/chaos1_model/event_chaos1_node_30.inc"
};
static NJS_MATERIAL event_chaos1_materials_26[1] = {
#include "shared/chaos1_model/event_chaos1_materials_26.inc"
};
static Sint16 event_chaos1_strips_26[70] = {
#include "shared/chaos1_model/event_chaos1_strips_26.inc"
};
static NJS_COLOR event_chaos1_colours_26[63] = {
#include "shared/chaos1_model/event_chaos1_colours_26.inc"
};
static NJS_MESHSET event_chaos1_meshsets_26[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_26.inc"
};
static NJS_POINT3 event_chaos1_points_26[28] = {
#include "shared/chaos1_model/event_chaos1_points_26.inc"
};
static NJS_VECTOR event_chaos1_normals_26[28] = {
#include "shared/chaos1_model/event_chaos1_normals_26.inc"
};
static NJS_MODEL event_chaos1_model_26 = {
#include "shared/chaos1_model/event_chaos1_model_26.inc"
};
NJS_OBJECT event_chaos1_fingers_minus_z = {
#include "shared/chaos1_model/event_chaos1_fingers_minus_z.inc"
};
static NJS_MATERIAL event_chaos1_materials_27[1] = {
#include "shared/chaos1_model/event_chaos1_materials_27.inc"
};
static Sint16 event_chaos1_strips_27[110] = {
#include "shared/chaos1_model/event_chaos1_strips_27.inc"
};
static NJS_COLOR event_chaos1_colours_27[88] = {
#include "shared/chaos1_model/event_chaos1_colours_27.inc"
};
static NJS_MESHSET event_chaos1_meshsets_27[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_27.inc"
};
static NJS_POINT3 event_chaos1_points_27[24] = {
#include "shared/chaos1_model/event_chaos1_points_27.inc"
};
static NJS_VECTOR event_chaos1_normals_27[24] = {
#include "shared/chaos1_model/event_chaos1_normals_27.inc"
};
static NJS_MODEL event_chaos1_model_27 = {
#include "shared/chaos1_model/event_chaos1_model_27.inc"
};
static NJS_OBJECT event_chaos1_node_31 = {
#include "shared/chaos1_model/event_chaos1_node_31.inc"
};
static NJS_MATERIAL event_chaos1_materials_28[1] = {
#include "shared/chaos1_model/event_chaos1_materials_28.inc"
};
static Sint16 event_chaos1_strips_28[110] = {
#include "shared/chaos1_model/event_chaos1_strips_28.inc"
};
static NJS_COLOR event_chaos1_colours_28[88] = {
#include "shared/chaos1_model/event_chaos1_colours_28.inc"
};
static NJS_MESHSET event_chaos1_meshsets_28[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_28.inc"
};
static NJS_POINT3 event_chaos1_points_28[24] = {
#include "shared/chaos1_model/event_chaos1_points_28.inc"
};
static NJS_VECTOR event_chaos1_normals_28[24] = {
#include "shared/chaos1_model/event_chaos1_normals_28.inc"
};
static NJS_MODEL event_chaos1_model_28 = {
#include "shared/chaos1_model/event_chaos1_model_28.inc"
};
static NJS_OBJECT event_chaos1_node_32 = {
#include "shared/chaos1_model/event_chaos1_node_32.inc"
};
static NJS_MATERIAL event_chaos1_materials_29[1] = {
#include "shared/chaos1_model/event_chaos1_materials_29.inc"
};
static Sint16 event_chaos1_strips_29[24] = {
#include "shared/chaos1_model/event_chaos1_strips_29.inc"
};
static NJS_COLOR event_chaos1_colours_29[21] = {
#include "shared/chaos1_model/event_chaos1_colours_29.inc"
};
static NJS_MESHSET event_chaos1_meshsets_29[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_29.inc"
};
static NJS_POINT3 event_chaos1_points_29[10] = {
#include "shared/chaos1_model/event_chaos1_points_29.inc"
};
static NJS_VECTOR event_chaos1_normals_29[10] = {
#include "shared/chaos1_model/event_chaos1_normals_29.inc"
};
static NJS_MODEL event_chaos1_model_29 = {
#include "shared/chaos1_model/event_chaos1_model_29.inc"
};
static NJS_OBJECT event_chaos1_node_33 = {
#include "shared/chaos1_model/event_chaos1_node_33.inc"
};
static NJS_MATERIAL event_chaos1_materials_30[1] = {
#include "shared/chaos1_model/event_chaos1_materials_30.inc"
};
static Sint16 event_chaos1_strips_30[56] = {
#include "shared/chaos1_model/event_chaos1_strips_30.inc"
};
static NJS_COLOR event_chaos1_colours_30[50] = {
#include "shared/chaos1_model/event_chaos1_colours_30.inc"
};
static NJS_MESHSET event_chaos1_meshsets_30[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_30.inc"
};
static NJS_POINT3 event_chaos1_points_30[25] = {
#include "shared/chaos1_model/event_chaos1_points_30.inc"
};
static NJS_VECTOR event_chaos1_normals_30[25] = {
#include "shared/chaos1_model/event_chaos1_normals_30.inc"
};
static NJS_MODEL event_chaos1_model_30 = {
#include "shared/chaos1_model/event_chaos1_model_30.inc"
};
NJS_OBJECT event_chaos1_hand_minus_z = {
#include "shared/chaos1_model/event_chaos1_hand_minus_z.inc"
};
static NJS_OBJECT event_chaos1_node_34 = {
#include "shared/chaos1_model/event_chaos1_node_34.inc"
};
static NJS_OBJECT event_chaos1_node_35 = {
#include "shared/chaos1_model/event_chaos1_node_35.inc"
};
static NJS_OBJECT event_chaos1_node_36 = {
#include "shared/chaos1_model/event_chaos1_node_36.inc"
};
static NJS_OBJECT event_chaos1_node_37 = {
#include "shared/chaos1_model/event_chaos1_node_37.inc"
};
static NJS_OBJECT event_chaos1_node_38 = {
#include "shared/chaos1_model/event_chaos1_node_38.inc"
};
static NJS_MATERIAL event_chaos1_materials_31[2] = {
#include "shared/chaos1_model/event_chaos1_materials_31.inc"
};
static Sint16 event_chaos1_strips_31[16] = {
#include "shared/chaos1_model/event_chaos1_strips_31.inc"
};
static Sint16 event_chaos1_strips_32[44] = {
#include "shared/chaos1_model/event_chaos1_strips_32.inc"
};
static NJS_COLOR event_chaos1_colours_31[14] = {
#include "shared/chaos1_model/event_chaos1_colours_31.inc"
};
static NJS_COLOR event_chaos1_colours_32[40] = {
#include "shared/chaos1_model/event_chaos1_colours_32.inc"
};
static NJS_MESHSET event_chaos1_meshsets_31[2] = {
#include "shared/chaos1_model/event_chaos1_meshsets_31.inc"
};
static NJS_POINT3 event_chaos1_points_31[48] = {
#include "shared/chaos1_model/event_chaos1_points_31.inc"
};
static NJS_VECTOR event_chaos1_normals_31[48] = {
#include "shared/chaos1_model/event_chaos1_normals_31.inc"
};
static NJS_MODEL event_chaos1_model_31 = {
#include "shared/chaos1_model/event_chaos1_model_31.inc"
};
static NJS_OBJECT event_chaos1_node_39 = {
#include "shared/chaos1_model/event_chaos1_node_39.inc"
};
static NJS_MATERIAL event_chaos1_materials_32[1] = {
#include "shared/chaos1_model/event_chaos1_materials_32.inc"
};
static Sint16 event_chaos1_strips_33[84] = {
#include "shared/chaos1_model/event_chaos1_strips_33.inc"
};
static NJS_COLOR event_chaos1_colours_33[76] = {
#include "shared/chaos1_model/event_chaos1_colours_33.inc"
};
static NJS_MESHSET event_chaos1_meshsets_32[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_32.inc"
};
static NJS_POINT3 event_chaos1_points_32[32] = {
#include "shared/chaos1_model/event_chaos1_points_32.inc"
};
static NJS_VECTOR event_chaos1_normals_32[32] = {
#include "shared/chaos1_model/event_chaos1_normals_32.inc"
};
static NJS_MODEL event_chaos1_model_32 = {
#include "shared/chaos1_model/event_chaos1_model_32.inc"
};
static NJS_OBJECT event_chaos1_node_40 = {
#include "shared/chaos1_model/event_chaos1_node_40.inc"
};
static NJS_MATERIAL event_chaos1_materials_33[1] = {
#include "shared/chaos1_model/event_chaos1_materials_33.inc"
};
static Sint16 event_chaos1_strips_34[82] = {
#include "shared/chaos1_model/event_chaos1_strips_34.inc"
};
static NJS_COLOR event_chaos1_colours_34[72] = {
#include "shared/chaos1_model/event_chaos1_colours_34.inc"
};
static NJS_MESHSET event_chaos1_meshsets_33[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_33.inc"
};
static NJS_POINT3 event_chaos1_points_33[38] = {
#include "shared/chaos1_model/event_chaos1_points_33.inc"
};
static NJS_VECTOR event_chaos1_normals_33[38] = {
#include "shared/chaos1_model/event_chaos1_normals_33.inc"
};
static NJS_MODEL event_chaos1_model_33 = {
#include "shared/chaos1_model/event_chaos1_model_33.inc"
};
static NJS_OBJECT event_chaos1_node_41 = {
#include "shared/chaos1_model/event_chaos1_node_41.inc"
};
static NJS_MATERIAL event_chaos1_materials_34[2] = {
#include "shared/chaos1_model/event_chaos1_materials_34.inc"
};
static Sint16 event_chaos1_strips_35[85] = {
#include "shared/chaos1_model/event_chaos1_strips_35.inc"
};
static Sint16 event_chaos1_strips_36[393] = {
#include "shared/chaos1_model/event_chaos1_strips_36.inc"
};
static NJS_COLOR event_chaos1_colours_35[74] = {
#include "shared/chaos1_model/event_chaos1_colours_35.inc"
};
static NJS_COLOR event_chaos1_colours_36[350] = {
#include "shared/chaos1_model/event_chaos1_colours_36.inc"
};
static NJS_MESHSET event_chaos1_meshsets_34[2] = {
#include "shared/chaos1_model/event_chaos1_meshsets_34.inc"
};
static NJS_POINT3 event_chaos1_points_34[162] = {
#include "shared/chaos1_model/event_chaos1_points_34.inc"
};
static NJS_VECTOR event_chaos1_normals_34[162] = {
#include "shared/chaos1_model/event_chaos1_normals_34.inc"
};
static NJS_MODEL event_chaos1_model_34 = {
#include "shared/chaos1_model/event_chaos1_model_34.inc"
};
static NJS_OBJECT event_chaos1_node_42 = {
#include "shared/chaos1_model/event_chaos1_node_42.inc"
};
static NJS_OBJECT event_chaos1_node_43 = {
#include "shared/chaos1_model/event_chaos1_node_43.inc"
};
static NJS_OBJECT event_chaos1_node_44 = {
#include "shared/chaos1_model/event_chaos1_node_44.inc"
};
static NJS_OBJECT event_chaos1_node_45 = {
#include "shared/chaos1_model/event_chaos1_node_45.inc"
};
static NJS_MATERIAL event_chaos1_materials_35[1] = {
#include "shared/chaos1_model/event_chaos1_materials_35.inc"
};
static Sint16 event_chaos1_strips_37[300] = {
#include "shared/chaos1_model/event_chaos1_strips_37.inc"
};
static NJS_COLOR event_chaos1_colours_37[262] = {
#include "shared/chaos1_model/event_chaos1_colours_37.inc"
};
static NJS_MESHSET event_chaos1_meshsets_35[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_35.inc"
};
static NJS_POINT3 event_chaos1_points_35[104] = {
#include "shared/chaos1_model/event_chaos1_points_35.inc"
};
static NJS_VECTOR event_chaos1_normals_35[104] = {
#include "shared/chaos1_model/event_chaos1_normals_35.inc"
};
static NJS_MODEL event_chaos1_model_35 = {
#include "shared/chaos1_model/event_chaos1_model_35.inc"
};
NJS_OBJECT event_chaos1_torso = {
#include "shared/chaos1_model/event_chaos1_torso.inc"
};
static NJS_OBJECT event_chaos1_node_46 = {
#include "shared/chaos1_model/event_chaos1_node_46.inc"
};
static NJS_OBJECT event_chaos1_node_47 = {
#include "shared/chaos1_model/event_chaos1_node_47.inc"
};
static NJS_OBJECT event_chaos1_node_48 = {
#include "shared/chaos1_model/event_chaos1_node_48.inc"
};
static NJS_MATERIAL event_chaos1_materials_36[1] = {
#include "shared/chaos1_model/event_chaos1_materials_36.inc"
};
static Sint16 event_chaos1_strips_38[96] = {
#include "shared/chaos1_model/event_chaos1_strips_38.inc"
};
static NJS_COLOR event_chaos1_colours_38[86] = {
#include "shared/chaos1_model/event_chaos1_colours_38.inc"
};
static NJS_MESHSET event_chaos1_meshsets_36[1] = {
#include "shared/chaos1_model/event_chaos1_meshsets_36.inc"
};
static NJS_POINT3 event_chaos1_points_36[37] = {
#include "shared/chaos1_model/event_chaos1_points_36.inc"
};
static NJS_VECTOR event_chaos1_normals_36[37] = {
#include "shared/chaos1_model/event_chaos1_normals_36.inc"
};
static NJS_MODEL event_chaos1_model_36 = {
#include "shared/chaos1_model/event_chaos1_model_36.inc"
};
NJS_OBJECT event_chaos1_hips = {
#include "shared/chaos1_model/event_chaos1_hips.inc"
};
static NJS_OBJECT event_chaos1_node_49 = {
#include "shared/chaos1_model/event_chaos1_node_49.inc"
};
static NJS_OBJECT event_chaos1_node_50 = {
#include "shared/chaos1_model/event_chaos1_node_50.inc"
};
NJS_OBJECT event_chaos1_object = {
#include "shared/chaos1_model/event_chaos1_object.inc"
};
