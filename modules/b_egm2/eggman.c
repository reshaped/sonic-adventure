/* Eggman in the Egg Walker's cockpit */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm2/eggman.h"
#include "b_egm2/texlists.h"
#include "draw/tree.h"
#include "system/task.h"

static NJS_MATERIAL egm2_eggman_node13_materials[1] = {
#include "b_egm2/eggman/egm2_eggman_node13_materials.inc"
};

static Sint16 egm2_eggman_node13_meshes[14] = {
#include "b_egm2/eggman/egm2_eggman_node13_meshes.inc"
};

static NJS_COLOR egm2_eggman_node13_uvs[12] = {
#include "b_egm2/eggman/egm2_eggman_node13_uvs.inc"
};

static NJS_MESHSET egm2_eggman_node13_meshsets[1] = {
#include "b_egm2/eggman/egm2_eggman_node13_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node13_points[9] = {
#include "b_egm2/eggman/egm2_eggman_node13_points.inc"
};

static NJS_VECTOR egm2_eggman_node13_normals[9] = {
#include "b_egm2/eggman/egm2_eggman_node13_normals.inc"
};

static NJS_MODEL egm2_eggman_node13_model = {
#include "b_egm2/eggman/egm2_eggman_node13_model.inc"
};

static NJS_OBJECT egm2_eggman_node13_object = {
#include "b_egm2/eggman/egm2_eggman_node13_object.inc"
};

static NJS_MATERIAL egm2_eggman_node12_materials[1] = {
#include "b_egm2/eggman/egm2_eggman_node12_materials.inc"
};

static Sint16 egm2_eggman_node12_meshes[20] = {
#include "b_egm2/eggman/egm2_eggman_node12_meshes.inc"
};

static NJS_COLOR egm2_eggman_node12_uvs[18] = {
#include "b_egm2/eggman/egm2_eggman_node12_uvs.inc"
};

static NJS_MESHSET egm2_eggman_node12_meshsets[1] = {
#include "b_egm2/eggman/egm2_eggman_node12_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node12_points[10] = {
#include "b_egm2/eggman/egm2_eggman_node12_points.inc"
};

static NJS_VECTOR egm2_eggman_node12_normals[10] = {
#include "b_egm2/eggman/egm2_eggman_node12_normals.inc"
};

static NJS_MODEL egm2_eggman_node12_model = {
#include "b_egm2/eggman/egm2_eggman_node12_model.inc"
};

static NJS_OBJECT egm2_eggman_node12_object = {
#include "b_egm2/eggman/egm2_eggman_node12_object.inc"
};

static NJS_MATERIAL egm2_eggman_node11_materials[1] = {
#include "b_egm2/eggman/egm2_eggman_node11_materials.inc"
};

static Sint16 egm2_eggman_node11_meshes[19] = {
#include "b_egm2/eggman/egm2_eggman_node11_meshes.inc"
};

static NJS_MESHSET egm2_eggman_node11_meshsets[1] = {
#include "b_egm2/eggman/egm2_eggman_node11_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node11_points[8] = {
#include "b_egm2/eggman/egm2_eggman_node11_points.inc"
};

static NJS_VECTOR egm2_eggman_node11_normals[8] = {
#include "b_egm2/eggman/egm2_eggman_node11_normals.inc"
};

static NJS_MODEL egm2_eggman_node11_model = {
#include "b_egm2/eggman/egm2_eggman_node11_model.inc"
};

static NJS_OBJECT egm2_eggman_node11_object = {
#include "b_egm2/eggman/egm2_eggman_node11_object.inc"
};

static NJS_MATERIAL egm2_eggman_node10_materials[3] = {
#include "b_egm2/eggman/egm2_eggman_node10_materials.inc"
};

static Sint16 egm2_eggman_node10_meshes0[10] = {
#include "b_egm2/eggman/egm2_eggman_node10_meshes0.inc"
};

static Sint16 egm2_eggman_node10_meshes1[8] = {
#include "b_egm2/eggman/egm2_eggman_node10_meshes1.inc"
};

static Sint16 egm2_eggman_node10_meshes2[5] = {
#include "b_egm2/eggman/egm2_eggman_node10_meshes2.inc"
};

