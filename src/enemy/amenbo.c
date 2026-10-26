/* the amenbo, a water strider that rows across the water in strokes and
 * chases a player it sees. beach's and twinkle's object lists name it
 * E AMEMB */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "enemy/amenbo.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 12
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 12
#include "enemy/amenbo/text.inc"

enum amenbo_mode {
    AMENBO_ROW,
    AMENBO_REST,
    AMENBO_CHASE,
    AMENBO_HIT,
    AMENBO_BURST
};

/* the amenbo's bits in its work flags. amenbo_turn turns it toward home
 * while AMENBO_HOME or AMENBO_TURNED_BACK is set */
#define AMENBO_NEW_STROKE 0x100 /* amenbo_animate starts the action over */
#define AMENBO_HOME 0x200 /* amenbo.c never sets it */
#define AMENBO_SEES 0x400 /* player 0 or 1 is in sight */
/* a player within sight_distance is to its left */
#define AMENBO_PLAYER_LEFT 0x800
/* water wasn't the nearest floor under it this step. amenbo_keep_on_water
 * put it back where it was and turned its speed round */
#define AMENBO_TURNED_BACK 0x1000

struct amenbo_vars {
    /* the strokes that ended with no player in sight since its last rest.
     * the amenbo rests at the end of the stroke that brings them to
     * work->tick + 2. work->tick is 2 to 5 at random, drawn again at each
     * rest */
    Uint8 strokes;
};

#define VARS(work) ((struct amenbo_vars *)&(work)->vars)

/* a stroke counts its steps in work->timer. at ROW_AIM or CHASE_AIM, it
 * aims STROKE_TURN left or right of its heading. the stroke ends once the
 * timer passes ROW_STROKE or CHASE_STROKE */
#define ROW_AIM 70
#define ROW_STROKE 150
#define CHASE_AIM 15
#define CHASE_STROKE 35
#define STROKE_TURN NJM_DEG_ANG(60)
#define ROW_TURN_SPEED 0x60
#define CHASE_TURN_SPEED 0x200

/* the turn that takes +z to +x. a heading of 0 faces +x, but the model
 * faces +z, and atan2f(x, z) gives an angle from +z */
#define Z_TURN NJM_DEG_ANG(90)

/* the range placement_try_leave_by takes, squared. the amenbo stays while
 * the camera or player 0 is within 700 of it, or player 1 is Sonic and
 * within 40 */
#define AMENBO_LEAVE_RANGE (700.0F * 700.0F)

static void amenbo_draw(struct task *task);
static void amenbo_destroy(struct task *task);
static void amenbo_step(struct task *task);

static NJS_TEXNAME e_amenbo_textures[23] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
};

NJS_TEXLIST e_amenbo_texlist = {
#include "enemy/amenbo/e_amenbo_texlist.inc"
};

static NJS_MATERIAL amenbo_thigh_back_minus_x_materials[1] = {
#include "enemy/amenbo/amenbo_thigh_back_minus_x_materials.inc"
};

static Sint16 amenbo_thigh_back_minus_x_meshes0[15] = {
#include "enemy/amenbo/amenbo_thigh_back_minus_x_meshes0.inc"
};

static NJS_COLOR amenbo_thigh_back_minus_x_uvs0[12] = {
#include "enemy/amenbo/amenbo_thigh_back_minus_x_uvs0.inc"
};

static NJS_MESHSET amenbo_thigh_back_minus_x_meshsets[1] = {
#include "enemy/amenbo/amenbo_thigh_back_minus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_thigh_back_minus_x_points[6] = {
#include "enemy/amenbo/amenbo_thigh_back_minus_x_points.inc"
};

static NJS_VECTOR amenbo_thigh_back_minus_x_normals[6] = {
#include "enemy/amenbo/amenbo_thigh_back_minus_x_normals.inc"
};

static NJS_MODEL amenbo_thigh_back_minus_x_model = {
#include "enemy/amenbo/amenbo_thigh_back_minus_x_model.inc"
};

static NJS_OBJECT amenbo_thigh_back_minus_x = {
#include "enemy/amenbo/amenbo_thigh_back_minus_x.inc"
};

static NJS_MATERIAL amenbo_foot_back_minus_x_materials[3] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_materials.inc"
};

static Sint16 amenbo_foot_back_minus_x_meshes0[14] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_meshes0.inc"
};

static Sint16 amenbo_foot_back_minus_x_meshes1[7] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_meshes1.inc"
};

static Sint16 amenbo_foot_back_minus_x_meshes2[5] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_meshes2.inc"
};

static NJS_COLOR amenbo_foot_back_minus_x_uvs0[12] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_uvs0.inc"
};

static NJS_COLOR amenbo_foot_back_minus_x_uvs1[6] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_uvs1.inc"
};

static NJS_COLOR amenbo_foot_back_minus_x_uvs2[4] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_uvs2.inc"
};

static NJS_MESHSET amenbo_foot_back_minus_x_meshsets[3] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_foot_back_minus_x_points[16] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_points.inc"
};

static NJS_VECTOR amenbo_foot_back_minus_x_normals[16] = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_normals.inc"
};

static NJS_MODEL amenbo_foot_back_minus_x_model = {
#include "enemy/amenbo/amenbo_foot_back_minus_x_model.inc"
};

static NJS_OBJECT amenbo_foot_back_minus_x = {
#include "enemy/amenbo/amenbo_foot_back_minus_x.inc"
};

static NJS_MATERIAL amenbo_shin_back_minus_x_materials[2] = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_materials.inc"
};

static Sint16 amenbo_shin_back_minus_x_meshes0[32] = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_meshes0.inc"
};

static Sint16 amenbo_shin_back_minus_x_meshes1[9] = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_meshes1.inc"
};

static NJS_COLOR amenbo_shin_back_minus_x_uvs0[24] = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_uvs0.inc"
};

static NJS_COLOR amenbo_shin_back_minus_x_uvs1[8] = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_uvs1.inc"
};

static NJS_MESHSET amenbo_shin_back_minus_x_meshsets[2] = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_shin_back_minus_x_points[12] = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_points.inc"
};

static NJS_VECTOR amenbo_shin_back_minus_x_normals[12] = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_normals.inc"
};

static NJS_MODEL amenbo_shin_back_minus_x_model = {
#include "enemy/amenbo/amenbo_shin_back_minus_x_model.inc"
};

static NJS_OBJECT amenbo_shin_back_minus_x = {
#include "enemy/amenbo/amenbo_shin_back_minus_x.inc"
};

static NJS_OBJECT amenbo_shin_tip_back_minus_x = {
#include "enemy/amenbo/amenbo_shin_tip_back_minus_x.inc"
};

static NJS_OBJECT amenbo_shin_bone_back_minus_x = {
#include "enemy/amenbo/amenbo_shin_bone_back_minus_x.inc"
};

