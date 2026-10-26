/* Chaos 6's Eggman, in Sonic's fight. he flies the Egg Mobile after
 * player 0 over the deck, a heading at a time, and drops bombs. he and his
 * bombs hold still while the set editor is open or the boss is in
 * BOSS_START. he climbs and flies on high when he nears the boss, or while
 * the boss is out of BOSS_CHASE, BOSS_STRIKE and BOSS_WEAVE. he comes down
 * again after more than EGGMAN_AWAY steps of flight. he must then be clear
 * of the boss, and the boss in one of those modes */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos6/bomb.h"
#include "b_chaos6/boss.h"
#include "b_chaos6/eggman.h"
#include "b_chaos6/eggman_textures.h"
#include "b_chaos6/hint.h"
#include "chaos/chaos.h"
#include "chaos/steer.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the sound a bomb drops with, and his voice with it, which he speaks only
 * while no hint is waiting */
#define SE_BOMB 395
#define VOICE_BOMB 175

/* the heights his chase keeps him between, and how far he comes down to
 * the first and climbs to the second a step */
#define EGGMAN_LOW 774.0F
#define EGGMAN_HIGH 844.0F
#define EGGMAN_DOWN 1.5F
#define EGGMAN_UP 2.0F
/* the most he turns toward player 0 a step */
#define EGGMAN_TURN 0x222
/* how far he flies across a step */
#define EGGMAN_SPEED 1.5F
/* within NEAR_PLAYER of player 0 across, his chase climbs by EGGMAN_RISE a
 * step until he's EGGMAN_CLEAR above player 0 */
#define NEAR_PLAYER 30.0F
#define EGGMAN_CLEAR 20.0F
#define EGGMAN_RISE 3.0F
/* farther from player 0, his chase sinks by this a step */
#define EGGMAN_SINK (-2.0F)
/* how near the boss he comes across before he climbs. once he has
 * flown more than EGGMAN_AWAY steps, he may come down again */
#define NEAR_BOSS 50.0F
#define EGGMAN_AWAY 180
/* he tries to drop a bomb each time bomb_steps passes BOMB_WAIT. none drops
 * while BOMBS are out */
#define BOMB_WAIT 120
#define BOMBS 6
/* each chase counts down from a random start, from CHASE_TIME steps up to
 * twice that. he aims again once it reaches CHASE_LEFT */
#define CHASE_TIME 60.0F
#define CHASE_LEFT 16
/* his shadow's size on a floor right under him */
#define EGGMAN_SHADOW 1.2F

struct eggman_vars {
    Uint8 unk_00[4];
    Sint16 chase; /* the steps left of his chase */
    Sint16 bomb_steps; /* the steps he has counted toward his next bomb */
};

#define VARS(work) ((struct eggman_vars *)&(work)->vars)

enum eggman_mode {
    EGGMAN_DESCEND,
    EGGMAN_AIM,
    EGGMAN_CHASE,
    EGGMAN_ASCEND,
    EGGMAN_FLY
};

static Sint8 dat_0ca91ce0;
/* set as a bomb drops and cleared as he aims */
static Sint8 dat_0ca91ce1;
/* his task, the parent of his bombs */
static struct task *eggman_task;
/* the steps he has spent in EGGMAN_FLY */
static Sint16 eggman_away;

static NJS_MATERIAL chaos6_eggman_node11_materials[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node11_materials.inc"
};

static Sint16 chaos6_eggman_node11_meshes0[14] = {
#include "b_chaos6/eggman/chaos6_eggman_node11_meshes0.inc"
};

static NJS_COLOR chaos6_eggman_node11_uvs0[12] = {
#include "b_chaos6/eggman/chaos6_eggman_node11_uvs0.inc"
};