static NJS_COLOR egm2_eggman_node10_uvs1[6] = {
#include "b_egm2/eggman/egm2_eggman_node10_uvs1.inc"
};

static NJS_COLOR egm2_eggman_node10_uvs2[4] = {
#include "b_egm2/eggman/egm2_eggman_node10_uvs2.inc"
};

static NJS_MESHSET egm2_eggman_node10_meshsets[3] = {
#include "b_egm2/eggman/egm2_eggman_node10_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node10_points[8] = {
#include "b_egm2/eggman/egm2_eggman_node10_points.inc"
};

static NJS_VECTOR egm2_eggman_node10_normals[8] = {
#include "b_egm2/eggman/egm2_eggman_node10_normals.inc"
};

static NJS_MODEL egm2_eggman_node10_model = {
#include "b_egm2/eggman/egm2_eggman_node10_model.inc"
};

static NJS_OBJECT egm2_eggman_node10_object = {
#include "b_egm2/eggman/egm2_eggman_node10_object.inc"
};

static NJS_MATERIAL egm2_eggman_node9_materials[4] = {
#include "b_egm2/eggman/egm2_eggman_node9_materials.inc"
};

static Sint16 egm2_eggman_node9_meshes0[19] = {
#include "b_egm2/eggman/egm2_eggman_node9_meshes0.inc"
};

static Sint16 egm2_eggman_node9_meshes1[5] = {
#include "b_egm2/eggman/egm2_eggman_node9_meshes1.inc"
};

static Sint16 egm2_eggman_node9_meshes2[11] = {
#include "b_egm2/eggman/egm2_eggman_node9_meshes2.inc"
};

static Sint16 egm2_eggman_node9_meshes3[16] = {
#include "b_egm2/eggman/egm2_eggman_node9_meshes3.inc"
};

static NJS_COLOR egm2_eggman_node9_uvs0[16] = {
#include "b_egm2/eggman/egm2_eggman_node9_uvs0.inc"
};

static NJS_COLOR egm2_eggman_node9_uvs1[4] = {
#include "b_egm2/eggman/egm2_eggman_node9_uvs1.inc"
};

static NJS_COLOR egm2_eggman_node9_uvs2[10] = {
#include "b_egm2/eggman/egm2_eggman_node9_uvs2.inc"
};

static NJS_MESHSET egm2_eggman_node9_meshsets[4] = {
#include "b_egm2/eggman/egm2_eggman_node9_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node9_points[26] = {
#include "b_egm2/eggman/egm2_eggman_node9_points.inc"
};

static NJS_VECTOR egm2_eggman_node9_normals[26] = {
#include "b_egm2/eggman/egm2_eggman_node9_normals.inc"
};

static NJS_MODEL egm2_eggman_node9_model = {
#include "b_egm2/eggman/egm2_eggman_node9_model.inc"
};

static NJS_OBJECT egm2_eggman_node9_object = {
#include "b_egm2/eggman/egm2_eggman_node9_object.inc"
};

static NJS_MATERIAL egm2_eggman_node8_materials[1] = {
#include "b_egm2/eggman/egm2_eggman_node8_materials.inc"
};

static Sint16 egm2_eggman_node8_meshes[52] = {
#include "b_egm2/eggman/egm2_eggman_node8_meshes.inc"
};

static NJS_COLOR egm2_eggman_node8_uvs[41] = {
#include "b_egm2/eggman/egm2_eggman_node8_uvs.inc"
};

static NJS_MESHSET egm2_eggman_node8_meshsets[1] = {
#include "b_egm2/eggman/egm2_eggman_node8_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node8_points[13] = {
#include "b_egm2/eggman/egm2_eggman_node8_points.inc"
};

static NJS_VECTOR egm2_eggman_node8_normals[13] = {
#include "b_egm2/eggman/egm2_eggman_node8_normals.inc"
};

static NJS_MODEL egm2_eggman_node8_model = {
#include "b_egm2/eggman/egm2_eggman_node8_model.inc"
};

static NJS_OBJECT egm2_eggman_node8_object = {
#include "b_egm2/eggman/egm2_eggman_node8_object.inc"
};