static NJS_OBJECT amenbo_thigh_bone_back_minus_x = {
#include "enemy/amenbo/amenbo_thigh_bone_back_minus_x.inc"
};

static NJS_OBJECT amenbo_hip_back_minus_x = {
#include "enemy/amenbo/amenbo_hip_back_minus_x.inc"
};

static NJS_MATERIAL amenbo_thigh_front_minus_x_materials[1] = {
#include "enemy/amenbo/amenbo_thigh_front_minus_x_materials.inc"
};

static Sint16 amenbo_thigh_front_minus_x_meshes0[15] = {
#include "enemy/amenbo/amenbo_thigh_front_minus_x_meshes0.inc"
};

static NJS_COLOR amenbo_thigh_front_minus_x_uvs0[12] = {
#include "enemy/amenbo/amenbo_thigh_front_minus_x_uvs0.inc"
};

static NJS_MESHSET amenbo_thigh_front_minus_x_meshsets[1] = {
#include "enemy/amenbo/amenbo_thigh_front_minus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_thigh_front_minus_x_points[6] = {
#include "enemy/amenbo/amenbo_thigh_front_minus_x_points.inc"
};

static NJS_VECTOR amenbo_thigh_front_minus_x_normals[6] = {
#include "enemy/amenbo/amenbo_thigh_front_minus_x_normals.inc"
};

static NJS_MODEL amenbo_thigh_front_minus_x_model = {
#include "enemy/amenbo/amenbo_thigh_front_minus_x_model.inc"
};

static NJS_OBJECT amenbo_thigh_front_minus_x = {
#include "enemy/amenbo/amenbo_thigh_front_minus_x.inc"
};

static NJS_MATERIAL amenbo_foot_front_minus_x_materials[3] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_materials.inc"
};

static Sint16 amenbo_foot_front_minus_x_meshes0[14] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_meshes0.inc"
};

static Sint16 amenbo_foot_front_minus_x_meshes1[7] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_meshes1.inc"
};

static Sint16 amenbo_foot_front_minus_x_meshes2[5] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_meshes2.inc"
};

static NJS_COLOR amenbo_foot_front_minus_x_uvs0[12] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_uvs0.inc"
};

static NJS_COLOR amenbo_foot_front_minus_x_uvs1[6] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_uvs1.inc"
};

static NJS_COLOR amenbo_foot_front_minus_x_uvs2[4] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_uvs2.inc"
};

static NJS_MESHSET amenbo_foot_front_minus_x_meshsets[3] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_foot_front_minus_x_points[16] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_points.inc"
};

static NJS_VECTOR amenbo_foot_front_minus_x_normals[16] = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_normals.inc"
};

static NJS_MODEL amenbo_foot_front_minus_x_model = {
#include "enemy/amenbo/amenbo_foot_front_minus_x_model.inc"
};

static NJS_OBJECT amenbo_foot_front_minus_x = {
#include "enemy/amenbo/amenbo_foot_front_minus_x.inc"
};

static NJS_MATERIAL amenbo_shin_front_minus_x_materials[2] = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_materials.inc"
};

static Sint16 amenbo_shin_front_minus_x_meshes0[32] = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_meshes0.inc"
};

static Sint16 amenbo_shin_front_minus_x_meshes1[9] = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_meshes1.inc"
};

static NJS_COLOR amenbo_shin_front_minus_x_uvs0[24] = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_uvs0.inc"
};

static NJS_COLOR amenbo_shin_front_minus_x_uvs1[8] = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_uvs1.inc"
};

static NJS_MESHSET amenbo_shin_front_minus_x_meshsets[2] = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_shin_front_minus_x_points[12] = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_points.inc"
};

static NJS_VECTOR amenbo_shin_front_minus_x_normals[12] = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_normals.inc"
};

static NJS_MODEL amenbo_shin_front_minus_x_model = {
#include "enemy/amenbo/amenbo_shin_front_minus_x_model.inc"
};

static NJS_OBJECT amenbo_shin_front_minus_x = {
#include "enemy/amenbo/amenbo_shin_front_minus_x.inc"
};

static NJS_OBJECT amenbo_shin_tip_front_minus_x = {
#include "enemy/amenbo/amenbo_shin_tip_front_minus_x.inc"
};

static NJS_OBJECT amenbo_shin_bone_front_minus_x = {
#include "enemy/amenbo/amenbo_shin_bone_front_minus_x.inc"
};

static NJS_OBJECT amenbo_thigh_bone_front_minus_x = {
#include "enemy/amenbo/amenbo_thigh_bone_front_minus_x.inc"
};

static NJS_OBJECT amenbo_hip_front_minus_x = {
#include "enemy/amenbo/amenbo_hip_front_minus_x.inc"
};

static NJS_MATERIAL amenbo_thigh_back_plus_x_materials[1] = {
#include "enemy/amenbo/amenbo_thigh_back_plus_x_materials.inc"
};

static Sint16 amenbo_thigh_back_plus_x_meshes0[15] = {
#include "enemy/amenbo/amenbo_thigh_back_plus_x_meshes0.inc"
};

static NJS_COLOR amenbo_thigh_back_plus_x_uvs0[12] = {
#include "enemy/amenbo/amenbo_thigh_back_plus_x_uvs0.inc"
};

static NJS_MESHSET amenbo_thigh_back_plus_x_meshsets[1] = {
#include "enemy/amenbo/amenbo_thigh_back_plus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_thigh_back_plus_x_points[6] = {
#include "enemy/amenbo/amenbo_thigh_back_plus_x_points.inc"
};

static NJS_VECTOR amenbo_thigh_back_plus_x_normals[6] = {
#include "enemy/amenbo/amenbo_thigh_back_plus_x_normals.inc"
};

static NJS_MODEL amenbo_thigh_back_plus_x_model = {
#include "enemy/amenbo/amenbo_thigh_back_plus_x_model.inc"
};

static NJS_OBJECT amenbo_thigh_back_plus_x = {
#include "enemy/amenbo/amenbo_thigh_back_plus_x.inc"
};

static NJS_MATERIAL amenbo_foot_back_plus_x_materials[3] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_materials.inc"
};

static Sint16 amenbo_foot_back_plus_x_meshes0[14] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_meshes0.inc"
};

static Sint16 amenbo_foot_back_plus_x_meshes1[7] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_meshes1.inc"
};

static Sint16 amenbo_foot_back_plus_x_meshes2[5] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_meshes2.inc"
};

static NJS_COLOR amenbo_foot_back_plus_x_uvs0[12] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_uvs0.inc"
};

static NJS_COLOR amenbo_foot_back_plus_x_uvs1[6] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_uvs1.inc"
};

static NJS_COLOR amenbo_foot_back_plus_x_uvs2[4] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_uvs2.inc"
};