static NJS_MESHSET chaos6_eggman_node11_meshsets[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node11_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node11_points[9] = {
#include "b_chaos6/eggman/chaos6_eggman_node11_points.inc"
};

static NJS_VECTOR chaos6_eggman_node11_normals[9] = {
#include "b_chaos6/eggman/chaos6_eggman_node11_normals.inc"
};

static NJS_MODEL chaos6_eggman_node11_model = {
#include "b_chaos6/eggman/chaos6_eggman_node11_model.inc"
};

static NJS_OBJECT chaos6_eggman_node11_object = {
#include "b_chaos6/eggman/chaos6_eggman_node11_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node10_materials[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node10_materials.inc"
};

static Sint16 chaos6_eggman_node10_meshes0[5] = {
#include "b_chaos6/eggman/chaos6_eggman_node10_meshes0.inc"
};

static NJS_MESHSET chaos6_eggman_node10_meshsets[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node10_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node10_points[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node10_points.inc"
};

static NJS_VECTOR chaos6_eggman_node10_normals[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node10_normals.inc"
};

static NJS_MODEL chaos6_eggman_node10_model = {
#include "b_chaos6/eggman/chaos6_eggman_node10_model.inc"
};

static NJS_OBJECT chaos6_eggman_node10_object = {
#include "b_chaos6/eggman/chaos6_eggman_node10_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node9_materials[3] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_materials.inc"
};

static Sint16 chaos6_eggman_node9_meshes0[10] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_meshes0.inc"
};

static Sint16 chaos6_eggman_node9_meshes1[8] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_meshes1.inc"
};

static Sint16 chaos6_eggman_node9_meshes2[5] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_meshes2.inc"
};

static NJS_COLOR chaos6_eggman_node9_uvs1[6] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_uvs1.inc"
};

static NJS_COLOR chaos6_eggman_node9_uvs2[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_uvs2.inc"
};

static NJS_MESHSET chaos6_eggman_node9_meshsets[3] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node9_points[8] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_points.inc"
};

static NJS_VECTOR chaos6_eggman_node9_normals[8] = {
#include "b_chaos6/eggman/chaos6_eggman_node9_normals.inc"
};

static NJS_MODEL chaos6_eggman_node9_model = {
#include "b_chaos6/eggman/chaos6_eggman_node9_model.inc"
};

static NJS_OBJECT chaos6_eggman_node9_object = {
#include "b_chaos6/eggman/chaos6_eggman_node9_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node8_materials[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_materials.inc"
};

static Sint16 chaos6_eggman_node8_meshes0[19] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_meshes0.inc"
};

static Sint16 chaos6_eggman_node8_meshes1[5] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_meshes1.inc"
};

static Sint16 chaos6_eggman_node8_meshes2[11] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_meshes2.inc"
};

static Sint16 chaos6_eggman_node8_meshes3[16] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_meshes3.inc"
};

static NJS_COLOR chaos6_eggman_node8_uvs0[16] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_uvs0.inc"
};

static NJS_COLOR chaos6_eggman_node8_uvs1[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_uvs1.inc"
};

static NJS_COLOR chaos6_eggman_node8_uvs2[10] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_uvs2.inc"
};

static NJS_MESHSET chaos6_eggman_node8_meshsets[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node8_points[26] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_points.inc"
};

static NJS_VECTOR chaos6_eggman_node8_normals[26] = {
#include "b_chaos6/eggman/chaos6_eggman_node8_normals.inc"
};

static NJS_MODEL chaos6_eggman_node8_model = {
#include "b_chaos6/eggman/chaos6_eggman_node8_model.inc"
};

static NJS_OBJECT chaos6_eggman_node8_object = {
#include "b_chaos6/eggman/chaos6_eggman_node8_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node7_materials[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node7_materials.inc"
};

static Sint16 chaos6_eggman_node7_meshes0[44] = {
#include "b_chaos6/eggman/chaos6_eggman_node7_meshes0.inc"
};

static NJS_COLOR chaos6_eggman_node7_uvs0[35] = {
#include "b_chaos6/eggman/chaos6_eggman_node7_uvs0.inc"
};

static NJS_MESHSET chaos6_eggman_node7_meshsets[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node7_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node7_points[12] = {
#include "b_chaos6/eggman/chaos6_eggman_node7_points.inc"
};

static NJS_VECTOR chaos6_eggman_node7_normals[12] = {
#include "b_chaos6/eggman/chaos6_eggman_node7_normals.inc"
};

static NJS_MODEL chaos6_eggman_node7_model = {
#include "b_chaos6/eggman/chaos6_eggman_node7_model.inc"
};

static NJS_OBJECT chaos6_eggman_node7_object = {
#include "b_chaos6/eggman/chaos6_eggman_node7_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node6_materials[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node6_materials.inc"
};

static Sint16 chaos6_eggman_node6_meshes0[5] = {
#include "b_chaos6/eggman/chaos6_eggman_node6_meshes0.inc"
};

static NJS_MESHSET chaos6_eggman_node6_meshsets[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node6_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node6_points[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node6_points.inc"
};

static NJS_VECTOR chaos6_eggman_node6_normals[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node6_normals.inc"
};

static NJS_MODEL chaos6_eggman_node6_model = {
#include "b_chaos6/eggman/chaos6_eggman_node6_model.inc"
};

static NJS_OBJECT chaos6_eggman_node6_object = {
#include "b_chaos6/eggman/chaos6_eggman_node6_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node5_materials[3] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_materials.inc"
};

static Sint16 chaos6_eggman_node5_meshes0[10] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_meshes0.inc"
};

static Sint16 chaos6_eggman_node5_meshes1[8] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_meshes1.inc"
};