static NJS_MATERIAL egm2_eggman_node7_materials[1] = {
#include "b_egm2/eggman/egm2_eggman_node7_materials.inc"
};

static Sint16 egm2_eggman_node7_meshes[20] = {
#include "b_egm2/eggman/egm2_eggman_node7_meshes.inc"
};

static NJS_COLOR egm2_eggman_node7_uvs[18] = {
#include "b_egm2/eggman/egm2_eggman_node7_uvs.inc"
};

static NJS_MESHSET egm2_eggman_node7_meshsets[1] = {
#include "b_egm2/eggman/egm2_eggman_node7_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node7_points[10] = {
#include "b_egm2/eggman/egm2_eggman_node7_points.inc"
};

static NJS_VECTOR egm2_eggman_node7_normals[10] = {
#include "b_egm2/eggman/egm2_eggman_node7_normals.inc"
};

static NJS_MODEL egm2_eggman_node7_model = {
#include "b_egm2/eggman/egm2_eggman_node7_model.inc"
};

static NJS_OBJECT egm2_eggman_node7_object = {
#include "b_egm2/eggman/egm2_eggman_node7_object.inc"
};

static NJS_MATERIAL egm2_eggman_node6_materials[1] = {
#include "b_egm2/eggman/egm2_eggman_node6_materials.inc"
};

static Sint16 egm2_eggman_node6_meshes[19] = {
#include "b_egm2/eggman/egm2_eggman_node6_meshes.inc"
};

static NJS_MESHSET egm2_eggman_node6_meshsets[1] = {
#include "b_egm2/eggman/egm2_eggman_node6_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node6_points[8] = {
#include "b_egm2/eggman/egm2_eggman_node6_points.inc"
};

static NJS_VECTOR egm2_eggman_node6_normals[8] = {
#include "b_egm2/eggman/egm2_eggman_node6_normals.inc"
};

static NJS_MODEL egm2_eggman_node6_model = {
#include "b_egm2/eggman/egm2_eggman_node6_model.inc"
};

static NJS_OBJECT egm2_eggman_node6_object = {
#include "b_egm2/eggman/egm2_eggman_node6_object.inc"
};

static NJS_MATERIAL egm2_eggman_node5_materials[3] = {
#include "b_egm2/eggman/egm2_eggman_node5_materials.inc"
};

static Sint16 egm2_eggman_node5_meshes0[10] = {
#include "b_egm2/eggman/egm2_eggman_node5_meshes0.inc"
};

static Sint16 egm2_eggman_node5_meshes1[8] = {
#include "b_egm2/eggman/egm2_eggman_node5_meshes1.inc"
};

static Sint16 egm2_eggman_node5_meshes2[5] = {
#include "b_egm2/eggman/egm2_eggman_node5_meshes2.inc"
};

static NJS_COLOR egm2_eggman_node5_uvs1[6] = {
#include "b_egm2/eggman/egm2_eggman_node5_uvs1.inc"
};

static NJS_COLOR egm2_eggman_node5_uvs2[4] = {
#include "b_egm2/eggman/egm2_eggman_node5_uvs2.inc"
};

static NJS_MESHSET egm2_eggman_node5_meshsets[3] = {
#include "b_egm2/eggman/egm2_eggman_node5_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node5_points[8] = {
#include "b_egm2/eggman/egm2_eggman_node5_points.inc"
};

static NJS_VECTOR egm2_eggman_node5_normals[8] = {
#include "b_egm2/eggman/egm2_eggman_node5_normals.inc"
};

static NJS_MODEL egm2_eggman_node5_model = {
#include "b_egm2/eggman/egm2_eggman_node5_model.inc"
};

static NJS_OBJECT egm2_eggman_node5_object = {
#include "b_egm2/eggman/egm2_eggman_node5_object.inc"
};

static NJS_MATERIAL egm2_eggman_node4_materials[4] = {
#include "b_egm2/eggman/egm2_eggman_node4_materials.inc"
};

static Sint16 egm2_eggman_node4_meshes0[19] = {
#include "b_egm2/eggman/egm2_eggman_node4_meshes0.inc"
};