static NJS_MESHSET amenbo_foot_back_plus_x_meshsets[3] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_foot_back_plus_x_points[16] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_points.inc"
};

static NJS_VECTOR amenbo_foot_back_plus_x_normals[16] = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_normals.inc"
};

static NJS_MODEL amenbo_foot_back_plus_x_model = {
#include "enemy/amenbo/amenbo_foot_back_plus_x_model.inc"
};

static NJS_OBJECT amenbo_foot_back_plus_x = {
#include "enemy/amenbo/amenbo_foot_back_plus_x.inc"
};

static NJS_MATERIAL amenbo_shin_back_plus_x_materials[2] = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_materials.inc"
};

static Sint16 amenbo_shin_back_plus_x_meshes0[32] = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_meshes0.inc"
};

static Sint16 amenbo_shin_back_plus_x_meshes1[9] = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_meshes1.inc"
};

static NJS_COLOR amenbo_shin_back_plus_x_uvs0[24] = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_uvs0.inc"
};

static NJS_COLOR amenbo_shin_back_plus_x_uvs1[8] = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_uvs1.inc"
};

static NJS_MESHSET amenbo_shin_back_plus_x_meshsets[2] = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_shin_back_plus_x_points[12] = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_points.inc"
};

static NJS_VECTOR amenbo_shin_back_plus_x_normals[12] = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_normals.inc"
};

static NJS_MODEL amenbo_shin_back_plus_x_model = {
#include "enemy/amenbo/amenbo_shin_back_plus_x_model.inc"
};

static NJS_OBJECT amenbo_shin_back_plus_x = {
#include "enemy/amenbo/amenbo_shin_back_plus_x.inc"
};

static NJS_OBJECT amenbo_shin_tip_back_plus_x = {
#include "enemy/amenbo/amenbo_shin_tip_back_plus_x.inc"
};

static NJS_OBJECT amenbo_shin_bone_back_plus_x = {
#include "enemy/amenbo/amenbo_shin_bone_back_plus_x.inc"
};

static NJS_OBJECT amenbo_thigh_bone_back_plus_x = {
#include "enemy/amenbo/amenbo_thigh_bone_back_plus_x.inc"
};

static NJS_OBJECT amenbo_hip_back_plus_x = {
#include "enemy/amenbo/amenbo_hip_back_plus_x.inc"
};

static NJS_MATERIAL amenbo_thigh_front_plus_x_materials[1] = {
#include "enemy/amenbo/amenbo_thigh_front_plus_x_materials.inc"
};

static Sint16 amenbo_thigh_front_plus_x_meshes0[15] = {
#include "enemy/amenbo/amenbo_thigh_front_plus_x_meshes0.inc"
};

static NJS_COLOR amenbo_thigh_front_plus_x_uvs0[12] = {
#include "enemy/amenbo/amenbo_thigh_front_plus_x_uvs0.inc"
};

static NJS_MESHSET amenbo_thigh_front_plus_x_meshsets[1] = {
#include "enemy/amenbo/amenbo_thigh_front_plus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_thigh_front_plus_x_points[6] = {
#include "enemy/amenbo/amenbo_thigh_front_plus_x_points.inc"
};

static NJS_VECTOR amenbo_thigh_front_plus_x_normals[6] = {
#include "enemy/amenbo/amenbo_thigh_front_plus_x_normals.inc"
};

static NJS_MODEL amenbo_thigh_front_plus_x_model = {
#include "enemy/amenbo/amenbo_thigh_front_plus_x_model.inc"
};

static NJS_OBJECT amenbo_thigh_front_plus_x = {
#include "enemy/amenbo/amenbo_thigh_front_plus_x.inc"
};

static NJS_MATERIAL amenbo_foot_front_plus_x_materials[3] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_materials.inc"
};

static Sint16 amenbo_foot_front_plus_x_meshes0[14] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_meshes0.inc"
};

static Sint16 amenbo_foot_front_plus_x_meshes1[7] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_meshes1.inc"
};

static Sint16 amenbo_foot_front_plus_x_meshes2[5] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_meshes2.inc"
};

static NJS_COLOR amenbo_foot_front_plus_x_uvs0[12] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_uvs0.inc"
};

static NJS_COLOR amenbo_foot_front_plus_x_uvs1[6] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_uvs1.inc"
};

static NJS_COLOR amenbo_foot_front_plus_x_uvs2[4] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_uvs2.inc"
};

static NJS_MESHSET amenbo_foot_front_plus_x_meshsets[3] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_foot_front_plus_x_points[16] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_points.inc"
};

static NJS_VECTOR amenbo_foot_front_plus_x_normals[16] = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_normals.inc"
};

static NJS_MODEL amenbo_foot_front_plus_x_model = {
#include "enemy/amenbo/amenbo_foot_front_plus_x_model.inc"
};

static NJS_OBJECT amenbo_foot_front_plus_x = {
#include "enemy/amenbo/amenbo_foot_front_plus_x.inc"
};

static NJS_MATERIAL amenbo_shin_front_plus_x_materials[2] = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_materials.inc"
};

static Sint16 amenbo_shin_front_plus_x_meshes0[32] = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_meshes0.inc"
};

static Sint16 amenbo_shin_front_plus_x_meshes1[9] = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_meshes1.inc"
};

static NJS_COLOR amenbo_shin_front_plus_x_uvs0[24] = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_uvs0.inc"
};

static NJS_COLOR amenbo_shin_front_plus_x_uvs1[8] = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_uvs1.inc"
};

static NJS_MESHSET amenbo_shin_front_plus_x_meshsets[2] = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_meshsets.inc"
};

static NJS_POINT3 amenbo_shin_front_plus_x_points[12] = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_points.inc"
};

static NJS_VECTOR amenbo_shin_front_plus_x_normals[12] = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_normals.inc"
};

static NJS_MODEL amenbo_shin_front_plus_x_model = {
#include "enemy/amenbo/amenbo_shin_front_plus_x_model.inc"
};

static NJS_OBJECT amenbo_shin_front_plus_x = {
#include "enemy/amenbo/amenbo_shin_front_plus_x.inc"
};

static NJS_OBJECT amenbo_shin_tip_front_plus_x = {
#include "enemy/amenbo/amenbo_shin_tip_front_plus_x.inc"
};

static NJS_OBJECT amenbo_shin_bone_front_plus_x = {
#include "enemy/amenbo/amenbo_shin_bone_front_plus_x.inc"
};

static NJS_OBJECT amenbo_thigh_bone_front_plus_x = {
#include "enemy/amenbo/amenbo_thigh_bone_front_plus_x.inc"
};

static NJS_OBJECT amenbo_hip_front_plus_x = {
#include "enemy/amenbo/amenbo_hip_front_plus_x.inc"
};

