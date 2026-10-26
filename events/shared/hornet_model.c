/* the Egg Hornet's model but for its root node, which hornet.c holds: 14
 * nodes and 15 models. the last model, event_hornet_model, is the root's */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/hornet_model.h"

static NJS_MATERIAL event_hornet_materials_0[1] = {
#include "shared/hornet_model/event_hornet_materials_0.inc"
};
static Sint16 event_hornet_strips_0[6] = {
#include "shared/hornet_model/event_hornet_strips_0.inc"
};
static NJS_COLOR event_hornet_colours_0[4] = {
#include "shared/hornet_model/event_hornet_colours_0.inc"
};
static NJS_MESHSET event_hornet_meshsets_0[1] = {
#include "shared/hornet_model/event_hornet_meshsets_0.inc"
};
static NJS_POINT3 event_hornet_points_0[4] = {
#include "shared/hornet_model/event_hornet_points_0.inc"
};
static NJS_VECTOR event_hornet_normals_0[4] = {
#include "shared/hornet_model/event_hornet_normals_0.inc"
};
static NJS_MODEL event_hornet_model_0 = {
#include "shared/hornet_model/event_hornet_model_0.inc"
};
static NJS_OBJECT event_hornet_node_0 = {
#include "shared/hornet_model/event_hornet_node_0.inc"
};
static NJS_MATERIAL event_hornet_materials_1[7] = {
#include "shared/hornet_model/event_hornet_materials_1.inc"
};
static Sint16 event_hornet_strips_1[7] = {
#include "shared/hornet_model/event_hornet_strips_1.inc"
};
static Sint16 event_hornet_strips_2[9] = {
#include "shared/hornet_model/event_hornet_strips_2.inc"
};
static Sint16 event_hornet_strips_3[9] = {
#include "shared/hornet_model/event_hornet_strips_3.inc"
};
static Sint16 event_hornet_strips_4[15] = {
#include "shared/hornet_model/event_hornet_strips_4.inc"
};
static Sint16 event_hornet_strips_5[9] = {
#include "shared/hornet_model/event_hornet_strips_5.inc"
};
static Sint16 event_hornet_strips_6[33] = {
#include "shared/hornet_model/event_hornet_strips_6.inc"
};
static Sint16 event_hornet_strips_7[14] = {
#include "shared/hornet_model/event_hornet_strips_7.inc"
};
static NJS_COLOR event_hornet_colours_1[6] = {
#include "shared/hornet_model/event_hornet_colours_1.inc"
};
static NJS_COLOR event_hornet_colours_2[8] = {
#include "shared/hornet_model/event_hornet_colours_2.inc"
};
static NJS_COLOR event_hornet_colours_3[8] = {
#include "shared/hornet_model/event_hornet_colours_3.inc"
};
static NJS_COLOR event_hornet_colours_4[12] = {
#include "shared/hornet_model/event_hornet_colours_4.inc"
};
static NJS_COLOR event_hornet_colours_5[8] = {
#include "shared/hornet_model/event_hornet_colours_5.inc"
};
static NJS_COLOR event_hornet_colours_6[30] = {
#include "shared/hornet_model/event_hornet_colours_6.inc"
};
static NJS_COLOR event_hornet_colours_7[10] = {
#include "shared/hornet_model/event_hornet_colours_7.inc"
};
static NJS_MESHSET event_hornet_meshsets_1[7] = {
#include "shared/hornet_model/event_hornet_meshsets_1.inc"
};
static NJS_POINT3 event_hornet_points_1[30] = {
#include "shared/hornet_model/event_hornet_points_1.inc"
};
static NJS_VECTOR event_hornet_normals_1[30] = {
#include "shared/hornet_model/event_hornet_normals_1.inc"
};
static NJS_MODEL event_hornet_model_1 = {
#include "shared/hornet_model/event_hornet_model_1.inc"
};
static NJS_OBJECT event_hornet_node_1 = {
#include "shared/hornet_model/event_hornet_node_1.inc"
};
static NJS_MATERIAL event_hornet_materials_2[5] = {
#include "shared/hornet_model/event_hornet_materials_2.inc"
};
static Sint16 event_hornet_strips_8[12] = {
#include "shared/hornet_model/event_hornet_strips_8.inc"
};
static Sint16 event_hornet_strips_9[15] = {
#include "shared/hornet_model/event_hornet_strips_9.inc"
};
static Sint16 event_hornet_strips_10[30] = {
#include "shared/hornet_model/event_hornet_strips_10.inc"
};
static Sint16 event_hornet_strips_11[15] = {
#include "shared/hornet_model/event_hornet_strips_11.inc"
};
static Sint16 event_hornet_strips_12[16] = {
#include "shared/hornet_model/event_hornet_strips_12.inc"
};
static NJS_COLOR event_hornet_colours_8[10] = {
#include "shared/hornet_model/event_hornet_colours_8.inc"
};
static NJS_COLOR event_hornet_colours_9[14] = {
#include "shared/hornet_model/event_hornet_colours_9.inc"
};
static NJS_COLOR event_hornet_colours_10[24] = {
#include "shared/hornet_model/event_hornet_colours_10.inc"
};
static NJS_COLOR event_hornet_colours_11[14] = {
#include "shared/hornet_model/event_hornet_colours_11.inc"
};
static NJS_COLOR event_hornet_colours_12[14] = {
#include "shared/hornet_model/event_hornet_colours_12.inc"
};
static NJS_MESHSET event_hornet_meshsets_2[5] = {
#include "shared/hornet_model/event_hornet_meshsets_2.inc"
};
static NJS_POINT3 event_hornet_points_2[31] = {
#include "shared/hornet_model/event_hornet_points_2.inc"
};
static NJS_VECTOR event_hornet_normals_2[31] = {
#include "shared/hornet_model/event_hornet_normals_2.inc"
};
static NJS_MODEL event_hornet_model_2 = {
#include "shared/hornet_model/event_hornet_model_2.inc"
};
static NJS_OBJECT event_hornet_node_2 = {
#include "shared/hornet_model/event_hornet_node_2.inc"
};
static NJS_MATERIAL event_hornet_materials_3[5] = {
#include "shared/hornet_model/event_hornet_materials_3.inc"
};
static Sint16 event_hornet_strips_13[15] = {
#include "shared/hornet_model/event_hornet_strips_13.inc"
};
static Sint16 event_hornet_strips_14[9] = {
#include "shared/hornet_model/event_hornet_strips_14.inc"
};
static Sint16 event_hornet_strips_15[15] = {
#include "shared/hornet_model/event_hornet_strips_15.inc"
};
static Sint16 event_hornet_strips_16[9] = {
#include "shared/hornet_model/event_hornet_strips_16.inc"
};
static Sint16 event_hornet_strips_17[10] = {
#include "shared/hornet_model/event_hornet_strips_17.inc"
};
static NJS_COLOR event_hornet_colours_13[12] = {
#include "shared/hornet_model/event_hornet_colours_13.inc"
};
static NJS_COLOR event_hornet_colours_14[8] = {
#include "shared/hornet_model/event_hornet_colours_14.inc"
};
static NJS_COLOR event_hornet_colours_15[12] = {
#include "shared/hornet_model/event_hornet_colours_15.inc"
};
static NJS_COLOR event_hornet_colours_16[8] = {
#include "shared/hornet_model/event_hornet_colours_16.inc"
};
static NJS_MESHSET event_hornet_meshsets_3[5] = {
#include "shared/hornet_model/event_hornet_meshsets_3.inc"
};
static NJS_POINT3 event_hornet_points_3[20] = {
#include "shared/hornet_model/event_hornet_points_3.inc"
};
static NJS_VECTOR event_hornet_normals_3[20] = {
#include "shared/hornet_model/event_hornet_normals_3.inc"
};
static NJS_MODEL event_hornet_model_3 = {
#include "shared/hornet_model/event_hornet_model_3.inc"
};
static NJS_OBJECT event_hornet_node_3 = {
#include "shared/hornet_model/event_hornet_node_3.inc"
};
static NJS_MATERIAL event_hornet_materials_4[2] = {
#include "shared/hornet_model/event_hornet_materials_4.inc"
};
static Sint16 event_hornet_strips_18[12] = {
#include "shared/hornet_model/event_hornet_strips_18.inc"
};
static Sint16 event_hornet_strips_19[16] = {
#include "shared/hornet_model/event_hornet_strips_19.inc"
};
static NJS_COLOR event_hornet_colours_17[10] = {
#include "shared/hornet_model/event_hornet_colours_17.inc"
};
static NJS_COLOR event_hornet_colours_18[14] = {
#include "shared/hornet_model/event_hornet_colours_18.inc"
};
static NJS_MESHSET event_hornet_meshsets_4[2] = {
#include "shared/hornet_model/event_hornet_meshsets_4.inc"
};
static NJS_POINT3 event_hornet_points_4[13] = {
#include "shared/hornet_model/event_hornet_points_4.inc"
};
static NJS_VECTOR event_hornet_normals_4[13] = {
#include "shared/hornet_model/event_hornet_normals_4.inc"
};
static NJS_MODEL event_hornet_model_4 = {
#include "shared/hornet_model/event_hornet_model_4.inc"
};
static NJS_OBJECT event_hornet_node_4 = {
#include "shared/hornet_model/event_hornet_node_4.inc"
};
static NJS_MATERIAL event_hornet_materials_5[6] = {
#include "shared/hornet_model/event_hornet_materials_5.inc"
};
static Sint16 event_hornet_strips_20[19] = {
#include "shared/hornet_model/event_hornet_strips_20.inc"
};
static Sint16 event_hornet_strips_21[15] = {
#include "shared/hornet_model/event_hornet_strips_21.inc"
};
static Sint16 event_hornet_strips_22[9] = {
#include "shared/hornet_model/event_hornet_strips_22.inc"
};
static Sint16 event_hornet_strips_23[9] = {
#include "shared/hornet_model/event_hornet_strips_23.inc"
};
static Sint16 event_hornet_strips_24[13] = {
#include "shared/hornet_model/event_hornet_strips_24.inc"
};
static Sint16 event_hornet_strips_25[21] = {
#include "shared/hornet_model/event_hornet_strips_25.inc"
};
static NJS_COLOR event_hornet_colours_19[15] = {
#include "shared/hornet_model/event_hornet_colours_19.inc"
};
static NJS_COLOR event_hornet_colours_20[14] = {
#include "shared/hornet_model/event_hornet_colours_20.inc"
};
static NJS_COLOR event_hornet_colours_21[8] = {
#include "shared/hornet_model/event_hornet_colours_21.inc"
};
static NJS_COLOR event_hornet_colours_22[8] = {
#include "shared/hornet_model/event_hornet_colours_22.inc"
};
static NJS_COLOR event_hornet_colours_23[12] = {
#include "shared/hornet_model/event_hornet_colours_23.inc"
};
static NJS_COLOR event_hornet_colours_24[17] = {
#include "shared/hornet_model/event_hornet_colours_24.inc"
};
static NJS_MESHSET event_hornet_meshsets_5[6] = {
#include "shared/hornet_model/event_hornet_meshsets_5.inc"
};
static NJS_POINT3 event_hornet_points_5[28] = {
#include "shared/hornet_model/event_hornet_points_5.inc"
};
static NJS_VECTOR event_hornet_normals_5[28] = {
#include "shared/hornet_model/event_hornet_normals_5.inc"
};
static NJS_MODEL event_hornet_model_5 = {
#include "shared/hornet_model/event_hornet_model_5.inc"
};
static NJS_OBJECT event_hornet_node_5 = {
#include "shared/hornet_model/event_hornet_node_5.inc"
};
static NJS_MATERIAL event_hornet_materials_6[2] = {
#include "shared/hornet_model/event_hornet_materials_6.inc"
};
static Sint16 event_hornet_strips_26[10] = {
#include "shared/hornet_model/event_hornet_strips_26.inc"
};
static Sint16 event_hornet_strips_27[8] = {
#include "shared/hornet_model/event_hornet_strips_27.inc"
};
static NJS_COLOR event_hornet_colours_25[8] = {
#include "shared/hornet_model/event_hornet_colours_25.inc"
};
static NJS_COLOR event_hornet_colours_26[6] = {
#include "shared/hornet_model/event_hornet_colours_26.inc"
};
static NJS_MESHSET event_hornet_meshsets_6[2] = {
#include "shared/hornet_model/event_hornet_meshsets_6.inc"
};
static NJS_POINT3 event_hornet_points_6[8] = {
#include "shared/hornet_model/event_hornet_points_6.inc"
};
static NJS_VECTOR event_hornet_normals_6[8] = {
#include "shared/hornet_model/event_hornet_normals_6.inc"
};
static NJS_MODEL event_hornet_model_6 = {
#include "shared/hornet_model/event_hornet_model_6.inc"
};
static NJS_OBJECT event_hornet_node_6 = {
#include "shared/hornet_model/event_hornet_node_6.inc"
};
static NJS_MATERIAL event_hornet_materials_7[8] = {
#include "shared/hornet_model/event_hornet_materials_7.inc"
};
static Sint16 event_hornet_strips_28[24] = {
#include "shared/hornet_model/event_hornet_strips_28.inc"
};
static Sint16 event_hornet_strips_29[15] = {
#include "shared/hornet_model/event_hornet_strips_29.inc"
};
static Sint16 event_hornet_strips_30[30] = {
#include "shared/hornet_model/event_hornet_strips_30.inc"
};
static Sint16 event_hornet_strips_31[24] = {
#include "shared/hornet_model/event_hornet_strips_31.inc"
};
static Sint16 event_hornet_strips_32[15] = {
#include "shared/hornet_model/event_hornet_strips_32.inc"
};
static Sint16 event_hornet_strips_33[30] = {
#include "shared/hornet_model/event_hornet_strips_33.inc"
};
static Sint16 event_hornet_strips_34[15] = {
#include "shared/hornet_model/event_hornet_strips_34.inc"
};
static Sint16 event_hornet_strips_35[15] = {
#include "shared/hornet_model/event_hornet_strips_35.inc"
};
static NJS_COLOR event_hornet_colours_27[18] = {
#include "shared/hornet_model/event_hornet_colours_27.inc"
};
static NJS_COLOR event_hornet_colours_28[14] = {
#include "shared/hornet_model/event_hornet_colours_28.inc"
};
static NJS_COLOR event_hornet_colours_29[28] = {
#include "shared/hornet_model/event_hornet_colours_29.inc"
};
static NJS_COLOR event_hornet_colours_30[18] = {
#include "shared/hornet_model/event_hornet_colours_30.inc"
};
static NJS_COLOR event_hornet_colours_31[14] = {
#include "shared/hornet_model/event_hornet_colours_31.inc"
};
static NJS_COLOR event_hornet_colours_32[24] = {
#include "shared/hornet_model/event_hornet_colours_32.inc"
};
static NJS_COLOR event_hornet_colours_33[14] = {
#include "shared/hornet_model/event_hornet_colours_33.inc"
};
static NJS_COLOR event_hornet_colours_34[14] = {
#include "shared/hornet_model/event_hornet_colours_34.inc"
};
static NJS_MESHSET event_hornet_meshsets_7[8] = {
#include "shared/hornet_model/event_hornet_meshsets_7.inc"
};
static NJS_POINT3 event_hornet_points_7[54] = {
#include "shared/hornet_model/event_hornet_points_7.inc"
};
static NJS_VECTOR event_hornet_normals_7[54] = {
#include "shared/hornet_model/event_hornet_normals_7.inc"
};
static NJS_MODEL event_hornet_model_7 = {
#include "shared/hornet_model/event_hornet_model_7.inc"
};
static NJS_OBJECT event_hornet_node_7 = {
#include "shared/hornet_model/event_hornet_node_7.inc"
};
static NJS_MATERIAL event_hornet_materials_8[2] = {
#include "shared/hornet_model/event_hornet_materials_8.inc"
};
static Sint16 event_hornet_strips_36[24] = {
#include "shared/hornet_model/event_hornet_strips_36.inc"
};
static Sint16 event_hornet_strips_37[16] = {
#include "shared/hornet_model/event_hornet_strips_37.inc"
};
static NJS_COLOR event_hornet_colours_35[18] = {
#include "shared/hornet_model/event_hornet_colours_35.inc"
};
static NJS_COLOR event_hornet_colours_36[14] = {
#include "shared/hornet_model/event_hornet_colours_36.inc"
};
static NJS_MESHSET event_hornet_meshsets_8[2] = {
#include "shared/hornet_model/event_hornet_meshsets_8.inc"
};
static NJS_POINT3 event_hornet_points_8[13] = {
#include "shared/hornet_model/event_hornet_points_8.inc"
};
static NJS_VECTOR event_hornet_normals_8[13] = {
#include "shared/hornet_model/event_hornet_normals_8.inc"
};
static NJS_MODEL event_hornet_model_8 = {
#include "shared/hornet_model/event_hornet_model_8.inc"
};
static NJS_OBJECT event_hornet_node_8 = {
#include "shared/hornet_model/event_hornet_node_8.inc"
};
static NJS_MATERIAL event_hornet_materials_9[2] = {
#include "shared/hornet_model/event_hornet_materials_9.inc"
};
static Sint16 event_hornet_strips_38[12] = {
#include "shared/hornet_model/event_hornet_strips_38.inc"
};
static Sint16 event_hornet_strips_39[16] = {
#include "shared/hornet_model/event_hornet_strips_39.inc"
};
static NJS_COLOR event_hornet_colours_37[10] = {
#include "shared/hornet_model/event_hornet_colours_37.inc"
};
static NJS_COLOR event_hornet_colours_38[14] = {
#include "shared/hornet_model/event_hornet_colours_38.inc"
};
static NJS_MESHSET event_hornet_meshsets_9[2] = {
#include "shared/hornet_model/event_hornet_meshsets_9.inc"
};
static NJS_POINT3 event_hornet_points_9[13] = {
#include "shared/hornet_model/event_hornet_points_9.inc"
};
static NJS_VECTOR event_hornet_normals_9[13] = {
#include "shared/hornet_model/event_hornet_normals_9.inc"
};
static NJS_MODEL event_hornet_model_9 = {
#include "shared/hornet_model/event_hornet_model_9.inc"
};
static NJS_OBJECT event_hornet_node_9 = {
#include "shared/hornet_model/event_hornet_node_9.inc"
};
static NJS_MATERIAL event_hornet_materials_10[6] = {
#include "shared/hornet_model/event_hornet_materials_10.inc"
};
static Sint16 event_hornet_strips_40[19] = {
#include "shared/hornet_model/event_hornet_strips_40.inc"
};
static Sint16 event_hornet_strips_41[15] = {
#include "shared/hornet_model/event_hornet_strips_41.inc"
};
static Sint16 event_hornet_strips_42[9] = {
#include "shared/hornet_model/event_hornet_strips_42.inc"
};
static Sint16 event_hornet_strips_43[9] = {
#include "shared/hornet_model/event_hornet_strips_43.inc"
};
static Sint16 event_hornet_strips_44[13] = {
#include "shared/hornet_model/event_hornet_strips_44.inc"
};
static Sint16 event_hornet_strips_45[21] = {
#include "shared/hornet_model/event_hornet_strips_45.inc"
};
static NJS_COLOR event_hornet_colours_39[15] = {
#include "shared/hornet_model/event_hornet_colours_39.inc"
};
static NJS_COLOR event_hornet_colours_40[14] = {
#include "shared/hornet_model/event_hornet_colours_40.inc"
};
static NJS_COLOR event_hornet_colours_41[8] = {
#include "shared/hornet_model/event_hornet_colours_41.inc"
};
static NJS_COLOR event_hornet_colours_42[8] = {
#include "shared/hornet_model/event_hornet_colours_42.inc"
};
static NJS_COLOR event_hornet_colours_43[12] = {
#include "shared/hornet_model/event_hornet_colours_43.inc"
};
static NJS_COLOR event_hornet_colours_44[17] = {
#include "shared/hornet_model/event_hornet_colours_44.inc"
};
static NJS_MESHSET event_hornet_meshsets_10[6] = {
#include "shared/hornet_model/event_hornet_meshsets_10.inc"
};
static NJS_POINT3 event_hornet_points_10[28] = {
#include "shared/hornet_model/event_hornet_points_10.inc"
};
static NJS_VECTOR event_hornet_normals_10[28] = {
#include "shared/hornet_model/event_hornet_normals_10.inc"
};
static NJS_MODEL event_hornet_model_10 = {
#include "shared/hornet_model/event_hornet_model_10.inc"
};
static NJS_OBJECT event_hornet_node_10 = {
#include "shared/hornet_model/event_hornet_node_10.inc"
};
static NJS_MATERIAL event_hornet_materials_11[2] = {
#include "shared/hornet_model/event_hornet_materials_11.inc"
};
static Sint16 event_hornet_strips_46[10] = {
#include "shared/hornet_model/event_hornet_strips_46.inc"
};
static Sint16 event_hornet_strips_47[8] = {
#include "shared/hornet_model/event_hornet_strips_47.inc"
};
static NJS_COLOR event_hornet_colours_45[8] = {
#include "shared/hornet_model/event_hornet_colours_45.inc"
};
static NJS_COLOR event_hornet_colours_46[6] = {
#include "shared/hornet_model/event_hornet_colours_46.inc"
};
static NJS_MESHSET event_hornet_meshsets_11[2] = {
#include "shared/hornet_model/event_hornet_meshsets_11.inc"
};
static NJS_POINT3 event_hornet_points_11[8] = {
#include "shared/hornet_model/event_hornet_points_11.inc"
};
static NJS_VECTOR event_hornet_normals_11[8] = {
#include "shared/hornet_model/event_hornet_normals_11.inc"
};
static NJS_MODEL event_hornet_model_11 = {
#include "shared/hornet_model/event_hornet_model_11.inc"
};
static NJS_OBJECT event_hornet_node_11 = {
#include "shared/hornet_model/event_hornet_node_11.inc"
};
static NJS_MATERIAL event_hornet_materials_12[8] = {
#include "shared/hornet_model/event_hornet_materials_12.inc"
};
static Sint16 event_hornet_strips_48[24] = {
#include "shared/hornet_model/event_hornet_strips_48.inc"
};
static Sint16 event_hornet_strips_49[15] = {
#include "shared/hornet_model/event_hornet_strips_49.inc"
};
static Sint16 event_hornet_strips_50[30] = {
#include "shared/hornet_model/event_hornet_strips_50.inc"
};
static Sint16 event_hornet_strips_51[24] = {
#include "shared/hornet_model/event_hornet_strips_51.inc"
};
static Sint16 event_hornet_strips_52[15] = {
#include "shared/hornet_model/event_hornet_strips_52.inc"
};
static Sint16 event_hornet_strips_53[30] = {
#include "shared/hornet_model/event_hornet_strips_53.inc"
};
static Sint16 event_hornet_strips_54[15] = {
#include "shared/hornet_model/event_hornet_strips_54.inc"
};
static Sint16 event_hornet_strips_55[15] = {
#include "shared/hornet_model/event_hornet_strips_55.inc"
};
static NJS_COLOR event_hornet_colours_47[18] = {
#include "shared/hornet_model/event_hornet_colours_47.inc"
};
static NJS_COLOR event_hornet_colours_48[14] = {
#include "shared/hornet_model/event_hornet_colours_48.inc"
};
static NJS_COLOR event_hornet_colours_49[28] = {
#include "shared/hornet_model/event_hornet_colours_49.inc"
};
static NJS_COLOR event_hornet_colours_50[18] = {
#include "shared/hornet_model/event_hornet_colours_50.inc"
};
static NJS_COLOR event_hornet_colours_51[14] = {
#include "shared/hornet_model/event_hornet_colours_51.inc"
};
static NJS_COLOR event_hornet_colours_52[24] = {
#include "shared/hornet_model/event_hornet_colours_52.inc"
};
static NJS_COLOR event_hornet_colours_53[14] = {
#include "shared/hornet_model/event_hornet_colours_53.inc"
};
static NJS_COLOR event_hornet_colours_54[14] = {
#include "shared/hornet_model/event_hornet_colours_54.inc"
};
static NJS_MESHSET event_hornet_meshsets_12[8] = {
#include "shared/hornet_model/event_hornet_meshsets_12.inc"
};
static NJS_POINT3 event_hornet_points_12[54] = {
#include "shared/hornet_model/event_hornet_points_12.inc"
};
static NJS_VECTOR event_hornet_normals_12[54] = {
#include "shared/hornet_model/event_hornet_normals_12.inc"
};
static NJS_MODEL event_hornet_model_12 = {
#include "shared/hornet_model/event_hornet_model_12.inc"
};
static NJS_OBJECT event_hornet_node_12 = {
#include "shared/hornet_model/event_hornet_node_12.inc"
};
static NJS_MATERIAL event_hornet_materials_13[2] = {
#include "shared/hornet_model/event_hornet_materials_13.inc"
};
static Sint16 event_hornet_strips_56[24] = {
#include "shared/hornet_model/event_hornet_strips_56.inc"
};
static Sint16 event_hornet_strips_57[16] = {
#include "shared/hornet_model/event_hornet_strips_57.inc"
};
static NJS_COLOR event_hornet_colours_55[18] = {
#include "shared/hornet_model/event_hornet_colours_55.inc"
};
static NJS_COLOR event_hornet_colours_56[14] = {
#include "shared/hornet_model/event_hornet_colours_56.inc"
};
static NJS_MESHSET event_hornet_meshsets_13[2] = {
#include "shared/hornet_model/event_hornet_meshsets_13.inc"
};
static NJS_POINT3 event_hornet_points_13[13] = {
#include "shared/hornet_model/event_hornet_points_13.inc"
};
static NJS_VECTOR event_hornet_normals_13[13] = {
#include "shared/hornet_model/event_hornet_normals_13.inc"
};
static NJS_MODEL event_hornet_model_13 = {
#include "shared/hornet_model/event_hornet_model_13.inc"
};
NJS_OBJECT event_hornet_1 = {
#include "shared/hornet_model/event_hornet_1.inc"
};
static NJS_MATERIAL event_hornet_materials_14[21] = {
#include "shared/hornet_model/event_hornet_materials_14.inc"
};
static Sint16 event_hornet_strips_58[71] = {
#include "shared/hornet_model/event_hornet_strips_58.inc"
};
static Sint16 event_hornet_strips_59[106] = {
#include "shared/hornet_model/event_hornet_strips_59.inc"
};
static Sint16 event_hornet_strips_60[22] = {
#include "shared/hornet_model/event_hornet_strips_60.inc"
};
static Sint16 event_hornet_strips_61[31] = {
#include "shared/hornet_model/event_hornet_strips_61.inc"
};
static Sint16 event_hornet_strips_62[5] = {
#include "shared/hornet_model/event_hornet_strips_62.inc"
};
static Sint16 event_hornet_strips_63[10] = {
#include "shared/hornet_model/event_hornet_strips_63.inc"
};
static Sint16 event_hornet_strips_64[18] = {
#include "shared/hornet_model/event_hornet_strips_64.inc"
};
static Sint16 event_hornet_strips_65[16] = {
#include "shared/hornet_model/event_hornet_strips_65.inc"
};
static Sint16 event_hornet_strips_66[15] = {
#include "shared/hornet_model/event_hornet_strips_66.inc"
};
static Sint16 event_hornet_strips_67[8] = {
#include "shared/hornet_model/event_hornet_strips_67.inc"
};
static Sint16 event_hornet_strips_68[19] = {
#include "shared/hornet_model/event_hornet_strips_68.inc"
};
static Sint16 event_hornet_strips_69[93] = {
#include "shared/hornet_model/event_hornet_strips_69.inc"
};
static Sint16 event_hornet_strips_70[10] = {
#include "shared/hornet_model/event_hornet_strips_70.inc"
};
static Sint16 event_hornet_strips_71[10] = {
#include "shared/hornet_model/event_hornet_strips_71.inc"
};
static Sint16 event_hornet_strips_72[10] = {
#include "shared/hornet_model/event_hornet_strips_72.inc"
};
static Sint16 event_hornet_strips_73[20] = {
#include "shared/hornet_model/event_hornet_strips_73.inc"
};
static Sint16 event_hornet_strips_74[36] = {
#include "shared/hornet_model/event_hornet_strips_74.inc"
};
static Sint16 event_hornet_strips_75[36] = {
#include "shared/hornet_model/event_hornet_strips_75.inc"
};
static Sint16 event_hornet_strips_76[12] = {
#include "shared/hornet_model/event_hornet_strips_76.inc"
};
static Sint16 event_hornet_strips_77[30] = {
#include "shared/hornet_model/event_hornet_strips_77.inc"
};
static Sint16 event_hornet_strips_78[6] = {
#include "shared/hornet_model/event_hornet_strips_78.inc"
};
static NJS_COLOR event_hornet_colours_57[62] = {
#include "shared/hornet_model/event_hornet_colours_57.inc"
};
static NJS_COLOR event_hornet_colours_58[94] = {
#include "shared/hornet_model/event_hornet_colours_58.inc"
};
static NJS_COLOR event_hornet_colours_59[18] = {
#include "shared/hornet_model/event_hornet_colours_59.inc"
};
static NJS_COLOR event_hornet_colours_60[24] = {
#include "shared/hornet_model/event_hornet_colours_60.inc"
};
static NJS_COLOR event_hornet_colours_61[4] = {
#include "shared/hornet_model/event_hornet_colours_61.inc"
};
static NJS_COLOR event_hornet_colours_62[8] = {
#include "shared/hornet_model/event_hornet_colours_62.inc"
};
static NJS_COLOR event_hornet_colours_63[14] = {
#include "shared/hornet_model/event_hornet_colours_63.inc"
};
static NJS_COLOR event_hornet_colours_64[14] = {
#include "shared/hornet_model/event_hornet_colours_64.inc"
};
static NJS_COLOR event_hornet_colours_65[12] = {
#include "shared/hornet_model/event_hornet_colours_65.inc"
};
static NJS_COLOR event_hornet_colours_66[6] = {
#include "shared/hornet_model/event_hornet_colours_66.inc"
};
static NJS_COLOR event_hornet_colours_67[16] = {
#include "shared/hornet_model/event_hornet_colours_67.inc"
};
static NJS_COLOR event_hornet_colours_68[83] = {
#include "shared/hornet_model/event_hornet_colours_68.inc"
};
static NJS_COLOR event_hornet_colours_69[8] = {
#include "shared/hornet_model/event_hornet_colours_69.inc"
};
static NJS_COLOR event_hornet_colours_70[8] = {
#include "shared/hornet_model/event_hornet_colours_70.inc"
};
static NJS_COLOR event_hornet_colours_71[8] = {
#include "shared/hornet_model/event_hornet_colours_71.inc"
};
static NJS_COLOR event_hornet_colours_72[16] = {
#include "shared/hornet_model/event_hornet_colours_72.inc"
};
static NJS_COLOR event_hornet_colours_73[32] = {
#include "shared/hornet_model/event_hornet_colours_73.inc"
};
static NJS_COLOR event_hornet_colours_74[32] = {
#include "shared/hornet_model/event_hornet_colours_74.inc"
};
static NJS_COLOR event_hornet_colours_75[10] = {
#include "shared/hornet_model/event_hornet_colours_75.inc"
};
static NJS_COLOR event_hornet_colours_76[28] = {
#include "shared/hornet_model/event_hornet_colours_76.inc"
};
static NJS_COLOR event_hornet_colours_77[4] = {
#include "shared/hornet_model/event_hornet_colours_77.inc"
};
static NJS_MESHSET event_hornet_meshsets_14[21] = {
#include "shared/hornet_model/event_hornet_meshsets_14.inc"
};
static NJS_POINT3 event_hornet_points_14[239] = {
#include "shared/hornet_model/event_hornet_points_14.inc"
};
static NJS_VECTOR event_hornet_normals_14[239] = {
#include "shared/hornet_model/event_hornet_normals_14.inc"
};
NJS_MODEL event_hornet_model = {
#include "shared/hornet_model/event_hornet_model.inc"
};