static Sint16 chaos6_eggman_node5_meshes2[5] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_meshes2.inc"
};

static NJS_COLOR chaos6_eggman_node5_uvs1[6] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_uvs1.inc"
};

static NJS_COLOR chaos6_eggman_node5_uvs2[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_uvs2.inc"
};

static NJS_MESHSET chaos6_eggman_node5_meshsets[3] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node5_points[8] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_points.inc"
};

static NJS_VECTOR chaos6_eggman_node5_normals[8] = {
#include "b_chaos6/eggman/chaos6_eggman_node5_normals.inc"
};

static NJS_MODEL chaos6_eggman_node5_model = {
#include "b_chaos6/eggman/chaos6_eggman_node5_model.inc"
};

static NJS_OBJECT chaos6_eggman_node5_object = {
#include "b_chaos6/eggman/chaos6_eggman_node5_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node4_materials[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_materials.inc"
};

static Sint16 chaos6_eggman_node4_meshes0[19] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_meshes0.inc"
};

static Sint16 chaos6_eggman_node4_meshes1[5] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_meshes1.inc"
};

static Sint16 chaos6_eggman_node4_meshes2[11] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_meshes2.inc"
};

static Sint16 chaos6_eggman_node4_meshes3[16] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_meshes3.inc"
};

static NJS_COLOR chaos6_eggman_node4_uvs0[16] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_uvs0.inc"
};

static NJS_COLOR chaos6_eggman_node4_uvs1[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_uvs1.inc"
};

static NJS_COLOR chaos6_eggman_node4_uvs2[10] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_uvs2.inc"
};

static NJS_MESHSET chaos6_eggman_node4_meshsets[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node4_points[26] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_points.inc"
};

static NJS_VECTOR chaos6_eggman_node4_normals[26] = {
#include "b_chaos6/eggman/chaos6_eggman_node4_normals.inc"
};

static NJS_MODEL chaos6_eggman_node4_model = {
#include "b_chaos6/eggman/chaos6_eggman_node4_model.inc"
};

static NJS_OBJECT chaos6_eggman_node4_object = {
#include "b_chaos6/eggman/chaos6_eggman_node4_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node3_materials[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node3_materials.inc"
};

static Sint16 chaos6_eggman_node3_meshes0[44] = {
#include "b_chaos6/eggman/chaos6_eggman_node3_meshes0.inc"
};

static NJS_COLOR chaos6_eggman_node3_uvs0[35] = {
#include "b_chaos6/eggman/chaos6_eggman_node3_uvs0.inc"
};

static NJS_MESHSET chaos6_eggman_node3_meshsets[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node3_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node3_points[12] = {
#include "b_chaos6/eggman/chaos6_eggman_node3_points.inc"
};

static NJS_VECTOR chaos6_eggman_node3_normals[12] = {
#include "b_chaos6/eggman/chaos6_eggman_node3_normals.inc"
};

static NJS_MODEL chaos6_eggman_node3_model = {
#include "b_chaos6/eggman/chaos6_eggman_node3_model.inc"
};

static NJS_OBJECT chaos6_eggman_node3_object = {
#include "b_chaos6/eggman/chaos6_eggman_node3_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node2_materials[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node2_materials.inc"
};

static Sint16 chaos6_eggman_node2_meshes0[17] = {
#include "b_chaos6/eggman/chaos6_eggman_node2_meshes0.inc"
};

static NJS_COLOR chaos6_eggman_node2_uvs0[14] = {
#include "b_chaos6/eggman/chaos6_eggman_node2_uvs0.inc"
};

static NJS_MESHSET chaos6_eggman_node2_meshsets[1] = {
#include "b_chaos6/eggman/chaos6_eggman_node2_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node2_points[10] = {
#include "b_chaos6/eggman/chaos6_eggman_node2_points.inc"
};

static NJS_VECTOR chaos6_eggman_node2_normals[10] = {
#include "b_chaos6/eggman/chaos6_eggman_node2_normals.inc"
};

static NJS_MODEL chaos6_eggman_node2_model = {
#include "b_chaos6/eggman/chaos6_eggman_node2_model.inc"
};

static NJS_OBJECT chaos6_eggman_node2_object = {
#include "b_chaos6/eggman/chaos6_eggman_node2_object.inc"
};

static NJS_MATERIAL chaos6_eggman_node1_materials[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_materials.inc"
};

static Sint16 chaos6_eggman_node1_meshes0[118] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_meshes0.inc"
};