static NJS_MATERIAL amenbo_tail_materials[3] = {
#include "enemy/amenbo/amenbo_tail_materials.inc"
};

static Sint16 amenbo_tail_meshes0[10] = {
#include "enemy/amenbo/amenbo_tail_meshes0.inc"
};

static Sint16 amenbo_tail_meshes1[11] = {
#include "enemy/amenbo/amenbo_tail_meshes1.inc"
};

static Sint16 amenbo_tail_meshes2[5] = {
#include "enemy/amenbo/amenbo_tail_meshes2.inc"
};

static NJS_COLOR amenbo_tail_uvs0[8] = {
#include "enemy/amenbo/amenbo_tail_uvs0.inc"
};

static NJS_COLOR amenbo_tail_uvs1[10] = {
#include "enemy/amenbo/amenbo_tail_uvs1.inc"
};

static NJS_COLOR amenbo_tail_uvs2[4] = {
#include "enemy/amenbo/amenbo_tail_uvs2.inc"
};

static NJS_MESHSET amenbo_tail_meshsets[3] = {
#include "enemy/amenbo/amenbo_tail_meshsets.inc"
};

static NJS_POINT3 amenbo_tail_points[9] = {
#include "enemy/amenbo/amenbo_tail_points.inc"
};

static NJS_VECTOR amenbo_tail_normals[9] = {
#include "enemy/amenbo/amenbo_tail_normals.inc"
};

static NJS_MODEL amenbo_tail_model = {
#include "enemy/amenbo/amenbo_tail_model.inc"
};

static NJS_OBJECT amenbo_tail = {
#include "enemy/amenbo/amenbo_tail.inc"
};

static NJS_MATERIAL amenbo_mouth_materials[4] = {
#include "enemy/amenbo/amenbo_mouth_materials.inc"
};

static Sint16 amenbo_mouth_meshes0[10] = {
#include "enemy/amenbo/amenbo_mouth_meshes0.inc"
};

static Sint16 amenbo_mouth_meshes1[11] = {
#include "enemy/amenbo/amenbo_mouth_meshes1.inc"
};

static Sint16 amenbo_mouth_meshes2[9] = {
#include "enemy/amenbo/amenbo_mouth_meshes2.inc"
};

static Sint16 amenbo_mouth_meshes3[4] = {
#include "enemy/amenbo/amenbo_mouth_meshes3.inc"
};

static NJS_COLOR amenbo_mouth_uvs0[8] = {
#include "enemy/amenbo/amenbo_mouth_uvs0.inc"
};

static NJS_COLOR amenbo_mouth_uvs1[10] = {
#include "enemy/amenbo/amenbo_mouth_uvs1.inc"
};

static NJS_COLOR amenbo_mouth_uvs2[8] = {
#include "enemy/amenbo/amenbo_mouth_uvs2.inc"
};

static NJS_COLOR amenbo_mouth_uvs3[3] = {
#include "enemy/amenbo/amenbo_mouth_uvs3.inc"
};

static NJS_MESHSET amenbo_mouth_meshsets[4] = {
#include "enemy/amenbo/amenbo_mouth_meshsets.inc"
};

static NJS_POINT3 amenbo_mouth_points[14] = {
#include "enemy/amenbo/amenbo_mouth_points.inc"
};

static NJS_VECTOR amenbo_mouth_normals[14] = {
#include "enemy/amenbo/amenbo_mouth_normals.inc"
};

static NJS_MODEL amenbo_mouth_model = {
#include "enemy/amenbo/amenbo_mouth_model.inc"
};

static NJS_OBJECT amenbo_mouth = {
#include "enemy/amenbo/amenbo_mouth.inc"
};

static NJS_MATERIAL amenbo_head_materials[8] = {
#include "enemy/amenbo/amenbo_head_materials.inc"
};

static Sint16 amenbo_head_meshes0[17] = {
#include "enemy/amenbo/amenbo_head_meshes0.inc"
};

static Sint16 amenbo_head_meshes1[28] = {
#include "enemy/amenbo/amenbo_head_meshes1.inc"
};

static Sint16 amenbo_head_meshes2[5] = {
#include "enemy/amenbo/amenbo_head_meshes2.inc"
};

static Sint16 amenbo_head_meshes3[22] = {
#include "enemy/amenbo/amenbo_head_meshes3.inc"
};

static Sint16 amenbo_head_meshes4[8] = {
#include "enemy/amenbo/amenbo_head_meshes4.inc"
};

static Sint16 amenbo_head_meshes5[30] = {
#include "enemy/amenbo/amenbo_head_meshes5.inc"
};

static Sint16 amenbo_head_meshes6[30] = {
#include "enemy/amenbo/amenbo_head_meshes6.inc"
};

static Sint16 amenbo_head_meshes7[7] = {
#include "enemy/amenbo/amenbo_head_meshes7.inc"
};

static NJS_COLOR amenbo_head_uvs0[14] = {
#include "enemy/amenbo/amenbo_head_uvs0.inc"
};

static NJS_COLOR amenbo_head_uvs1[24] = {
#include "enemy/amenbo/amenbo_head_uvs1.inc"
};

static NJS_COLOR amenbo_head_uvs2[4] = {
#include "enemy/amenbo/amenbo_head_uvs2.inc"
};

static NJS_COLOR amenbo_head_uvs3[20] = {
#include "enemy/amenbo/amenbo_head_uvs3.inc"
};

static NJS_COLOR amenbo_head_uvs4[6] = {
#include "enemy/amenbo/amenbo_head_uvs4.inc"
};

static NJS_COLOR amenbo_head_uvs5[26] = {
#include "enemy/amenbo/amenbo_head_uvs5.inc"
};

static NJS_COLOR amenbo_head_uvs6[26] = {
#include "enemy/amenbo/amenbo_head_uvs6.inc"
};

static NJS_COLOR amenbo_head_uvs7[6] = {
#include "enemy/amenbo/amenbo_head_uvs7.inc"
};

static NJS_MESHSET amenbo_head_meshsets[8] = {
#include "enemy/amenbo/amenbo_head_meshsets.inc"
};

static NJS_POINT3 amenbo_head_points[56] = {
#include "enemy/amenbo/amenbo_head_points.inc"
};

static NJS_VECTOR amenbo_head_normals[56] = {
#include "enemy/amenbo/amenbo_head_normals.inc"
};

static NJS_MODEL amenbo_head_model = {
#include "enemy/amenbo/amenbo_head_model.inc"
};

static NJS_OBJECT amenbo_head = {
#include "enemy/amenbo/amenbo_head.inc"
};

static NJS_MATERIAL amenbo_body_materials[5] = {
#include "enemy/amenbo/amenbo_body_materials.inc"
};

static Sint16 amenbo_body_meshes0[26] = {
#include "enemy/amenbo/amenbo_body_meshes0.inc"
};

