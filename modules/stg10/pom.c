/* poms, the bursts of textures pom01 to pom08 that play once where they're
 * made; the targets' box and boards; and the list of the particle pools Final
 * Egg adds */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "stg10/drum.h"
#include "stg10/pom.h"
#include "stg10/puff.h"

#define TEXT_1_LENGTH 5
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 5
#define TEXT_7_LENGTH 5
#define TEXT_8_LENGTH 5
#include "stg10/pom/text.inc"

#define POM_FRAMES 8

static void final_pom_step(struct particle *particle,
                           struct particle_pool *pool);

static NJS_TEXNAME final_pom_textures[POM_FRAMES] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
static NJS_TEXANIM final_pom_frames[POM_FRAMES] = {
#include "stg10/pom/final_pom_frames.inc"
};
static NJS_TEXLIST final_pom_texlist;
static struct particle_anim final_pom_anim = {
    &final_pom_texlist, final_pom_frames, POM_FRAMES,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_ONE};
/* a pom plays 0.4 frames of its animation a step, in white at full alpha.
 * final_pom_anim's blend adds it to the color behind it */
static struct dust_params final_pom_params = {
    1.0F,
    0.0F,
    0.4F,
    0.0F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static NJS_TEXLIST final_pom_texlist = {
#include "stg10/pom/final_pom_texlist.inc"
};

static struct particle_pool_spec final_pom_pool = {&final_pom_anim, 0, NULL,
                                                   NULL};

static NJS_OBJECT target_box_node5_object = {
#include "stg10/pom/target_box_node5_object.inc"
};

static NJS_MATERIAL target_box_node4_materials[1] = {
#include "stg10/pom/target_box_node4_materials.inc"
};

static Sint16 target_box_node4_meshes[63] = {
#include "stg10/pom/target_box_node4_meshes.inc"
};

static NJS_COLOR target_box_node4_uvs[56] = {
#include "stg10/pom/target_box_node4_uvs.inc"
};

static NJS_MESHSET target_box_node4_meshsets[1] = {
#include "stg10/pom/target_box_node4_meshsets.inc"
};

static NJS_POINT3 target_box_node4_points[28] = {
#include "stg10/pom/target_box_node4_points.inc"
};

static NJS_VECTOR target_box_node4_normals[28] = {
#include "stg10/pom/target_box_node4_normals.inc"
};

static NJS_MODEL target_box_node4_model = {
#include "stg10/pom/target_box_node4_model.inc"
};

static NJS_OBJECT target_box_node4_object = {
#include "stg10/pom/target_box_node4_object.inc"
};

static NJS_MATERIAL target_box_node3_materials[19] = {
#include "stg10/pom/target_box_node3_materials.inc"
};

static Sint16 target_box_node3_meshes0[25] = {
#include "stg10/pom/target_box_node3_meshes0.inc"
};

static Sint16 target_box_node3_meshes1[7] = {
#include "stg10/pom/target_box_node3_meshes1.inc"
};

static Sint16 target_box_node3_meshes2[20] = {
#include "stg10/pom/target_box_node3_meshes2.inc"
};

static Sint16 target_box_node3_meshes3[21] = {
#include "stg10/pom/target_box_node3_meshes3.inc"
};

static Sint16 target_box_node3_meshes4[30] = {
#include "stg10/pom/target_box_node3_meshes4.inc"
};

static Sint16 target_box_node3_meshes5[40] = {
#include "stg10/pom/target_box_node3_meshes5.inc"
};

static Sint16 target_box_node3_meshes6[25] = {
#include "stg10/pom/target_box_node3_meshes6.inc"
};

static Sint16 target_box_node3_meshes7[30] = {
#include "stg10/pom/target_box_node3_meshes7.inc"
};

static Sint16 target_box_node3_meshes8[45] = {
#include "stg10/pom/target_box_node3_meshes8.inc"
};

static Sint16 target_box_node3_meshes9[6] = {
#include "stg10/pom/target_box_node3_meshes9.inc"
};

static Sint16 target_box_node3_meshes10[6] = {
#include "stg10/pom/target_box_node3_meshes10.inc"
};

static Sint16 target_box_node3_meshes11[50] = {
#include "stg10/pom/target_box_node3_meshes11.inc"
};

static Sint16 target_box_node3_meshes12[36] = {
#include "stg10/pom/target_box_node3_meshes12.inc"
};

static Sint16 target_box_node3_meshes13[9] = {
#include "stg10/pom/target_box_node3_meshes13.inc"
};

static Sint16 target_box_node3_meshes14[18] = {
#include "stg10/pom/target_box_node3_meshes14.inc"
};

static Sint16 target_box_node3_meshes15[30] = {
#include "stg10/pom/target_box_node3_meshes15.inc"
};

static Sint16 target_box_node3_meshes16[15] = {
#include "stg10/pom/target_box_node3_meshes16.inc"
};

static Sint16 target_box_node3_meshes17[20] = {
#include "stg10/pom/target_box_node3_meshes17.inc"
};

static Sint16 target_box_node3_meshes18[10] = {
#include "stg10/pom/target_box_node3_meshes18.inc"
};

static NJS_COLOR target_box_node3_uvs0[20] = {
#include "stg10/pom/target_box_node3_uvs0.inc"
};

static NJS_COLOR target_box_node3_uvs1[6] = {
#include "stg10/pom/target_box_node3_uvs1.inc"
};

static NJS_COLOR target_box_node3_uvs2[16] = {
#include "stg10/pom/target_box_node3_uvs2.inc"
};

static NJS_COLOR target_box_node3_uvs3[18] = {
#include "stg10/pom/target_box_node3_uvs3.inc"
};

static NJS_COLOR target_box_node3_uvs4[24] = {
#include "stg10/pom/target_box_node3_uvs4.inc"
};

static NJS_COLOR target_box_node3_uvs5[32] = {
#include "stg10/pom/target_box_node3_uvs5.inc"
};

static NJS_COLOR target_box_node3_uvs6[20] = {
#include "stg10/pom/target_box_node3_uvs6.inc"
};

static NJS_COLOR target_box_node3_uvs7[24] = {
#include "stg10/pom/target_box_node3_uvs7.inc"
};

static NJS_COLOR target_box_node3_uvs8[36] = {
#include "stg10/pom/target_box_node3_uvs8.inc"
};

static NJS_COLOR target_box_node3_uvs9[5] = {
#include "stg10/pom/target_box_node3_uvs9.inc"
};

static NJS_COLOR target_box_node3_uvs11[40] = {
#include "stg10/pom/target_box_node3_uvs11.inc"
};

static NJS_COLOR target_box_node3_uvs12[32] = {
#include "stg10/pom/target_box_node3_uvs12.inc"
};

static NJS_COLOR target_box_node3_uvs13[8] = {
#include "stg10/pom/target_box_node3_uvs13.inc"
};

static NJS_COLOR target_box_node3_uvs14[16] = {
#include "stg10/pom/target_box_node3_uvs14.inc"
};

static NJS_COLOR target_box_node3_uvs15[24] = {
#include "stg10/pom/target_box_node3_uvs15.inc"
};

static NJS_COLOR target_box_node3_uvs16[12] = {
#include "stg10/pom/target_box_node3_uvs16.inc"
};

static NJS_COLOR target_box_node3_uvs17[16] = {
#include "stg10/pom/target_box_node3_uvs17.inc"
};

static NJS_COLOR target_box_node3_uvs18[8] = {
#include "stg10/pom/target_box_node3_uvs18.inc"
};

static NJS_MESHSET target_box_node3_meshsets[19] = {
#include "stg10/pom/target_box_node3_meshsets.inc"
};

static NJS_POINT3 target_box_node3_points[143] = {
#include "stg10/pom/target_box_node3_points.inc"
};

static NJS_VECTOR target_box_node3_normals[143] = {
#include "stg10/pom/target_box_node3_normals.inc"
};

static NJS_MODEL target_box_node3_model = {
#include "stg10/pom/target_box_node3_model.inc"
};

static NJS_OBJECT target_box_node3_object = {
#include "stg10/pom/target_box_node3_object.inc"
};

static NJS_MATERIAL target_box_node2_materials[2] = {
#include "stg10/pom/target_box_node2_materials.inc"
};

static Sint16 target_box_node2_meshes0[5] = {
#include "stg10/pom/target_box_node2_meshes0.inc"
};

static Sint16 target_box_node2_meshes1[5] = {
#include "stg10/pom/target_box_node2_meshes1.inc"
};

static NJS_COLOR target_box_node2_uvs0[4] = {
#include "stg10/pom/target_box_node2_uvs0.inc"
};

static NJS_COLOR target_box_node2_uvs1[4] = {
#include "stg10/pom/target_box_node2_uvs1.inc"
};

static NJS_MESHSET target_box_node2_meshsets[2] = {
#include "stg10/pom/target_box_node2_meshsets.inc"
};

static NJS_POINT3 target_box_node2_points[6] = {
#include "stg10/pom/target_box_node2_points.inc"
};

static NJS_VECTOR target_box_node2_normals[6] = {
#include "stg10/pom/target_box_node2_normals.inc"
};

static NJS_MODEL target_box_node2_model = {
#include "stg10/pom/target_box_node2_model.inc"
};

static NJS_OBJECT target_box_node2_object = {
#include "stg10/pom/target_box_node2_object.inc"
};

static NJS_MATERIAL target_box_node1_materials[2] = {
#include "stg10/pom/target_box_node1_materials.inc"
};

static Sint16 target_box_node1_meshes0[5] = {
#include "stg10/pom/target_box_node1_meshes0.inc"
};

static Sint16 target_box_node1_meshes1[5] = {
#include "stg10/pom/target_box_node1_meshes1.inc"
};

static NJS_COLOR target_box_node1_uvs0[4] = {
#include "stg10/pom/target_box_node1_uvs0.inc"
};

static NJS_COLOR target_box_node1_uvs1[4] = {
#include "stg10/pom/target_box_node1_uvs1.inc"
};

static NJS_MESHSET target_box_node1_meshsets[2] = {
#include "stg10/pom/target_box_node1_meshsets.inc"
};

static NJS_POINT3 target_box_node1_points[6] = {
#include "stg10/pom/target_box_node1_points.inc"
};

static NJS_VECTOR target_box_node1_normals[6] = {
#include "stg10/pom/target_box_node1_normals.inc"
};

static NJS_MODEL target_box_node1_model = {
#include "stg10/pom/target_box_node1_model.inc"
};

static NJS_OBJECT target_box_node1_object = {
#include "stg10/pom/target_box_node1_object.inc"
};

static NJS_MATERIAL target_box_materials[5] = {
#include "stg10/pom/target_box_materials.inc"
};

static Sint16 target_box_meshes0[14] = {
#include "stg10/pom/target_box_meshes0.inc"
};

static Sint16 target_box_meshes1[17] = {
#include "stg10/pom/target_box_meshes1.inc"
};

static Sint16 target_box_meshes2[5] = {
#include "stg10/pom/target_box_meshes2.inc"
};

static Sint16 target_box_meshes3[11] = {
#include "stg10/pom/target_box_meshes3.inc"
};

static Sint16 target_box_meshes4[20] = {
#include "stg10/pom/target_box_meshes4.inc"
};

static NJS_COLOR target_box_uvs0[12] = {
#include "stg10/pom/target_box_uvs0.inc"
};

static NJS_COLOR target_box_uvs1[14] = {
#include "stg10/pom/target_box_uvs1.inc"
};

static NJS_COLOR target_box_uvs2[4] = {
#include "stg10/pom/target_box_uvs2.inc"
};

static NJS_COLOR target_box_uvs3[10] = {
#include "stg10/pom/target_box_uvs3.inc"
};

static NJS_COLOR target_box_uvs4[16] = {
#include "stg10/pom/target_box_uvs4.inc"
};

static NJS_MESHSET target_box_meshsets[5] = {
#include "stg10/pom/target_box_meshsets.inc"
};

static NJS_POINT3 target_box_points[20] = {
#include "stg10/pom/target_box_points.inc"
};

static NJS_VECTOR target_box_normals[20] = {
#include "stg10/pom/target_box_normals.inc"
};

static NJS_MODEL target_box_model = {
#include "stg10/pom/target_box_model.inc"
};

NJS_OBJECT target_box_object = {
#include "stg10/pom/target_box_object.inc"
};

static NJS_MATERIAL target_sonic_node8_materials[5] = {
#include "stg10/pom/target_sonic_node8_materials.inc"
};

static Sint16 target_sonic_node8_meshes0[22] = {
#include "stg10/pom/target_sonic_node8_meshes0.inc"
};

static Sint16 target_sonic_node8_meshes1[25] = {
#include "stg10/pom/target_sonic_node8_meshes1.inc"
};

static Sint16 target_sonic_node8_meshes2[30] = {
#include "stg10/pom/target_sonic_node8_meshes2.inc"
};

static Sint16 target_sonic_node8_meshes3[7] = {
#include "stg10/pom/target_sonic_node8_meshes3.inc"
};

static Sint16 target_sonic_node8_meshes4[10] = {
#include "stg10/pom/target_sonic_node8_meshes4.inc"
};

static NJS_COLOR target_sonic_node8_uvs0[20] = {
#include "stg10/pom/target_sonic_node8_uvs0.inc"
};

static NJS_COLOR target_sonic_node8_uvs1[21] = {
#include "stg10/pom/target_sonic_node8_uvs1.inc"
};

static NJS_COLOR target_sonic_node8_uvs2[24] = {
#include "stg10/pom/target_sonic_node8_uvs2.inc"
};

static NJS_COLOR target_sonic_node8_uvs3[6] = {
#include "stg10/pom/target_sonic_node8_uvs3.inc"
};

static NJS_COLOR target_sonic_node8_uvs4[8] = {
#include "stg10/pom/target_sonic_node8_uvs4.inc"
};

static NJS_MESHSET target_sonic_node8_meshsets[5] = {
#include "stg10/pom/target_sonic_node8_meshsets.inc"
};

static NJS_POINT3 target_sonic_node8_points[34] = {
#include "stg10/pom/target_sonic_node8_points.inc"
};

static NJS_VECTOR target_sonic_node8_normals[34] = {
#include "stg10/pom/target_sonic_node8_normals.inc"
};

static NJS_MODEL target_sonic_node8_model = {
#include "stg10/pom/target_sonic_node8_model.inc"
};

static NJS_OBJECT target_sonic_node8_object = {
#include "stg10/pom/target_sonic_node8_object.inc"
};

static NJS_MATERIAL target_sonic_node7_materials[2] = {
#include "stg10/pom/target_sonic_node7_materials.inc"
};

static Sint16 target_sonic_node7_meshes0[100] = {
#include "stg10/pom/target_sonic_node7_meshes0.inc"
};

static Sint16 target_sonic_node7_meshes1[22] = {
#include "stg10/pom/target_sonic_node7_meshes1.inc"
};

static NJS_COLOR target_sonic_node7_uvs0[88] = {
#include "stg10/pom/target_sonic_node7_uvs0.inc"
};

static NJS_COLOR target_sonic_node7_uvs1[20] = {
#include "stg10/pom/target_sonic_node7_uvs1.inc"
};

static NJS_MESHSET target_sonic_node7_meshsets[2] = {
#include "stg10/pom/target_sonic_node7_meshsets.inc"
};

static NJS_POINT3 target_sonic_node7_points[50] = {
#include "stg10/pom/target_sonic_node7_points.inc"
};

static NJS_VECTOR target_sonic_node7_normals[50] = {
#include "stg10/pom/target_sonic_node7_normals.inc"
};

static NJS_MODEL target_sonic_node7_model = {
#include "stg10/pom/target_sonic_node7_model.inc"
};

static NJS_OBJECT target_sonic_node7_object = {
#include "stg10/pom/target_sonic_node7_object.inc"
};

static NJS_MATERIAL target_sonic_node6_materials[5] = {
#include "stg10/pom/target_sonic_node6_materials.inc"
};

static Sint16 target_sonic_node6_meshes0[22] = {
#include "stg10/pom/target_sonic_node6_meshes0.inc"
};

static Sint16 target_sonic_node6_meshes1[22] = {
#include "stg10/pom/target_sonic_node6_meshes1.inc"
};

static Sint16 target_sonic_node6_meshes2[27] = {
#include "stg10/pom/target_sonic_node6_meshes2.inc"
};

static Sint16 target_sonic_node6_meshes3[10] = {
#include "stg10/pom/target_sonic_node6_meshes3.inc"
};

static Sint16 target_sonic_node6_meshes4[10] = {
#include "stg10/pom/target_sonic_node6_meshes4.inc"
};

static NJS_COLOR target_sonic_node6_uvs0[20] = {
#include "stg10/pom/target_sonic_node6_uvs0.inc"
};

static NJS_COLOR target_sonic_node6_uvs1[19] = {
#include "stg10/pom/target_sonic_node6_uvs1.inc"
};

static NJS_COLOR target_sonic_node6_uvs2[22] = {
#include "stg10/pom/target_sonic_node6_uvs2.inc"
};

static NJS_COLOR target_sonic_node6_uvs3[8] = {
#include "stg10/pom/target_sonic_node6_uvs3.inc"
};

static NJS_COLOR target_sonic_node6_uvs4[8] = {
#include "stg10/pom/target_sonic_node6_uvs4.inc"
};

static NJS_MESHSET target_sonic_node6_meshsets[5] = {
#include "stg10/pom/target_sonic_node6_meshsets.inc"
};

static NJS_POINT3 target_sonic_node6_points[34] = {
#include "stg10/pom/target_sonic_node6_points.inc"
};

static NJS_VECTOR target_sonic_node6_normals[34] = {
#include "stg10/pom/target_sonic_node6_normals.inc"
};

static NJS_MODEL target_sonic_node6_model = {
#include "stg10/pom/target_sonic_node6_model.inc"
};

static NJS_OBJECT target_sonic_node6_object = {
#include "stg10/pom/target_sonic_node6_object.inc"
};

static NJS_MATERIAL target_sonic_node5_materials[2] = {
#include "stg10/pom/target_sonic_node5_materials.inc"
};

static Sint16 target_sonic_node5_meshes0[100] = {
#include "stg10/pom/target_sonic_node5_meshes0.inc"
};

static Sint16 target_sonic_node5_meshes1[22] = {
#include "stg10/pom/target_sonic_node5_meshes1.inc"
};

static NJS_COLOR target_sonic_node5_uvs0[88] = {
#include "stg10/pom/target_sonic_node5_uvs0.inc"
};

static NJS_COLOR target_sonic_node5_uvs1[20] = {
#include "stg10/pom/target_sonic_node5_uvs1.inc"
};

static NJS_MESHSET target_sonic_node5_meshsets[2] = {
#include "stg10/pom/target_sonic_node5_meshsets.inc"
};

static NJS_POINT3 target_sonic_node5_points[50] = {
#include "stg10/pom/target_sonic_node5_points.inc"
};

static NJS_VECTOR target_sonic_node5_normals[50] = {
#include "stg10/pom/target_sonic_node5_normals.inc"
};

static NJS_MODEL target_sonic_node5_model = {
#include "stg10/pom/target_sonic_node5_model.inc"
};

static NJS_OBJECT target_sonic_node5_object = {
#include "stg10/pom/target_sonic_node5_object.inc"
};

static NJS_MATERIAL target_sonic_node4_materials[1] = {
#include "stg10/pom/target_sonic_node4_materials.inc"
};

static Sint16 target_sonic_node4_meshes[16] = {
#include "stg10/pom/target_sonic_node4_meshes.inc"
};

static NJS_COLOR target_sonic_node4_uvs[14] = {
#include "stg10/pom/target_sonic_node4_uvs.inc"
};

static NJS_MESHSET target_sonic_node4_meshsets[1] = {
#include "stg10/pom/target_sonic_node4_meshsets.inc"
};

static NJS_POINT3 target_sonic_node4_points[8] = {
#include "stg10/pom/target_sonic_node4_points.inc"
};

static NJS_VECTOR target_sonic_node4_normals[8] = {
#include "stg10/pom/target_sonic_node4_normals.inc"
};

static NJS_MODEL target_sonic_node4_model = {
#include "stg10/pom/target_sonic_node4_model.inc"
};

static NJS_OBJECT target_sonic_node4_object = {
#include "stg10/pom/target_sonic_node4_object.inc"
};

static NJS_MATERIAL target_sonic_node3_materials[1] = {
#include "stg10/pom/target_sonic_node3_materials.inc"
};

static Sint16 target_sonic_node3_meshes[16] = {
#include "stg10/pom/target_sonic_node3_meshes.inc"
};

static NJS_COLOR target_sonic_node3_uvs[14] = {
#include "stg10/pom/target_sonic_node3_uvs.inc"
};

static NJS_MESHSET target_sonic_node3_meshsets[1] = {
#include "stg10/pom/target_sonic_node3_meshsets.inc"
};

static NJS_POINT3 target_sonic_node3_points[8] = {
#include "stg10/pom/target_sonic_node3_points.inc"
};

static NJS_VECTOR target_sonic_node3_normals[8] = {
#include "stg10/pom/target_sonic_node3_normals.inc"
};

static NJS_MODEL target_sonic_node3_model = {
#include "stg10/pom/target_sonic_node3_model.inc"
};

static NJS_OBJECT target_sonic_node3_object = {
#include "stg10/pom/target_sonic_node3_object.inc"
};

static NJS_MATERIAL target_sonic_node2_materials[1] = {
#include "stg10/pom/target_sonic_node2_materials.inc"
};

static Sint16 target_sonic_node2_meshes[16] = {
#include "stg10/pom/target_sonic_node2_meshes.inc"
};

static NJS_COLOR target_sonic_node2_uvs[14] = {
#include "stg10/pom/target_sonic_node2_uvs.inc"
};

static NJS_MESHSET target_sonic_node2_meshsets[1] = {
#include "stg10/pom/target_sonic_node2_meshsets.inc"
};

static NJS_POINT3 target_sonic_node2_points[8] = {
#include "stg10/pom/target_sonic_node2_points.inc"
};

static NJS_VECTOR target_sonic_node2_normals[8] = {
#include "stg10/pom/target_sonic_node2_normals.inc"
};

static NJS_MODEL target_sonic_node2_model = {
#include "stg10/pom/target_sonic_node2_model.inc"
};

static NJS_OBJECT target_sonic_node2_object = {
#include "stg10/pom/target_sonic_node2_object.inc"
};

static NJS_MATERIAL target_sonic_node1_materials[5] = {
#include "stg10/pom/target_sonic_node1_materials.inc"
};

static Sint16 target_sonic_node1_meshes0[113] = {
#include "stg10/pom/target_sonic_node1_meshes0.inc"
};

static Sint16 target_sonic_node1_meshes1[61] = {
#include "stg10/pom/target_sonic_node1_meshes1.inc"
};

static Sint16 target_sonic_node1_meshes2[32] = {
#include "stg10/pom/target_sonic_node1_meshes2.inc"
};

static Sint16 target_sonic_node1_meshes3[21] = {
#include "stg10/pom/target_sonic_node1_meshes3.inc"
};

static Sint16 target_sonic_node1_meshes4[8] = {
#include "stg10/pom/target_sonic_node1_meshes4.inc"
};

static NJS_COLOR target_sonic_node1_uvs0[100] = {
#include "stg10/pom/target_sonic_node1_uvs0.inc"
};

static NJS_COLOR target_sonic_node1_uvs1[52] = {
#include "stg10/pom/target_sonic_node1_uvs1.inc"
};

static NJS_COLOR target_sonic_node1_uvs2[26] = {
#include "stg10/pom/target_sonic_node1_uvs2.inc"
};

static NJS_COLOR target_sonic_node1_uvs3[18] = {
#include "stg10/pom/target_sonic_node1_uvs3.inc"
};

static NJS_COLOR target_sonic_node1_uvs4[6] = {
#include "stg10/pom/target_sonic_node1_uvs4.inc"
};

static NJS_MESHSET target_sonic_node1_meshsets[5] = {
#include "stg10/pom/target_sonic_node1_meshsets.inc"
};

static NJS_POINT3 target_sonic_node1_points[70] = {
#include "stg10/pom/target_sonic_node1_points.inc"
};

static NJS_VECTOR target_sonic_node1_normals[70] = {
#include "stg10/pom/target_sonic_node1_normals.inc"
};

static NJS_MODEL target_sonic_node1_model = {
#include "stg10/pom/target_sonic_node1_model.inc"
};

static NJS_OBJECT target_sonic_node1_object = {
#include "stg10/pom/target_sonic_node1_object.inc"
};

static NJS_MATERIAL target_sonic_materials[2] = {
#include "stg10/pom/target_sonic_materials.inc"
};

static Sint16 target_sonic_meshes0[82] = {
#include "stg10/pom/target_sonic_meshes0.inc"
};

static Sint16 target_sonic_meshes1[17] = {
#include "stg10/pom/target_sonic_meshes1.inc"
};

static NJS_COLOR target_sonic_uvs0[74] = {
#include "stg10/pom/target_sonic_uvs0.inc"
};

static NJS_COLOR target_sonic_uvs1[14] = {
#include "stg10/pom/target_sonic_uvs1.inc"
};

static NJS_MESHSET target_sonic_meshsets[2] = {
#include "stg10/pom/target_sonic_meshsets.inc"
};

static NJS_POINT3 target_sonic_points[35] = {
#include "stg10/pom/target_sonic_points.inc"
};

static NJS_VECTOR target_sonic_normals[35] = {
#include "stg10/pom/target_sonic_normals.inc"
};

static NJS_MODEL target_sonic_model = {
#include "stg10/pom/target_sonic_model.inc"
};

NJS_OBJECT target_sonic_object = {
#include "stg10/pom/target_sonic_object.inc"
};

static NJS_MATERIAL target_knuckles_node8_materials[6] = {
#include "stg10/pom/target_knuckles_node8_materials.inc"
};

static Sint16 target_knuckles_node8_meshes0[22] = {
#include "stg10/pom/target_knuckles_node8_meshes0.inc"
};

static Sint16 target_knuckles_node8_meshes1[20] = {
#include "stg10/pom/target_knuckles_node8_meshes1.inc"
};

static Sint16 target_knuckles_node8_meshes2[7] = {
#include "stg10/pom/target_knuckles_node8_meshes2.inc"
};

static Sint16 target_knuckles_node8_meshes3[22] = {
#include "stg10/pom/target_knuckles_node8_meshes3.inc"
};

static Sint16 target_knuckles_node8_meshes4[15] = {
#include "stg10/pom/target_knuckles_node8_meshes4.inc"
};

static Sint16 target_knuckles_node8_meshes5[5] = {
#include "stg10/pom/target_knuckles_node8_meshes5.inc"
};

static NJS_COLOR target_knuckles_node8_uvs0[20] = {
#include "stg10/pom/target_knuckles_node8_uvs0.inc"
};

static NJS_COLOR target_knuckles_node8_uvs1[16] = {
#include "stg10/pom/target_knuckles_node8_uvs1.inc"
};

static NJS_COLOR target_knuckles_node8_uvs2[6] = {
#include "stg10/pom/target_knuckles_node8_uvs2.inc"
};

static NJS_COLOR target_knuckles_node8_uvs3[19] = {
#include "stg10/pom/target_knuckles_node8_uvs3.inc"
};

static NJS_COLOR target_knuckles_node8_uvs4[12] = {
#include "stg10/pom/target_knuckles_node8_uvs4.inc"
};

static NJS_COLOR target_knuckles_node8_uvs5[4] = {
#include "stg10/pom/target_knuckles_node8_uvs5.inc"
};

static NJS_MESHSET target_knuckles_node8_meshsets[6] = {
#include "stg10/pom/target_knuckles_node8_meshsets.inc"
};

static NJS_POINT3 target_knuckles_node8_points[34] = {
#include "stg10/pom/target_knuckles_node8_points.inc"
};

static NJS_VECTOR target_knuckles_node8_normals[34] = {
#include "stg10/pom/target_knuckles_node8_normals.inc"
};

static NJS_MODEL target_knuckles_node8_model = {
#include "stg10/pom/target_knuckles_node8_model.inc"
};

static NJS_OBJECT target_knuckles_node8_object = {
#include "stg10/pom/target_knuckles_node8_object.inc"
};

static NJS_MATERIAL target_knuckles_node7_materials[2] = {
#include "stg10/pom/target_knuckles_node7_materials.inc"
};

static Sint16 target_knuckles_node7_meshes0[152] = {
#include "stg10/pom/target_knuckles_node7_meshes0.inc"
};

static Sint16 target_knuckles_node7_meshes1[22] = {
#include "stg10/pom/target_knuckles_node7_meshes1.inc"
};

static NJS_COLOR target_knuckles_node7_uvs0[131] = {
#include "stg10/pom/target_knuckles_node7_uvs0.inc"
};

static NJS_COLOR target_knuckles_node7_uvs1[20] = {
#include "stg10/pom/target_knuckles_node7_uvs1.inc"
};

static NJS_MESHSET target_knuckles_node7_meshsets[2] = {
#include "stg10/pom/target_knuckles_node7_meshsets.inc"
};

static NJS_POINT3 target_knuckles_node7_points[70] = {
#include "stg10/pom/target_knuckles_node7_points.inc"
};

static NJS_VECTOR target_knuckles_node7_normals[70] = {
#include "stg10/pom/target_knuckles_node7_normals.inc"
};

static NJS_MODEL target_knuckles_node7_model = {
#include "stg10/pom/target_knuckles_node7_model.inc"
};

static NJS_OBJECT target_knuckles_node7_object = {
#include "stg10/pom/target_knuckles_node7_object.inc"
};

static NJS_MATERIAL target_knuckles_node6_materials[6] = {
#include "stg10/pom/target_knuckles_node6_materials.inc"
};

static Sint16 target_knuckles_node6_meshes0[22] = {
#include "stg10/pom/target_knuckles_node6_meshes0.inc"
};

static Sint16 target_knuckles_node6_meshes1[17] = {
#include "stg10/pom/target_knuckles_node6_meshes1.inc"
};

static Sint16 target_knuckles_node6_meshes2[7] = {
#include "stg10/pom/target_knuckles_node6_meshes2.inc"
};

static Sint16 target_knuckles_node6_meshes3[19] = {
#include "stg10/pom/target_knuckles_node6_meshes3.inc"
};

static Sint16 target_knuckles_node6_meshes4[15] = {
#include "stg10/pom/target_knuckles_node6_meshes4.inc"
};

static Sint16 target_knuckles_node6_meshes5[5] = {
#include "stg10/pom/target_knuckles_node6_meshes5.inc"
};

static NJS_COLOR target_knuckles_node6_uvs0[20] = {
#include "stg10/pom/target_knuckles_node6_uvs0.inc"
};

static NJS_COLOR target_knuckles_node6_uvs1[14] = {
#include "stg10/pom/target_knuckles_node6_uvs1.inc"
};

static NJS_COLOR target_knuckles_node6_uvs2[6] = {
#include "stg10/pom/target_knuckles_node6_uvs2.inc"
};

static NJS_COLOR target_knuckles_node6_uvs3[17] = {
#include "stg10/pom/target_knuckles_node6_uvs3.inc"
};

static NJS_COLOR target_knuckles_node6_uvs4[12] = {
#include "stg10/pom/target_knuckles_node6_uvs4.inc"
};

static NJS_COLOR target_knuckles_node6_uvs5[4] = {
#include "stg10/pom/target_knuckles_node6_uvs5.inc"
};

static NJS_MESHSET target_knuckles_node6_meshsets[6] = {
#include "stg10/pom/target_knuckles_node6_meshsets.inc"
};

static NJS_POINT3 target_knuckles_node6_points[34] = {
#include "stg10/pom/target_knuckles_node6_points.inc"
};

static NJS_VECTOR target_knuckles_node6_normals[34] = {
#include "stg10/pom/target_knuckles_node6_normals.inc"
};

static NJS_MODEL target_knuckles_node6_model = {
#include "stg10/pom/target_knuckles_node6_model.inc"
};

static NJS_OBJECT target_knuckles_node6_object = {
#include "stg10/pom/target_knuckles_node6_object.inc"
};

static NJS_MATERIAL target_knuckles_node5_materials[2] = {
#include "stg10/pom/target_knuckles_node5_materials.inc"
};

static Sint16 target_knuckles_node5_meshes0[152] = {
#include "stg10/pom/target_knuckles_node5_meshes0.inc"
};

static Sint16 target_knuckles_node5_meshes1[22] = {
#include "stg10/pom/target_knuckles_node5_meshes1.inc"
};

static NJS_COLOR target_knuckles_node5_uvs0[131] = {
#include "stg10/pom/target_knuckles_node5_uvs0.inc"
};

static NJS_COLOR target_knuckles_node5_uvs1[20] = {
#include "stg10/pom/target_knuckles_node5_uvs1.inc"
};

static NJS_MESHSET target_knuckles_node5_meshsets[2] = {
#include "stg10/pom/target_knuckles_node5_meshsets.inc"
};

static NJS_POINT3 target_knuckles_node5_points[70] = {
#include "stg10/pom/target_knuckles_node5_points.inc"
};

static NJS_VECTOR target_knuckles_node5_normals[70] = {
#include "stg10/pom/target_knuckles_node5_normals.inc"
};

static NJS_MODEL target_knuckles_node5_model = {
#include "stg10/pom/target_knuckles_node5_model.inc"
};

static NJS_OBJECT target_knuckles_node5_object = {
#include "stg10/pom/target_knuckles_node5_object.inc"
};

static NJS_MATERIAL target_knuckles_node4_materials[1] = {
#include "stg10/pom/target_knuckles_node4_materials.inc"
};

static Sint16 target_knuckles_node4_meshes[16] = {
#include "stg10/pom/target_knuckles_node4_meshes.inc"
};

static NJS_COLOR target_knuckles_node4_uvs[14] = {
#include "stg10/pom/target_knuckles_node4_uvs.inc"
};

static NJS_MESHSET target_knuckles_node4_meshsets[1] = {
#include "stg10/pom/target_knuckles_node4_meshsets.inc"
};

static NJS_POINT3 target_knuckles_node4_points[8] = {
#include "stg10/pom/target_knuckles_node4_points.inc"
};

static NJS_VECTOR target_knuckles_node4_normals[8] = {
#include "stg10/pom/target_knuckles_node4_normals.inc"
};

static NJS_MODEL target_knuckles_node4_model = {
#include "stg10/pom/target_knuckles_node4_model.inc"
};

static NJS_OBJECT target_knuckles_node4_object = {
#include "stg10/pom/target_knuckles_node4_object.inc"
};

static NJS_MATERIAL target_knuckles_node3_materials[1] = {
#include "stg10/pom/target_knuckles_node3_materials.inc"
};

static Sint16 target_knuckles_node3_meshes[16] = {
#include "stg10/pom/target_knuckles_node3_meshes.inc"
};

static NJS_COLOR target_knuckles_node3_uvs[14] = {
#include "stg10/pom/target_knuckles_node3_uvs.inc"
};

static NJS_MESHSET target_knuckles_node3_meshsets[1] = {
#include "stg10/pom/target_knuckles_node3_meshsets.inc"
};

static NJS_POINT3 target_knuckles_node3_points[8] = {
#include "stg10/pom/target_knuckles_node3_points.inc"
};

static NJS_VECTOR target_knuckles_node3_normals[8] = {
#include "stg10/pom/target_knuckles_node3_normals.inc"
};

static NJS_MODEL target_knuckles_node3_model = {
#include "stg10/pom/target_knuckles_node3_model.inc"
};

static NJS_OBJECT target_knuckles_node3_object = {
#include "stg10/pom/target_knuckles_node3_object.inc"
};

static NJS_MATERIAL target_knuckles_node2_materials[1] = {
#include "stg10/pom/target_knuckles_node2_materials.inc"
};

static Sint16 target_knuckles_node2_meshes[16] = {
#include "stg10/pom/target_knuckles_node2_meshes.inc"
};

static NJS_COLOR target_knuckles_node2_uvs[14] = {
#include "stg10/pom/target_knuckles_node2_uvs.inc"
};

static NJS_MESHSET target_knuckles_node2_meshsets[1] = {
#include "stg10/pom/target_knuckles_node2_meshsets.inc"
};

static NJS_POINT3 target_knuckles_node2_points[8] = {
#include "stg10/pom/target_knuckles_node2_points.inc"
};

static NJS_VECTOR target_knuckles_node2_normals[8] = {
#include "stg10/pom/target_knuckles_node2_normals.inc"
};

static NJS_MODEL target_knuckles_node2_model = {
#include "stg10/pom/target_knuckles_node2_model.inc"
};

static NJS_OBJECT target_knuckles_node2_object = {
#include "stg10/pom/target_knuckles_node2_object.inc"
};

static NJS_MATERIAL target_knuckles_node1_materials[4] = {
#include "stg10/pom/target_knuckles_node1_materials.inc"
};

static Sint16 target_knuckles_node1_meshes0[97] = {
#include "stg10/pom/target_knuckles_node1_meshes0.inc"
};

static Sint16 target_knuckles_node1_meshes1[61] = {
#include "stg10/pom/target_knuckles_node1_meshes1.inc"
};

static Sint16 target_knuckles_node1_meshes2[36] = {
#include "stg10/pom/target_knuckles_node1_meshes2.inc"
};

static Sint16 target_knuckles_node1_meshes3[11] = {
#include "stg10/pom/target_knuckles_node1_meshes3.inc"
};

static NJS_COLOR target_knuckles_node1_uvs0[88] = {
#include "stg10/pom/target_knuckles_node1_uvs0.inc"
};

static NJS_COLOR target_knuckles_node1_uvs1[52] = {
#include "stg10/pom/target_knuckles_node1_uvs1.inc"
};

static NJS_COLOR target_knuckles_node1_uvs2[32] = {
#include "stg10/pom/target_knuckles_node1_uvs2.inc"
};

static NJS_COLOR target_knuckles_node1_uvs3[10] = {
#include "stg10/pom/target_knuckles_node1_uvs3.inc"
};

static NJS_MESHSET target_knuckles_node1_meshsets[4] = {
#include "stg10/pom/target_knuckles_node1_meshsets.inc"
};

static NJS_POINT3 target_knuckles_node1_points[70] = {
#include "stg10/pom/target_knuckles_node1_points.inc"
};

static NJS_VECTOR target_knuckles_node1_normals[70] = {
#include "stg10/pom/target_knuckles_node1_normals.inc"
};

static NJS_MODEL target_knuckles_node1_model = {
#include "stg10/pom/target_knuckles_node1_model.inc"
};

static NJS_OBJECT target_knuckles_node1_object = {
#include "stg10/pom/target_knuckles_node1_object.inc"
};

static NJS_MATERIAL target_knuckles_materials[2] = {
#include "stg10/pom/target_knuckles_materials.inc"
};

static Sint16 target_knuckles_meshes0[69] = {
#include "stg10/pom/target_knuckles_meshes0.inc"
};

static Sint16 target_knuckles_meshes1[18] = {
#include "stg10/pom/target_knuckles_meshes1.inc"
};

static NJS_COLOR target_knuckles_uvs0[62] = {
#include "stg10/pom/target_knuckles_uvs0.inc"
};

static NJS_COLOR target_knuckles_uvs1[16] = {
#include "stg10/pom/target_knuckles_uvs1.inc"
};

static NJS_MESHSET target_knuckles_meshsets[2] = {
#include "stg10/pom/target_knuckles_meshsets.inc"
};

static NJS_POINT3 target_knuckles_points[32] = {
#include "stg10/pom/target_knuckles_points.inc"
};

static NJS_VECTOR target_knuckles_normals[32] = {
#include "stg10/pom/target_knuckles_normals.inc"
};

static NJS_MODEL target_knuckles_model = {
#include "stg10/pom/target_knuckles_model.inc"
};

NJS_OBJECT target_knuckles_object = {
#include "stg10/pom/target_knuckles_object.inc"
};

static NJS_MATERIAL target_tails_node10_materials[5] = {
#include "stg10/pom/target_tails_node10_materials.inc"
};

static Sint16 target_tails_node10_meshes0[22] = {
#include "stg10/pom/target_tails_node10_meshes0.inc"
};

static Sint16 target_tails_node10_meshes1[17] = {
#include "stg10/pom/target_tails_node10_meshes1.inc"
};

static Sint16 target_tails_node10_meshes2[27] = {
#include "stg10/pom/target_tails_node10_meshes2.inc"
};

static Sint16 target_tails_node10_meshes3[10] = {
#include "stg10/pom/target_tails_node10_meshes3.inc"
};

static Sint16 target_tails_node10_meshes4[12] = {
#include "stg10/pom/target_tails_node10_meshes4.inc"
};

static NJS_COLOR target_tails_node10_uvs0[20] = {
#include "stg10/pom/target_tails_node10_uvs0.inc"
};

static NJS_COLOR target_tails_node10_uvs1[14] = {
#include "stg10/pom/target_tails_node10_uvs1.inc"
};

static NJS_COLOR target_tails_node10_uvs2[23] = {
#include "stg10/pom/target_tails_node10_uvs2.inc"
};

static NJS_COLOR target_tails_node10_uvs3[8] = {
#include "stg10/pom/target_tails_node10_uvs3.inc"
};

static NJS_COLOR target_tails_node10_uvs4[10] = {
#include "stg10/pom/target_tails_node10_uvs4.inc"
};

static NJS_MESHSET target_tails_node10_meshsets[5] = {
#include "stg10/pom/target_tails_node10_meshsets.inc"
};

static NJS_POINT3 target_tails_node10_points[34] = {
#include "stg10/pom/target_tails_node10_points.inc"
};

static NJS_VECTOR target_tails_node10_normals[34] = {
#include "stg10/pom/target_tails_node10_normals.inc"
};

static NJS_MODEL target_tails_node10_model = {
#include "stg10/pom/target_tails_node10_model.inc"
};

static NJS_OBJECT target_tails_node10_object = {
#include "stg10/pom/target_tails_node10_object.inc"
};

static NJS_MATERIAL target_tails_node9_materials[2] = {
#include "stg10/pom/target_tails_node9_materials.inc"
};

static Sint16 target_tails_node9_meshes0[112] = {
#include "stg10/pom/target_tails_node9_meshes0.inc"
};

static Sint16 target_tails_node9_meshes1[11] = {
#include "stg10/pom/target_tails_node9_meshes1.inc"
};

static NJS_COLOR target_tails_node9_uvs0[96] = {
#include "stg10/pom/target_tails_node9_uvs0.inc"
};

static NJS_COLOR target_tails_node9_uvs1[10] = {
#include "stg10/pom/target_tails_node9_uvs1.inc"
};

static NJS_MESHSET target_tails_node9_meshsets[2] = {
#include "stg10/pom/target_tails_node9_meshsets.inc"
};

static NJS_POINT3 target_tails_node9_points[46] = {
#include "stg10/pom/target_tails_node9_points.inc"
};

static NJS_VECTOR target_tails_node9_normals[46] = {
#include "stg10/pom/target_tails_node9_normals.inc"
};

static NJS_MODEL target_tails_node9_model = {
#include "stg10/pom/target_tails_node9_model.inc"
};

static NJS_OBJECT target_tails_node9_object = {
#include "stg10/pom/target_tails_node9_object.inc"
};

static NJS_MATERIAL target_tails_node8_materials[1] = {
#include "stg10/pom/target_tails_node8_materials.inc"
};

static Sint16 target_tails_node8_meshes[16] = {
#include "stg10/pom/target_tails_node8_meshes.inc"
};

static NJS_COLOR target_tails_node8_uvs[14] = {
#include "stg10/pom/target_tails_node8_uvs.inc"
};

static NJS_MESHSET target_tails_node8_meshsets[1] = {
#include "stg10/pom/target_tails_node8_meshsets.inc"
};

static NJS_POINT3 target_tails_node8_points[8] = {
#include "stg10/pom/target_tails_node8_points.inc"
};

static NJS_VECTOR target_tails_node8_normals[8] = {
#include "stg10/pom/target_tails_node8_normals.inc"
};

static NJS_MODEL target_tails_node8_model = {
#include "stg10/pom/target_tails_node8_model.inc"
};

static NJS_OBJECT target_tails_node8_object = {
#include "stg10/pom/target_tails_node8_object.inc"
};

static NJS_MATERIAL target_tails_node7_materials[1] = {
#include "stg10/pom/target_tails_node7_materials.inc"
};

static Sint16 target_tails_node7_meshes[16] = {
#include "stg10/pom/target_tails_node7_meshes.inc"
};

static NJS_COLOR target_tails_node7_uvs[14] = {
#include "stg10/pom/target_tails_node7_uvs.inc"
};

static NJS_MESHSET target_tails_node7_meshsets[1] = {
#include "stg10/pom/target_tails_node7_meshsets.inc"
};

static NJS_POINT3 target_tails_node7_points[8] = {
#include "stg10/pom/target_tails_node7_points.inc"
};

static NJS_VECTOR target_tails_node7_normals[8] = {
#include "stg10/pom/target_tails_node7_normals.inc"
};

static NJS_MODEL target_tails_node7_model = {
#include "stg10/pom/target_tails_node7_model.inc"
};

static NJS_OBJECT target_tails_node7_object = {
#include "stg10/pom/target_tails_node7_object.inc"
};

static NJS_MATERIAL target_tails_node6_materials[1] = {
#include "stg10/pom/target_tails_node6_materials.inc"
};

static Sint16 target_tails_node6_meshes[16] = {
#include "stg10/pom/target_tails_node6_meshes.inc"
};

static NJS_COLOR target_tails_node6_uvs[14] = {
#include "stg10/pom/target_tails_node6_uvs.inc"
};

static NJS_MESHSET target_tails_node6_meshsets[1] = {
#include "stg10/pom/target_tails_node6_meshsets.inc"
};

static NJS_POINT3 target_tails_node6_points[8] = {
#include "stg10/pom/target_tails_node6_points.inc"
};

static NJS_VECTOR target_tails_node6_normals[8] = {
#include "stg10/pom/target_tails_node6_normals.inc"
};

static NJS_MODEL target_tails_node6_model = {
#include "stg10/pom/target_tails_node6_model.inc"
};

static NJS_OBJECT target_tails_node6_object = {
#include "stg10/pom/target_tails_node6_object.inc"
};

static NJS_MATERIAL target_tails_node5_materials[5] = {
#include "stg10/pom/target_tails_node5_materials.inc"
};

static Sint16 target_tails_node5_meshes0[119] = {
#include "stg10/pom/target_tails_node5_meshes0.inc"
};

static Sint16 target_tails_node5_meshes1[37] = {
#include "stg10/pom/target_tails_node5_meshes1.inc"
};

static Sint16 target_tails_node5_meshes2[28] = {
#include "stg10/pom/target_tails_node5_meshes2.inc"
};

static Sint16 target_tails_node5_meshes3[8] = {
#include "stg10/pom/target_tails_node5_meshes3.inc"
};

static Sint16 target_tails_node5_meshes4[21] = {
#include "stg10/pom/target_tails_node5_meshes4.inc"
};

static NJS_COLOR target_tails_node5_uvs0[106] = {
#include "stg10/pom/target_tails_node5_uvs0.inc"
};

static NJS_COLOR target_tails_node5_uvs1[32] = {
#include "stg10/pom/target_tails_node5_uvs1.inc"
};

static NJS_COLOR target_tails_node5_uvs2[24] = {
#include "stg10/pom/target_tails_node5_uvs2.inc"
};

static NJS_COLOR target_tails_node5_uvs3[6] = {
#include "stg10/pom/target_tails_node5_uvs3.inc"
};

static NJS_COLOR target_tails_node5_uvs4[18] = {
#include "stg10/pom/target_tails_node5_uvs4.inc"
};

static NJS_MESHSET target_tails_node5_meshsets[5] = {
#include "stg10/pom/target_tails_node5_meshsets.inc"
};

static NJS_POINT3 target_tails_node5_points[68] = {
#include "stg10/pom/target_tails_node5_points.inc"
};

static NJS_VECTOR target_tails_node5_normals[68] = {
#include "stg10/pom/target_tails_node5_normals.inc"
};

static NJS_MODEL target_tails_node5_model = {
#include "stg10/pom/target_tails_node5_model.inc"
};

static NJS_OBJECT target_tails_node5_object = {
#include "stg10/pom/target_tails_node5_object.inc"
};

static NJS_MATERIAL target_tails_node4_materials[5] = {
#include "stg10/pom/target_tails_node4_materials.inc"
};

static Sint16 target_tails_node4_meshes0[22] = {
#include "stg10/pom/target_tails_node4_meshes0.inc"
};

static Sint16 target_tails_node4_meshes1[17] = {
#include "stg10/pom/target_tails_node4_meshes1.inc"
};

static Sint16 target_tails_node4_meshes2[27] = {
#include "stg10/pom/target_tails_node4_meshes2.inc"
};

static Sint16 target_tails_node4_meshes3[10] = {
#include "stg10/pom/target_tails_node4_meshes3.inc"
};

static Sint16 target_tails_node4_meshes4[15] = {
#include "stg10/pom/target_tails_node4_meshes4.inc"
};

static NJS_COLOR target_tails_node4_uvs0[20] = {
#include "stg10/pom/target_tails_node4_uvs0.inc"
};

static NJS_COLOR target_tails_node4_uvs1[14] = {
#include "stg10/pom/target_tails_node4_uvs1.inc"
};

static NJS_COLOR target_tails_node4_uvs2[23] = {
#include "stg10/pom/target_tails_node4_uvs2.inc"
};

static NJS_COLOR target_tails_node4_uvs3[8] = {
#include "stg10/pom/target_tails_node4_uvs3.inc"
};

static NJS_COLOR target_tails_node4_uvs4[12] = {
#include "stg10/pom/target_tails_node4_uvs4.inc"
};

static NJS_MESHSET target_tails_node4_meshsets[5] = {
#include "stg10/pom/target_tails_node4_meshsets.inc"
};

static NJS_POINT3 target_tails_node4_points[34] = {
#include "stg10/pom/target_tails_node4_points.inc"
};

static NJS_VECTOR target_tails_node4_normals[34] = {
#include "stg10/pom/target_tails_node4_normals.inc"
};

static NJS_MODEL target_tails_node4_model = {
#include "stg10/pom/target_tails_node4_model.inc"
};

static NJS_OBJECT target_tails_node4_object = {
#include "stg10/pom/target_tails_node4_object.inc"
};

static NJS_MATERIAL target_tails_node3_materials[2] = {
#include "stg10/pom/target_tails_node3_materials.inc"
};

static Sint16 target_tails_node3_meshes0[109] = {
#include "stg10/pom/target_tails_node3_meshes0.inc"
};

static Sint16 target_tails_node3_meshes1[11] = {
#include "stg10/pom/target_tails_node3_meshes1.inc"
};

static NJS_COLOR target_tails_node3_uvs0[94] = {
#include "stg10/pom/target_tails_node3_uvs0.inc"
};

static NJS_COLOR target_tails_node3_uvs1[10] = {
#include "stg10/pom/target_tails_node3_uvs1.inc"
};

static NJS_MESHSET target_tails_node3_meshsets[2] = {
#include "stg10/pom/target_tails_node3_meshsets.inc"
};

static NJS_POINT3 target_tails_node3_points[46] = {
#include "stg10/pom/target_tails_node3_points.inc"
};

static NJS_VECTOR target_tails_node3_normals[46] = {
#include "stg10/pom/target_tails_node3_normals.inc"
};

static NJS_MODEL target_tails_node3_model = {
#include "stg10/pom/target_tails_node3_model.inc"
};

static NJS_OBJECT target_tails_node3_object = {
#include "stg10/pom/target_tails_node3_object.inc"
};

static NJS_MATERIAL target_tails_node2_materials[1] = {
#include "stg10/pom/target_tails_node2_materials.inc"
};

static Sint16 target_tails_node2_meshes[45] = {
#include "stg10/pom/target_tails_node2_meshes.inc"
};

static NJS_COLOR target_tails_node2_uvs[40] = {
#include "stg10/pom/target_tails_node2_uvs.inc"
};

static NJS_MESHSET target_tails_node2_meshsets[1] = {
#include "stg10/pom/target_tails_node2_meshsets.inc"
};

static NJS_POINT3 target_tails_node2_points[17] = {
#include "stg10/pom/target_tails_node2_points.inc"
};

static NJS_VECTOR target_tails_node2_normals[17] = {
#include "stg10/pom/target_tails_node2_normals.inc"
};

static NJS_MODEL target_tails_node2_model = {
#include "stg10/pom/target_tails_node2_model.inc"
};

static NJS_OBJECT target_tails_node2_object = {
#include "stg10/pom/target_tails_node2_object.inc"
};

static NJS_MATERIAL target_tails_node1_materials[1] = {
#include "stg10/pom/target_tails_node1_materials.inc"
};

static Sint16 target_tails_node1_meshes[45] = {
#include "stg10/pom/target_tails_node1_meshes.inc"
};

static NJS_COLOR target_tails_node1_uvs[40] = {
#include "stg10/pom/target_tails_node1_uvs.inc"
};

static NJS_MESHSET target_tails_node1_meshsets[1] = {
#include "stg10/pom/target_tails_node1_meshsets.inc"
};

static NJS_POINT3 target_tails_node1_points[17] = {
#include "stg10/pom/target_tails_node1_points.inc"
};

static NJS_VECTOR target_tails_node1_normals[17] = {
#include "stg10/pom/target_tails_node1_normals.inc"
};

static NJS_MODEL target_tails_node1_model = {
#include "stg10/pom/target_tails_node1_model.inc"
};

static NJS_OBJECT target_tails_node1_object = {
#include "stg10/pom/target_tails_node1_object.inc"
};

static NJS_MATERIAL target_tails_materials[2] = {
#include "stg10/pom/target_tails_materials.inc"
};

static Sint16 target_tails_meshes0[59] = {
#include "stg10/pom/target_tails_meshes0.inc"
};

static Sint16 target_tails_meshes1[73] = {
#include "stg10/pom/target_tails_meshes1.inc"
};

static NJS_COLOR target_tails_uvs0[48] = {
#include "stg10/pom/target_tails_uvs0.inc"
};

static NJS_COLOR target_tails_uvs1[64] = {
#include "stg10/pom/target_tails_uvs1.inc"
};

static NJS_MESHSET target_tails_meshsets[2] = {
#include "stg10/pom/target_tails_meshsets.inc"
};

static NJS_POINT3 target_tails_points[53] = {
#include "stg10/pom/target_tails_points.inc"
};

static NJS_VECTOR target_tails_normals[53] = {
#include "stg10/pom/target_tails_normals.inc"
};

static NJS_MODEL target_tails_model = {
#include "stg10/pom/target_tails_model.inc"
};

NJS_OBJECT target_tails_object = {
#include "stg10/pom/target_tails_object.inc"
};

struct particle_pool_spec *final_pools[] = {
    &final_pom_pool,
    &final_drum_dust_pool,
    &final_puff_pool,
    NULL,
};

void final_pom_create(NJS_POINT3 *pos, Float scale)
{
    struct particle_pool *pool = final_pom_pool.pool;
    struct particle *particle;

    /* @hack spelling: the particle assigned in the test. SHC tests it in R0
     * (TST R0,R0 at 0x0c9150b2), and copies it to R5 in the branch's delay
     * slot. as two statements, SHC copies it to R4 to test it, and 14 of 43
     * words differ */
    if ((particle = particle_alloc(pool, final_pom_step)) != NULL) {
        particle->pos = *pos;
        particle->scale = scale;
        particle->time = 0.0F;
        particle->ang = 0;
        particle->color = final_pom_params.color;
        DUST(particle)->params = &final_pom_params;
    }
}

static void final_pom_step(struct particle *particle,
                           struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;

    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    /* @hack redundant: the rate times 1.0F, which SHC adds with an FMAC (FLDI1
     * FR0 at 0x0c91511e, FMAC FR0,FR3,FR2 at 0x0c915128); without it, 8 of 28
     * words differ */
    particle->time += params->rate * 1.0F;
}