static Sint16 egm2_eggman_node4_meshes1[5] = {
#include "b_egm2/eggman/egm2_eggman_node4_meshes1.inc"
};

static Sint16 egm2_eggman_node4_meshes2[11] = {
#include "b_egm2/eggman/egm2_eggman_node4_meshes2.inc"
};

static Sint16 egm2_eggman_node4_meshes3[16] = {
#include "b_egm2/eggman/egm2_eggman_node4_meshes3.inc"
};

static NJS_COLOR egm2_eggman_node4_uvs0[16] = {
#include "b_egm2/eggman/egm2_eggman_node4_uvs0.inc"
};

static NJS_COLOR egm2_eggman_node4_uvs1[4] = {
#include "b_egm2/eggman/egm2_eggman_node4_uvs1.inc"
};

static NJS_COLOR egm2_eggman_node4_uvs2[10] = {
#include "b_egm2/eggman/egm2_eggman_node4_uvs2.inc"
};

static NJS_MESHSET egm2_eggman_node4_meshsets[4] = {
#include "b_egm2/eggman/egm2_eggman_node4_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node4_points[26] = {
#include "b_egm2/eggman/egm2_eggman_node4_points.inc"
};

static NJS_VECTOR egm2_eggman_node4_normals[26] = {
#include "b_egm2/eggman/egm2_eggman_node4_normals.inc"
};

static NJS_MODEL egm2_eggman_node4_model = {
#include "b_egm2/eggman/egm2_eggman_node4_model.inc"
};

static NJS_OBJECT egm2_eggman_node4_object = {
#include "b_egm2/eggman/egm2_eggman_node4_object.inc"
};

static NJS_MATERIAL egm2_eggman_node3_materials[1] = {
#include "b_egm2/eggman/egm2_eggman_node3_materials.inc"
};

static Sint16 egm2_eggman_node3_meshes[52] = {
#include "b_egm2/eggman/egm2_eggman_node3_meshes.inc"
};

static NJS_COLOR egm2_eggman_node3_uvs[41] = {
#include "b_egm2/eggman/egm2_eggman_node3_uvs.inc"
};

static NJS_MESHSET egm2_eggman_node3_meshsets[1] = {
#include "b_egm2/eggman/egm2_eggman_node3_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node3_points[13] = {
#include "b_egm2/eggman/egm2_eggman_node3_points.inc"
};

static NJS_VECTOR egm2_eggman_node3_normals[13] = {
#include "b_egm2/eggman/egm2_eggman_node3_normals.inc"
};

static NJS_MODEL egm2_eggman_node3_model = {
#include "b_egm2/eggman/egm2_eggman_node3_model.inc"
};

static NJS_OBJECT egm2_eggman_node3_object = {
#include "b_egm2/eggman/egm2_eggman_node3_object.inc"
};

static NJS_MATERIAL egm2_eggman_node2_materials[1] = {
#include "b_egm2/eggman/egm2_eggman_node2_materials.inc"
};

static Sint16 egm2_eggman_node2_meshes[35] = {
#include "b_egm2/eggman/egm2_eggman_node2_meshes.inc"
};

static NJS_COLOR egm2_eggman_node2_uvs[29] = {
#include "b_egm2/eggman/egm2_eggman_node2_uvs.inc"
};

static NJS_MESHSET egm2_eggman_node2_meshsets[1] = {
#include "b_egm2/eggman/egm2_eggman_node2_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node2_points[15] = {
#include "b_egm2/eggman/egm2_eggman_node2_points.inc"
};

static NJS_VECTOR egm2_eggman_node2_normals[15] = {
#include "b_egm2/eggman/egm2_eggman_node2_normals.inc"
};

static NJS_MODEL egm2_eggman_node2_model = {
#include "b_egm2/eggman/egm2_eggman_node2_model.inc"
};

static NJS_OBJECT egm2_eggman_node2_object = {
#include "b_egm2/eggman/egm2_eggman_node2_object.inc"
};

static NJS_MATERIAL egm2_eggman_node1_materials[5] = {
#include "b_egm2/eggman/egm2_eggman_node1_materials.inc"
};