static Sint16 amenbo_body_meshes1[10] = {
#include "enemy/amenbo/amenbo_body_meshes1.inc"
};

static Sint16 amenbo_body_meshes2[20] = {
#include "enemy/amenbo/amenbo_body_meshes2.inc"
};

static Sint16 amenbo_body_meshes3[26] = {
#include "enemy/amenbo/amenbo_body_meshes3.inc"
};

static Sint16 amenbo_body_meshes4[40] = {
#include "enemy/amenbo/amenbo_body_meshes4.inc"
};

static NJS_COLOR amenbo_body_uvs0[20] = {
#include "enemy/amenbo/amenbo_body_uvs0.inc"
};

static NJS_COLOR amenbo_body_uvs1[8] = {
#include "enemy/amenbo/amenbo_body_uvs1.inc"
};

static NJS_COLOR amenbo_body_uvs2[16] = {
#include "enemy/amenbo/amenbo_body_uvs2.inc"
};

static NJS_COLOR amenbo_body_uvs3[20] = {
#include "enemy/amenbo/amenbo_body_uvs3.inc"
};

static NJS_COLOR amenbo_body_uvs4[32] = {
#include "enemy/amenbo/amenbo_body_uvs4.inc"
};

static NJS_MESHSET amenbo_body_meshsets[5] = {
#include "enemy/amenbo/amenbo_body_meshsets.inc"
};

static NJS_POINT3 amenbo_body_points[36] = {
#include "enemy/amenbo/amenbo_body_points.inc"
};

static NJS_VECTOR amenbo_body_normals[36] = {
#include "enemy/amenbo/amenbo_body_normals.inc"
};

static NJS_MODEL amenbo_body_model = {
#include "enemy/amenbo/amenbo_body_model.inc"
};

static NJS_OBJECT amenbo_body = {
#include "enemy/amenbo/amenbo_body.inc"
};

static NJS_OBJECT amenbo_object = {
#include "enemy/amenbo/amenbo_object.inc"
};

/* the number that ends a key array's name is its node's: amenbo_object's
 * tree counted depth first from 0 */
static NJS_MKEY_F amenbo_rest_pos1[21] = {
#include "enemy/amenbo/amenbo_rest_pos1.inc"
};

static NJS_MKEY_A amenbo_rest_ang1[21] = {
#include "enemy/amenbo/amenbo_rest_ang1.inc"
};

static NJS_MKEY_A amenbo_rest_ang2[53] = {
#include "enemy/amenbo/amenbo_rest_ang2.inc"
};

static NJS_MKEY_A amenbo_rest_ang6[60] = {
#include "enemy/amenbo/amenbo_rest_ang6.inc"
};

static NJS_MKEY_A amenbo_rest_ang7[60] = {
#include "enemy/amenbo/amenbo_rest_ang7.inc"
};

static NJS_MKEY_A amenbo_rest_ang10[60] = {
#include "enemy/amenbo/amenbo_rest_ang10.inc"
};

static NJS_MKEY_A amenbo_rest_ang13[60] = {
#include "enemy/amenbo/amenbo_rest_ang13.inc"
};

static NJS_MKEY_A amenbo_rest_ang14[60] = {
#include "enemy/amenbo/amenbo_rest_ang14.inc"
};

static NJS_MKEY_A amenbo_rest_ang17[58] = {
#include "enemy/amenbo/amenbo_rest_ang17.inc"
};

static NJS_MKEY_A amenbo_rest_ang20[60] = {
#include "enemy/amenbo/amenbo_rest_ang20.inc"
};

static NJS_MKEY_A amenbo_rest_ang21[60] = {
#include "enemy/amenbo/amenbo_rest_ang21.inc"
};

static NJS_MKEY_A amenbo_rest_ang24[60] = {
#include "enemy/amenbo/amenbo_rest_ang24.inc"
};

static NJS_MKEY_A amenbo_rest_ang27[60] = {
#include "enemy/amenbo/amenbo_rest_ang27.inc"
};

static NJS_MKEY_A amenbo_rest_ang28[60] = {
#include "enemy/amenbo/amenbo_rest_ang28.inc"
};

static NJS_MKEY_A amenbo_rest_ang31[58] = {
#include "enemy/amenbo/amenbo_rest_ang31.inc"
};

static NJS_MDATA2 amenbo_rest_mdata[33] = {
#include "enemy/amenbo/amenbo_rest_mdata.inc"
};

static NJS_MOTION amenbo_rest_motion = {
#include "enemy/amenbo/amenbo_rest_motion.inc"
};

static NJS_ACTION amenbo_rest_action = {
#include "enemy/amenbo/amenbo_rest_action.inc"
};

static NJS_MKEY_F amenbo_chase_pos1[30] = {
#include "enemy/amenbo/amenbo_chase_pos1.inc"
};

static NJS_MKEY_A amenbo_chase_ang1[26] = {
#include "enemy/amenbo/amenbo_chase_ang1.inc"
};

static NJS_MKEY_A amenbo_chase_ang2[29] = {
#include "enemy/amenbo/amenbo_chase_ang2.inc"
};

static NJS_MKEY_A amenbo_chase_ang4[29] = {
#include "enemy/amenbo/amenbo_chase_ang4.inc"
};

static NJS_MKEY_A amenbo_chase_ang6[30] = {
#include "enemy/amenbo/amenbo_chase_ang6.inc"
};

static NJS_MKEY_A amenbo_chase_ang7[28] = {
#include "enemy/amenbo/amenbo_chase_ang7.inc"
};

static NJS_MKEY_A amenbo_chase_ang10[30] = {
#include "enemy/amenbo/amenbo_chase_ang10.inc"
};

static NJS_MKEY_A amenbo_chase_ang13[30] = {
#include "enemy/amenbo/amenbo_chase_ang13.inc"
};

static NJS_MKEY_A amenbo_chase_ang14[30] = {
#include "enemy/amenbo/amenbo_chase_ang14.inc"
};

static NJS_MKEY_A amenbo_chase_ang17[30] = {
#include "enemy/amenbo/amenbo_chase_ang17.inc"
};

static NJS_MKEY_A amenbo_chase_ang20[30] = {
#include "enemy/amenbo/amenbo_chase_ang20.inc"
};

static NJS_MKEY_A amenbo_chase_ang21[28] = {
#include "enemy/amenbo/amenbo_chase_ang21.inc"
};

static NJS_MKEY_A amenbo_chase_ang24[30] = {
#include "enemy/amenbo/amenbo_chase_ang24.inc"
};

static NJS_MKEY_A amenbo_chase_ang27[30] = {
#include "enemy/amenbo/amenbo_chase_ang27.inc"
};

static NJS_MKEY_A amenbo_chase_ang28[30] = {
#include "enemy/amenbo/amenbo_chase_ang28.inc"
};