static Sint16 chaos6_eggman_node1_meshes1[20] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_meshes1.inc"
};

static Sint16 chaos6_eggman_node1_meshes2[29] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_meshes2.inc"
};

static Sint16 chaos6_eggman_node1_meshes3[11] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_meshes3.inc"
};

static NJS_COLOR chaos6_eggman_node1_uvs1[16] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_uvs1.inc"
};

static NJS_COLOR chaos6_eggman_node1_uvs2[22] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_uvs2.inc"
};

static NJS_MESHSET chaos6_eggman_node1_meshsets[4] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_node1_points[72] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_points.inc"
};

static NJS_VECTOR chaos6_eggman_node1_normals[72] = {
#include "b_chaos6/eggman/chaos6_eggman_node1_normals.inc"
};

static NJS_MODEL chaos6_eggman_node1_model = {
#include "b_chaos6/eggman/chaos6_eggman_node1_model.inc"
};

static NJS_OBJECT chaos6_eggman_node1_object = {
#include "b_chaos6/eggman/chaos6_eggman_node1_object.inc"
};

static NJS_MATERIAL chaos6_eggman_materials[4] = {
#include "b_chaos6/eggman/chaos6_eggman_materials.inc"
};

static Sint16 chaos6_eggman_meshes0[18] = {
#include "b_chaos6/eggman/chaos6_eggman_meshes0.inc"
};

static Sint16 chaos6_eggman_meshes1[9] = {
#include "b_chaos6/eggman/chaos6_eggman_meshes1.inc"
};

static Sint16 chaos6_eggman_meshes2[66] = {
#include "b_chaos6/eggman/chaos6_eggman_meshes2.inc"
};

static Sint16 chaos6_eggman_meshes3[9] = {
#include "b_chaos6/eggman/chaos6_eggman_meshes3.inc"
};

static NJS_COLOR chaos6_eggman_uvs0[14] = {
#include "b_chaos6/eggman/chaos6_eggman_uvs0.inc"
};

static NJS_COLOR chaos6_eggman_uvs1[7] = {
#include "b_chaos6/eggman/chaos6_eggman_uvs1.inc"
};

static NJS_COLOR chaos6_eggman_uvs2[54] = {
#include "b_chaos6/eggman/chaos6_eggman_uvs2.inc"
};

static NJS_COLOR chaos6_eggman_uvs3[7] = {
#include "b_chaos6/eggman/chaos6_eggman_uvs3.inc"
};

static NJS_MESHSET chaos6_eggman_meshsets[4] = {
#include "b_chaos6/eggman/chaos6_eggman_meshsets.inc"
};

static NJS_POINT3 chaos6_eggman_points[28] = {
#include "b_chaos6/eggman/chaos6_eggman_points.inc"
};

static NJS_VECTOR chaos6_eggman_normals[28] = {
#include "b_chaos6/eggman/chaos6_eggman_normals.inc"
};

static NJS_MODEL chaos6_eggman_model = {
#include "b_chaos6/eggman/chaos6_eggman_model.inc"
};

static NJS_OBJECT chaos6_eggman_object = {
#include "b_chaos6/eggman/chaos6_eggman_object.inc"
};

static NJS_MATERIAL chaos6_eggmobile_materials[25] = {
#include "b_chaos6/eggman/chaos6_eggmobile_materials.inc"
};

static Sint16 chaos6_eggmobile_meshes0[71] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes0.inc"
};

static Sint16 chaos6_eggmobile_meshes1[106] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes1.inc"
};

static Sint16 chaos6_eggmobile_meshes2[22] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes2.inc"
};