static Sint16 egm2_eggman_node1_meshes0[167] = {
#include "b_egm2/eggman/egm2_eggman_node1_meshes0.inc"
};

static Sint16 egm2_eggman_node1_meshes1[11] = {
#include "b_egm2/eggman/egm2_eggman_node1_meshes1.inc"
};

static Sint16 egm2_eggman_node1_meshes2[13] = {
#include "b_egm2/eggman/egm2_eggman_node1_meshes2.inc"
};

static Sint16 egm2_eggman_node1_meshes3[38] = {
#include "b_egm2/eggman/egm2_eggman_node1_meshes3.inc"
};

static Sint16 egm2_eggman_node1_meshes4[20] = {
#include "b_egm2/eggman/egm2_eggman_node1_meshes4.inc"
};

static NJS_COLOR egm2_eggman_node1_uvs3[30] = {
#include "b_egm2/eggman/egm2_eggman_node1_uvs3.inc"
};

static NJS_COLOR egm2_eggman_node1_uvs4[16] = {
#include "b_egm2/eggman/egm2_eggman_node1_uvs4.inc"
};

static NJS_MESHSET egm2_eggman_node1_meshsets[5] = {
#include "b_egm2/eggman/egm2_eggman_node1_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_node1_points[107] = {
#include "b_egm2/eggman/egm2_eggman_node1_points.inc"
};

static NJS_VECTOR egm2_eggman_node1_normals[107] = {
#include "b_egm2/eggman/egm2_eggman_node1_normals.inc"
};

static NJS_MODEL egm2_eggman_node1_model = {
#include "b_egm2/eggman/egm2_eggman_node1_model.inc"
};

static NJS_OBJECT egm2_eggman_node1_object = {
#include "b_egm2/eggman/egm2_eggman_node1_object.inc"
};

static NJS_MATERIAL egm2_eggman_materials[4] = {
#include "b_egm2/eggman/egm2_eggman_materials.inc"
};

static Sint16 egm2_eggman_meshes0[14] = {
#include "b_egm2/eggman/egm2_eggman_meshes0.inc"
};

static Sint16 egm2_eggman_meshes1[15] = {
#include "b_egm2/eggman/egm2_eggman_meshes1.inc"
};

static Sint16 egm2_eggman_meshes2[97] = {
#include "b_egm2/eggman/egm2_eggman_meshes2.inc"
};

static Sint16 egm2_eggman_meshes3[12] = {
#include "b_egm2/eggman/egm2_eggman_meshes3.inc"
};

static NJS_COLOR egm2_eggman_uvs0[12] = {
#include "b_egm2/eggman/egm2_eggman_uvs0.inc"
};

static NJS_COLOR egm2_eggman_uvs1[12] = {
#include "b_egm2/eggman/egm2_eggman_uvs1.inc"
};

static NJS_COLOR egm2_eggman_uvs2[80] = {
#include "b_egm2/eggman/egm2_eggman_uvs2.inc"
};

static NJS_COLOR egm2_eggman_uvs3[10] = {
#include "b_egm2/eggman/egm2_eggman_uvs3.inc"
};

static NJS_MESHSET egm2_eggman_meshsets[4] = {
#include "b_egm2/eggman/egm2_eggman_meshsets.inc"
};

static NJS_POINT3 egm2_eggman_points[41] = {
#include "b_egm2/eggman/egm2_eggman_points.inc"
};

static NJS_VECTOR egm2_eggman_normals[41] = {
#include "b_egm2/eggman/egm2_eggman_normals.inc"
};

static NJS_MODEL egm2_eggman_model = {
#include "b_egm2/eggman/egm2_eggman_model.inc"
};

static NJS_OBJECT egm2_eggman_object = {
#include "b_egm2/eggman/egm2_eggman_object.inc"
};

/* where Eggman sits, in the turret's coordinates */
static const NJS_VECTOR eggman_seat = {0.0F, 6.59F, 12.2F};

void egg_walker_eggman_draw(struct task *task)
{
    njSetTexture(&egm2_common_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)&eggman_seat);
    draw_tree(&egm2_eggman_object);
    njPopMatrix(1);
}