static NJS_MKEY_A amenbo_chase_ang31[30] = {
#include "enemy/amenbo/amenbo_chase_ang31.inc"
};

static NJS_MDATA2 amenbo_chase_mdata[33] = {
#include "enemy/amenbo/amenbo_chase_mdata.inc"
};

static NJS_MOTION amenbo_chase_motion = {
#include "enemy/amenbo/amenbo_chase_motion.inc"
};

static NJS_ACTION amenbo_chase_action = {
#include "enemy/amenbo/amenbo_chase_action.inc"
};

static NJS_MKEY_F amenbo_row_pos1[44] = {
#include "enemy/amenbo/amenbo_row_pos1.inc"
};

static NJS_MKEY_A amenbo_row_ang2[42] = {
#include "enemy/amenbo/amenbo_row_ang2.inc"
};

static NJS_MKEY_A amenbo_row_ang4[44] = {
#include "enemy/amenbo/amenbo_row_ang4.inc"
};

static NJS_MKEY_A amenbo_row_ang6[44] = {
#include "enemy/amenbo/amenbo_row_ang6.inc"
};

static NJS_MKEY_A amenbo_row_ang7[44] = {
#include "enemy/amenbo/amenbo_row_ang7.inc"
};

static NJS_MKEY_A amenbo_row_ang10[44] = {
#include "enemy/amenbo/amenbo_row_ang10.inc"
};

static NJS_MKEY_A amenbo_row_ang13[44] = {
#include "enemy/amenbo/amenbo_row_ang13.inc"
};

static NJS_MKEY_A amenbo_row_ang14[44] = {
#include "enemy/amenbo/amenbo_row_ang14.inc"
};

static NJS_MKEY_A amenbo_row_ang17[44] = {
#include "enemy/amenbo/amenbo_row_ang17.inc"
};

static NJS_MKEY_A amenbo_row_ang20[44] = {
#include "enemy/amenbo/amenbo_row_ang20.inc"
};

static NJS_MKEY_A amenbo_row_ang21[44] = {
#include "enemy/amenbo/amenbo_row_ang21.inc"
};

static NJS_MKEY_A amenbo_row_ang24[44] = {
#include "enemy/amenbo/amenbo_row_ang24.inc"
};

static NJS_MKEY_A amenbo_row_ang27[44] = {
#include "enemy/amenbo/amenbo_row_ang27.inc"
};

static NJS_MKEY_A amenbo_row_ang28[44] = {
#include "enemy/amenbo/amenbo_row_ang28.inc"
};

static NJS_MKEY_A amenbo_row_ang31[44] = {
#include "enemy/amenbo/amenbo_row_ang31.inc"
};

static NJS_MDATA2 amenbo_row_mdata[33] = {
#include "enemy/amenbo/amenbo_row_mdata.inc"
};

static NJS_MOTION amenbo_row_motion = {
#include "enemy/amenbo/amenbo_row_motion.inc"
};

static NJS_ACTION amenbo_row_action = {
#include "enemy/amenbo/amenbo_row_action.inc"
};

/* the body, then a smaller shape that collision_init, given a count of 1,
 * leaves out */
/* clang-format off */
static struct collision_shape amenbo_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 4.0F, 0.0F}, 13.5F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS, 0,
     {0.0F, 4.0F, 0.0F}, 8.0F, 2.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static NJS_MODEL *amenbo_pieces[] = {
    &amenbo_shin_back_minus_x_model,
    &amenbo_thigh_front_minus_x_model,
    &amenbo_foot_front_minus_x_model,
    &amenbo_shin_front_minus_x_model,
    &amenbo_thigh_back_plus_x_model,
    &amenbo_foot_back_plus_x_model,
    &amenbo_shin_back_plus_x_model,
    &amenbo_shin_front_plus_x_model,
    &amenbo_mouth_model,
    &amenbo_head_model,
    &amenbo_body_model,
    NULL,
};

void amenbo_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    struct angles slope;
    /* @hack spelling: the position read through pos, which keeps its address in
     * R11 for ground_nearest's arguments (ADD #32,R11 at 0x8c100414); read
     * through work, 117 of 124 words differ */
    NJS_POINT3 *pos;

    collision_init(task, amenbo_shapes, COLLISION_ENEMY, 1, 1);
    record = enemy_alloc_record(task, work);
    work->mode = AMENBO_ROW;
    work->submode = 0;
    work->tick = (Uint8)(rand() * (4.0F * RAND_SCALE)) + 2;
    work->timer = rand() * (ROW_STROKE * RAND_SCALE);
    pos = &work->pos;
    record->sight_distance = 200.0F * 200.0F;
    record->near_distance = 30.0F * 30.0F;
    work->pos.y = ground_nearest(pos->x, pos->y, pos->z, &slope);
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 10.0F;
    record->probe_offset.z = 0.0F;
    record->top = 10.0F;
    record->radius = 15.0F;
    record->bottom = -10.0F;
    record->floor_bounce = 1.0F;
    record->friction = 1.0F;
    record->unk_c4 = 1.0F;
    record->wall_bounce = 1.0F;
    task->motion = &record->motion;
    enemy_set_home(work, record);
    task->destructor = amenbo_destroy;
    task->step = amenbo_step;
    task->draw = amenbo_draw;
    record->flags |= ENEMY_WATER;
}

static void amenbo_aim_left(struct task_work *work,
                            struct lockon_target *record)
{
    switch (work->mode) {
    case AMENBO_ROW:
        if (TASK_TIMER(work) == ROW_AIM) {
            record->aim_y = work->ang.y + STROKE_TURN;
        }
        record->turn_speed = ROW_TURN_SPEED;
        break;
    case AMENBO_CHASE:
        if (TASK_TIMER(work) == CHASE_AIM) {
            record->aim_y = work->ang.y + STROKE_TURN;
        }
        record->turn_speed = CHASE_TURN_SPEED;
        break;
    /* @hack redundant: a default that returns, which gives the other modes an
     * RTS of their own (0x8c10052c); without it, they branch to the
     * function's, and 1 of 31 words differs */
    default:
        return;
    }
}

static void amenbo_aim_right(struct task_work *work,
                             struct lockon_target *record)
{
    switch (work->mode) {
    case AMENBO_ROW:
        if (TASK_TIMER(work) == ROW_AIM) {
            record->aim_y = work->ang.y - STROKE_TURN;
        }
        record->turn_speed = ROW_TURN_SPEED;
        break;
    case AMENBO_CHASE:
        if (TASK_TIMER(work) == CHASE_AIM) {
            record->aim_y = work->ang.y - STROKE_TURN;
        }
        record->turn_speed = CHASE_TURN_SPEED;
        break;
    /* @hack redundant: the same default, with its RTS at 0x8c10056a; without
     * it, 1 of 31 words differs */
    default:
        return;
    }
}