static Sint16 chaos6_eggmobile_meshes3[55] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes3.inc"
};

static Sint16 chaos6_eggmobile_meshes4[5] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes4.inc"
};

static Sint16 chaos6_eggmobile_meshes5[10] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes5.inc"
};

static Sint16 chaos6_eggmobile_meshes6[18] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes6.inc"
};

static Sint16 chaos6_eggmobile_meshes7[16] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes7.inc"
};

static Sint16 chaos6_eggmobile_meshes8[15] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes8.inc"
};

static Sint16 chaos6_eggmobile_meshes9[8] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes9.inc"
};

static Sint16 chaos6_eggmobile_meshes10[19] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes10.inc"
};

static Sint16 chaos6_eggmobile_meshes11[15] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes11.inc"
};

static Sint16 chaos6_eggmobile_meshes12[45] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes12.inc"
};

static Sint16 chaos6_eggmobile_meshes13[36] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes13.inc"
};

static Sint16 chaos6_eggmobile_meshes14[36] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes14.inc"
};

static Sint16 chaos6_eggmobile_meshes15[12] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes15.inc"
};

static Sint16 chaos6_eggmobile_meshes16[93] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes16.inc"
};

static Sint16 chaos6_eggmobile_meshes17[48] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes17.inc"
};

static Sint16 chaos6_eggmobile_meshes18[48] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes18.inc"
};

static Sint16 chaos6_eggmobile_meshes19[60] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes19.inc"
};

static Sint16 chaos6_eggmobile_meshes20[10] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes20.inc"
};

static Sint16 chaos6_eggmobile_meshes21[10] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes21.inc"
};

static Sint16 chaos6_eggmobile_meshes22[5] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes22.inc"
};

static Sint16 chaos6_eggmobile_meshes23[10] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes23.inc"
};

static Sint16 chaos6_eggmobile_meshes24[20] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshes24.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs0[62] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs0.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs1[94] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs1.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs2[18] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs2.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs3[42] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs3.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs4[4] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs4.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs5[8] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs5.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs6[14] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs6.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs7[14] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs7.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs8[12] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs8.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs9[6] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs9.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs10[16] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs10.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs11[14] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs11.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs12[42] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs12.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs13[32] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs13.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs14[32] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs14.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs15[10] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs15.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs16[83] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs16.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs17[36] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs17.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs18[40] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs18.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs19[48] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs19.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs20[8] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs20.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs21[8] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs21.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs22[4] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs22.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs23[8] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs23.inc"
};

static NJS_COLOR chaos6_eggmobile_uvs24[16] = {
#include "b_chaos6/eggman/chaos6_eggmobile_uvs24.inc"
};

static NJS_MESHSET chaos6_eggmobile_meshsets[25] = {
#include "b_chaos6/eggman/chaos6_eggmobile_meshsets.inc"
};

static NJS_POINT3 chaos6_eggmobile_points[296] = {
#include "b_chaos6/eggman/chaos6_eggmobile_points.inc"
};

static NJS_VECTOR chaos6_eggmobile_normals[296] = {
#include "b_chaos6/eggman/chaos6_eggmobile_normals.inc"
};

static NJS_MODEL chaos6_eggmobile_model = {
#include "b_chaos6/eggman/chaos6_eggmobile_model.inc"
};

static NJS_OBJECT chaos6_eggmobile_object = {
#include "b_chaos6/eggman/chaos6_eggmobile_object.inc"
};

static void chaos6_eggman_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&chaos6_eggman_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, -work->ang.y + 0x4000, ang);
    draw_tree(&chaos6_eggman_object);
    draw_tree(&chaos6_eggmobile_object);
    njPopMatrix(1);
}