static void amenbo_turn(struct task_work *work, struct lockon_target *record)
{
    if (work->flags & (AMENBO_HOME | AMENBO_TURNED_BACK)) {
        enemy_turn_home(work, record);
    } else {
        switch (work->mode) {
        case AMENBO_ROW:
            amenbo_aim_left(work, record);
            work->ang.y =
                angle_approach(work->ang.y, record->aim_y, record->turn_speed);
            break;
        case AMENBO_CHASE:
            if (work->flags & AMENBO_PLAYER_LEFT) {
                amenbo_aim_left(work, record);
            } else {
                amenbo_aim_right(work, record);
            }
            work->ang.y =
                angle_approach(work->ang.y, record->aim_y, record->turn_speed);
            break;
        }
    }
}

/* when one of its shapes is hurt, the amenbo starts AMENBO_HIT with its
 * scale springing. if the hitting collision record is a player's, not an
 * attack's, player 0's speed becomes 2 straight up, whichever player it was */
static void amenbo_take_hit(struct task_work *work,
                            struct lockon_target *record)
{
    struct collision_work *collision;
    struct collision_work *hit;

    enemy_ignore((void *)player_work);
    /* @hack spelling: collision and hit read ahead of the test, which loads hit
     * in the test's delay slot (MOV.L @(24,R5),R6 at 0x8c100668). with hit
     * chained from work ahead of the test, 5 of 31 words differ, and read in
     * the test, 19 of 31 */
    collision = work->collision;
    hit = collision->hit;
    if (work->flags & COLLISION_DAMAGED) {
        work->mode = AMENBO_HIT;
        ENEMY_MARK_BEATEN(work);
        work->timer = 0;
        work->scl.z = 0.35F;
        if (hit->kind == COLLISION_PLAYER) {
            player_set_speed(0, 0.0F, 2.0F, 0.0F);
        }
    }
}

/* spring scl.y back toward 1 at speed scl.z. each call damps scl.z by 0.12
 * of itself and pulls it by 0.32 of 1 - scl.y. scl.x becomes 2 - scl.y, and
 * neither is bounded */
static void amenbo_spring_scale(struct task_work *work)
{
    Float stretch = work->scl.y - 1.0F;

    work->scl.z += -(work->scl.z * 0.12F) - (stretch * 0.32F);
    work->scl.y += work->scl.z;
    work->scl.x = 2.0F - work->scl.y;
}

/* set AMENBO_SEES while player 0 or 1 is within sight_distance and either
 * within sight_angle of the heading or within near_distance. AMENBO_PLAYER_LEFT
 * says which side a player within sight_distance is on, player 1 where
 * both are. it keeps its value while neither is */
static void amenbo_look(struct task_work *work, struct lockon_target *record)
{
    struct task_work *player;
    Float distance2;
    Angle facing;
    Angle toward;
    Angle side;
    Angle away;
    Sint8 i;

    work->flags &= ~AMENBO_SEES;
    for (i = 0; i < 2; i++) {
        player = player_work[i];
        if (player != NULL) {
            distance2 = SQUARE(player->pos.x - work->pos.x) +
                        SQUARE(player->pos.y - work->pos.y) +
                        SQUARE(player->pos.z - work->pos.z);
            if (distance2 < record->sight_distance) {
                /* @hack spelling: the heading read into facing ahead of the
                 * arctangent, which loads it into R9 before the call
                 * (MOV.L @(24,R14),R9 at 0x8c1007b8); read in place at each
                 * use, the object runs 32 bytes longer and 103 of 123 words
                 * differ */
                facing = work->ang.y;
                toward = (Angle)(atan2f(player->pos.x - work->pos.x,
                                        player->pos.z - work->pos.z) *
                                 ANGLE_PER_RADIAN) -
                         Z_TURN;
                side = angle_difference(toward, facing);
                away = angle_distance(toward, facing);
                /* @hack spelling: the two tests set AMENBO_SEES in arms of
                 * their own (OR R12,R0 at 0x8c1007e8 and 0x8c1007f8); joined
                 * with ||, the object runs 32 bytes longer and 46 of 123 words
                 * differ */
                if (away < record->sight_angle) {
                    work->flags |= AMENBO_SEES;
                } else if (distance2 < record->near_distance) {
                    work->flags |= AMENBO_SEES;
                }
                if (side < 0) {
                    work->flags |= AMENBO_PLAYER_LEFT;
                } else {
                    work->flags &= ~AMENBO_PLAYER_LEFT;
                }
            }
        }
    }
}

static void amenbo_stroke(struct task_work *work, struct lockon_target *record)
{
    Float frame = record->frame;

    switch (work->mode) {
    case AMENBO_ROW:
        if (frame > 7.0F && frame < 14.0F) {
            record->motion.accel.x = njCos(work->ang.y) * 0.3F;
            record->motion.accel.z = -(njSin(work->ang.y) * 0.3F);
        }
        if (TASK_TIMER(work)++ > ROW_STROKE) {
            work->timer = 0;
            work->flags |= AMENBO_NEW_STROKE;
            if (work->flags & AMENBO_SEES) {
                work->mode = AMENBO_CHASE;
            } else if (VARS(work)->strokes++ > work->tick) {
                work->tick = (Uint8)(rand() * (4.0F * RAND_SCALE)) + 2;
                work->mode = AMENBO_REST;
                VARS(work)->strokes = 0;
            }
            record->frame = 0.0F;
        }
        break;
    case AMENBO_CHASE:
        if (frame > 7.0F && frame < 14.0F) {
            record->motion.accel.x = njCos(work->ang.y) * 0.35F;
            record->motion.accel.z = -(njSin(work->ang.y) * 0.35F);
        }
        if (TASK_TIMER(work)++ > CHASE_STROKE) {
            work->timer = 0;
            work->flags |= AMENBO_NEW_STROKE;
            if (work->flags & AMENBO_SEES) {
                work->mode = AMENBO_CHASE;
            } else if (VARS(work)->strokes++ > work->tick) {
                work->tick = (Uint8)(rand() * (4.0F * RAND_SCALE)) + 2;
                work->mode = AMENBO_REST;
                VARS(work)->strokes = 0;
            } else {
                work->mode = AMENBO_ROW;
            }
            record->frame = 0.0F;
        }
        break;
    }
}

static void amenbo_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.accel.x += -(record->motion.speed.x * 0.04F);
    if (_builtin_fabsf(record->motion.speed.x) < 0.01F) {
        record->motion.speed.x = 0.0F;
    }
    record->motion.accel.z += -(record->motion.speed.z * 0.04F);
    if (_builtin_fabsf(record->motion.speed.z) < 0.01F) {
        record->motion.speed.z = 0.0F;
    }
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.z = 0.0F;
}

/* advance the action's frame. rowing and chasing hold the last frame until
 * a new stroke starts the action over. a rest lasts one pass of its action */
static void amenbo_animate(struct task_work *work, struct lockon_target *record)
{
    NJS_ACTION *action = record->action;
    Float frames = (Float)action->motion->nbFrame;

    switch (work->mode) {
    case AMENBO_ROW:
        if (work->flags & AMENBO_NEW_STROKE) {
            record->frame = 0.0F;
            work->flags &= ~AMENBO_NEW_STROKE;
        }
        record->frame += 0.7F;
        if (record->frame > frames) {
            record->frame = frames - 0.1F;
        }
        /* @hack spelling: a return where a break would do, which gives the arm
         * an RTS of its own (0x8c100b84); as a break, it branches to the
         * function's, and 1 of 92 words differs */
        return;
    case AMENBO_CHASE:
        if (work->flags & AMENBO_NEW_STROKE) {
            record->frame = 0.0F;
            work->flags &= ~AMENBO_NEW_STROKE;
        }
        record->frame += 1.2F;
        if (record->frame > frames) {
            record->frame = frames - 0.1F;
            /* @hack redundant: the same return, with its RTS at 0x8c100bac;
             * without it, 1 of 92 words differs */
            return;
        }
        break;
    case AMENBO_REST:
        record->frame += 0.8F;
        if (record->frame > frames) {
            record->frame = 0.0F;
            /* @bug logic: the test reads AMENBO_SEES in the record's
             * flags, not the work's (MOV.W @(R0,R5),R3 at 0x8c100bc8). there
             * the bit is ENEMY_UNK_400, which enemy_alloc_record sets. only
             * enemy_take_hit clears it, and amenbo.c never calls
             * enemy_take_hit. so the test at 0x8c100bce always holds, and
             * every rest ends in AMENBO_CHASE */
            if (record->flags & AMENBO_SEES) {
                work->mode = AMENBO_CHASE;
            } else {
                work->mode = AMENBO_ROW;
            }
        }
        break;
    /* @hack redundant: a default that returns, with its RTS at 0x8c100b1e;
     * without it, 1 of 92 words differs */
    default:
        return;
    }
}

/* draw the amenbo while it's in view. only then does its scale spring back */
static void amenbo_draw_body(struct task_work *work,
                             struct lockon_target *record)
{
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    amenbo_spring_scale(work);
    njSetTexture(&e_amenbo_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + Z_TURN);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    draw_action(record->action, record->frame);
    njPopMatrix(1);
}

/* the task's draw, which task_draw_all runs under the pause menu. it draws
 * as amenbo_draw_body does, but leaves the spring as it is */
static void amenbo_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&e_amenbo_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + Z_TURN);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    draw_action(record->action, record->frame);
    njPopMatrix(1);
}

static void amenbo_keep_on_water(struct task_work *work,
                                 struct lockon_target *record)
{
    enemy_collide(work, record);
    if (!(record->flags & ENEMY_ON_WATER)) {
        enemy_restore_pos(work, record);
        record->motion.speed.x *= -1.0F;
        record->motion.speed.z *= -1.0F;
        work->ang.x = 0;
        work->ang.z = 0;
        work->flags |= AMENBO_TURNED_BACK;
    } else {
        work->flags &= ~AMENBO_TURNED_BACK;
    }
}

static void amenbo_edit_draw(struct task_work *work)
{
    njSetTexture(&e_amenbo_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + Z_TURN);
    draw_tree(&amenbo_object);
    njPopMatrix(1);
}

static void amenbo_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void amenbo_burst(struct task_work *work, struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_break(work, amenbo_pieces, &e_amenbo_texlist);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    animal_create_random(1, work->pos.x, work->pos.y + 15.0F, work->pos.z);
}

static void amenbo_row(struct task_work *work, struct lockon_target *record)
{
    record->action = &amenbo_row_action;
    record->sight_angle = NJM_DEG_ANG(80);
    amenbo_take_hit(work, record);
    amenbo_look(work, record);
    amenbo_turn(work, record);
    amenbo_stroke(work, record);
    amenbo_move(work, record);
    amenbo_keep_on_water(work, record);
    amenbo_animate(work, record);
    amenbo_draw_body(work, record);
}

static void amenbo_chase(struct task_work *work, struct lockon_target *record)
{
    record->sight_angle = NJM_DEG_ANG(180);
    record->action = &amenbo_chase_action;
    amenbo_take_hit(work, record);
    amenbo_look(work, record);
    amenbo_turn(work, record);
    amenbo_stroke(work, record);
    amenbo_move(work, record);
    amenbo_keep_on_water(work, record);
    amenbo_animate(work, record);
    amenbo_draw_body(work, record);
}

static void amenbo_rest(struct task_work *work, struct lockon_target *record)
{
    enemy_target_distance2(work, record);
    record->action = &amenbo_rest_action;
    record->sight_angle = NJM_DEG_ANG(180);
    amenbo_take_hit(work, record);
    amenbo_look(work, record);
    amenbo_keep_on_water(work, record);
    amenbo_animate(work, record);
    amenbo_draw_body(work, record);
}

static void amenbo_hit(struct task_work *work, struct lockon_target *record)
{
    amenbo_keep_on_water(work, record);
    amenbo_animate(work, record);
    amenbo_draw_body(work, record);
    if (TASK_TIMER(work)++ > 15) {
        work->mode = AMENBO_BURST;
    }
}

static void amenbo_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        amenbo_edit_draw(work);
        return;
    }
    /* @hack redundant: a discarded read of player 0's mode, which SHC drops
     * but which keeps the image's load of player_work[0] (MOV.L @R4,R4 at
     * 0x8c1010a6); without it, the object runs 54 bytes short and 87 of 88
     * words differ */
    (void)player->mode;
    if (placement_try_leave_by(task, AMENBO_LEAVE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case AMENBO_ROW:
        amenbo_row(work, record);
        break;
    case AMENBO_CHASE:
        amenbo_chase(work, record);
        break;
    case AMENBO_REST:
        amenbo_rest(work, record);
        break;
    case AMENBO_HIT:
        amenbo_hit(work, record);
        break;
    case AMENBO_BURST:
        amenbo_burst(work, record);
        /* @hack spelling: the two calls as one expression, which keeps the call
         * to placement_kill a JSR (0x8c10111c); as two statements, SHC
         * makes it a tail jump, the object runs 54 bytes short and 27 of 88
         * words differ */
        enemy_end(task, work, record), placement_kill(task);
        return;
    /* @hack redundant: an empty default, which puts enemy_save_pos in R3 and
     * collision_register in R2 (0x8c101160, 0x8c10116a); without it, the
     * two trade and 4 of 88 words differ */
    default:
        break;
    }
    enemy_save_pos(work, record);
    collision_register(work);
}