static void chaos6_eggman_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct task_motion *motion = task->motion;
    Angle ang;
    Float x;
    Float z;
    Float distance;

    if (edit_mode == 1) {
        return;
    }
    if (boss_work->mode == BOSS_START) {
        return;
    }
    work->ang.y = angle_approach(work->ang.y,
                                 (Angle)(atan2f(player->pos.z - work->pos.z,
                                                player->pos.x - work->pos.x) *
                                         ANGLE_PER_RADIAN),
                                 EGGMAN_TURN);
    switch (work->mode) {
    case EGGMAN_DESCEND:
        work->pos.y -= EGGMAN_DOWN;
        if (work->pos.y <= EGGMAN_LOW) {
            work->pos.y = EGGMAN_LOW;
            work->mode = EGGMAN_AIM;
        }
        break;
    case EGGMAN_AIM:
        x = player->pos.x - work->pos.x;
        z = player->pos.z - work->pos.z;
        ang = (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
        motion->speed.x = njCos(ang) * EGGMAN_SPEED;
        motion->speed.z = njSin(ang) * EGGMAN_SPEED;
        VARS(work)->chase =
            (Sint16)(((rand() * RAND_SCALE) + 1.0F) * CHASE_TIME);
        dat_0ca91ce1 = 0;
        work->mode = EGGMAN_CHASE;
        break;
    case EGGMAN_CHASE:
        motion->speed.y = 0.0F;
        x = work->pos.x - player->pos.x;
        z = work->pos.z - player->pos.z;
        distance = (x * x) + (z * z);
        if (distance < SQUARE(NEAR_PLAYER)) {
            /* @hack spelling: the height stored in distance, which gives it
             * FR4 at 0x0c905ac8. tested directly, it takes FR1, and 3 of 335
             * words differ */
            distance = work->pos.y - player->pos.y;
            if (distance < EGGMAN_CLEAR) {
                motion->speed.y = EGGMAN_RISE;
            }
        } else {
            motion->speed.y = EGGMAN_SINK;
        }
        njAddVector(&work->pos, &motion->speed);
        work->pos.y = CLAMP(work->pos.y, EGGMAN_LOW, EGGMAN_HIGH);
        if (chaos_distance_across(&boss_work->pos, &work->pos) < NEAR_BOSS ||
            chaos6_bombs_paused()) {
            work->mode = EGGMAN_ASCEND;
            break;
        }
        --VARS(work)->chase;
        if (VARS(work)->chase <= CHASE_LEFT) {
            work->mode = EGGMAN_AIM;
            break;
        }
        ++VARS(work)->bomb_steps;
        if (VARS(work)->bomb_steps > BOMB_WAIT) {
            VARS(work)->bomb_steps = 0;
            chaos6_eggman_drop_bomb();
        }
        break;
    case EGGMAN_ASCEND:
        work->pos.y += EGGMAN_UP;
        if (work->pos.y >= EGGMAN_HIGH) {
            eggman_away = 0;
            work->mode = EGGMAN_FLY;
        }
        break;
    case EGGMAN_FLY:
        ++eggman_away;
        work->pos.x += njCos(work->ang.y) * EGGMAN_SPEED;
        work->pos.z += njSin(work->ang.y) * EGGMAN_SPEED;
        if (!chaos6_bombs_paused()) {
            ++VARS(work)->bomb_steps;
            if (VARS(work)->bomb_steps > BOMB_WAIT) {
                VARS(work)->bomb_steps = 0;
                chaos6_eggman_drop_bomb();
            }
            if (chaos_distance_across(&work->pos, &boss_work->pos) >
                    NEAR_BOSS &&
                eggman_away > EGGMAN_AWAY) {
                work->mode = EGGMAN_DESCEND;
            }
        }
        break;
    }
    chaos6_eggman_draw(task);
    task_step_children(task);
    shadow_draw(work, EGGMAN_SHADOW);
}

void chaos6_eggman_step(struct task *task)
{
    struct task_work *work = task->work;

    task->step = chaos6_eggman_run;
    task->draw = chaos6_eggman_draw;
    work->pos = chaos_params->center;
    work->pos.y = EGGMAN_HIGH;
    eggman_task = task;
}

void chaos6_eggman_drop_bomb(void)
{
    struct task *bomb;

    if (get_character() != CHARACTER_SONIC) {
        return;
    }
    if (boss_work->mode == BOSS_START) {
        return;
    }
    if (chaos6_bomb_count >= BOMBS) {
        return;
    }
    bomb = task_create_child(TASK_WORK | TASK_MOTION, chaos6_bomb_step,
                             eggman_task);
    if (bomb != NULL) {
        bomb->work->pos = eggman_task->work->pos;
        /* his bombs keep no slot on the ring */
        BOMB_VARS(bomb->work)->slot[0] = -1;
        dat_0ca91ce1 = 1;
        sound_play(SE_BOMB, 0, 0, 127);
        if (chaos6_hint_waiting == 0) {
            stream_request_voice(VOICE_BOMB);
        }
    }
}
