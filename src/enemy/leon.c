/* the leon, a chameleon that stalks player 0 unseen, shows itself to lick
 * at it, and hides again. windy's, ruin's and shelter's object lists name
 * it E LEON */
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
#include "draw/pose.h"
#include "effect/explosion.h"
#include "enemy/enemy.h"
#include "enemy/leon.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/lockon.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 3
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 13
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#include "enemy/leon/text.inc"

enum leon_mode {
    /* it walks ahead, hidden, until player 0 comes within STALK_RANGE */
    LEON_WAIT = 0,
    /* it creeps, hidden, toward a spot near player 0. near its target and on
     * screen, it shows itself */
    LEON_STALK = 1,
    LEON_LICK = 2, /* it licks at player 0, then hides and stalks again */
    LEON_HIT = 4, /* it walks on, then bursts. leon.c never enters it */
    LEON_BURST = 5
};

/* in work->submode: leon_draw_blend blends action into next_action, then
 * the action plays */
enum leon_submode { LEON_BLENDING, LEON_PLAYING };

/* the leon's bits in work->flags */
#define LEON_TONGUE_OUT 0x100
/* the lick action plays, and leon_aim_head leaves the head as it is */
#define LEON_LICKING 0x200
/* leon_step registers the leon for collision only while this is set.
 * leon_take_hit clears it as the leon bursts */
#define LEON_SOLID 0x400
#define LEON_HIDDEN 0x1000 /* leon_fade fades the leon out, not in */
/* it targets player 0 itself, not a spot near it. leon_pick_target clears
 * the bit once the leon is off screen */
#define LEON_FOUND 0x2000
/* enemy_on_screen found the leon on screen at leon_pick_target's last call */
#define LEON_ON_SCREEN 0x4000

struct leon_vars {
    Uint8 unk_00;
    /* the steps leon_hit has run. ENEMY_MARK_BEATEN zeroes it */
    Uint8 since_hit;
};

#define VARS(work) ((struct leon_vars *)&(work)->vars)

/* the licks in this LEON_LICK, counted as the tongue comes back in */
#define LICKS(work) ((work)->tick)

/* how far leon_draw_blend has blended action into next_action, 0 to 1. it
 * lies in motion.record, a pointer only a player's task uses */
#define BLEND(record) (*(Float *)&(record)->motion.record)
/* the id of the leon's piece of the master emerald. it comes from the
 * placement's ang.z, the set editor's EMERALD */
#define PIECE(record) ((record)->motion.phase)
/* the tongue's length, which scales its model along z */
#define TONGUE_LENGTH(record) ((record)->motion.gravity)
/* added to TONGUE_LENGTH a step */
#define TONGUE_SPEED(record) ((record)->unk_80)
/* the frames the action advances each step the leon stalks. leon_walk sets
 * it with the leon's speed */
#define PACE(record) ((record)->unk_74)
/* the set editor's SEARCH RANGE and MOVE RANGE, each plus 100. the leon
 * shows itself only within SEARCH_RANGE of its target. it targets home
 * while it is farther than MOVE_RANGE + 100 from home */
#define SEARCH_RANGE(record) ((record)->unk_78)
#define MOVE_RANGE(record) ((record)->unk_7c)
#define HEAD_FRAME(record) ((record)->unk_84) /* leon_jaw_action's frame */
/* the degrees each tail segment pitches from the one before, besides the
 * sway */
#define CURL(record) ((record)->unk_88)
/* the pitch of the tail's root, before the head's share */
#define TAIL_LIFT(record) ((record)->heading)
/* the alpha leon_draw_body and leon_draw_blend give draw_set_material: 0
 * shows the leon whole, and -1 hides it */
#define FADE(record) ((record)->unk_8c)
/* which of enemy_target_near_player's spots, 0 to 3, the leon stalks */
#define SPOT(record) ((record)->unk_4c)

/* the head's angles from the body's, in record->angles */
enum leon_angle { LEON_HEAD_PITCH, LEON_HEAD_YAW };

/* leon_aim_head keeps the head's pitch and yaw within their _MIN and _MAX,
 * and turns each toward the target by at most its _SPEED a step */
#define HEAD_PITCH_MIN (-0x37ff)
#define HEAD_PITCH_MAX 0x1000
#define HEAD_YAW_MIN (-0x2fff)
#define HEAD_YAW_MAX 0x2000
#define HEAD_PITCH_SPEED 0x160
#define HEAD_YAW_SPEED 0x1a0

/* the lick action's frame at which the tongue shoots out. the action holds
 * there until the tongue is back in */
#define TONGUE_FRAME 15.0F

/* the tail's curl and lift move toward their _OUT values while the tongue
 * is out, and toward their _IN values while it is in */
#define CURL_OUT (-0.6F)
#define CURL_IN 6.5F
#define LIFT_OUT 0x200
#define LIFT_IN 0x1100

/* a heading of 0 faces +x. the model faces +z, so Z_TURN turns it to face
 * its heading */
#define Z_TURN NJM_DEG_ANG(90)

#define STALK_RANGE 300.0F

/* the range placement_try_leave_by takes, squared. the leon stays while
 * the camera or player 0 is within 700 of it, or player 1 is Sonic and
 * within 40 */
#define LEON_LEAVE_RANGE (700.0F * 700.0F)

static void leon_destroy(struct task *task);
static void leon_step(struct task *task);

static NJS_MATERIAL leon_tail_crest_materials[1] = {
#include "enemy/leon/leon_tail_crest_materials.inc"
};

static Sint16 leon_tail_crest_meshes0[24] = {
#include "enemy/leon/leon_tail_crest_meshes0.inc"
};

static NJS_COLOR leon_tail_crest_uvs0[20] = {
#include "enemy/leon/leon_tail_crest_uvs0.inc"
};

static NJS_MESHSET leon_tail_crest_meshsets[1] = {
#include "enemy/leon/leon_tail_crest_meshsets.inc"
};

static NJS_POINT3 leon_tail_crest_points[8] = {
#include "enemy/leon/leon_tail_crest_points.inc"
};

static NJS_VECTOR leon_tail_crest_normals[8] = {
#include "enemy/leon/leon_tail_crest_normals.inc"
};

static NJS_MODEL leon_tail_crest_model = {
#include "enemy/leon/leon_tail_crest_model.inc"
};

static NJS_OBJECT leon_tail_crest = {
#include "enemy/leon/leon_tail_crest.inc"
};

static NJS_MATERIAL leon_tail_materials[3] = {
#include "enemy/leon/leon_tail_materials.inc"
};

static Sint16 leon_tail_meshes0[21] = {
#include "enemy/leon/leon_tail_meshes0.inc"
};

static Sint16 leon_tail_meshes1[12] = {
#include "enemy/leon/leon_tail_meshes1.inc"
};

static Sint16 leon_tail_meshes2[7] = {
#include "enemy/leon/leon_tail_meshes2.inc"
};

static NJS_COLOR leon_tail_uvs0[18] = {
#include "enemy/leon/leon_tail_uvs0.inc"
};

static NJS_COLOR leon_tail_uvs1[10] = {
#include "enemy/leon/leon_tail_uvs1.inc"
};

static NJS_COLOR leon_tail_uvs2[6] = {
#include "enemy/leon/leon_tail_uvs2.inc"
};

static NJS_MESHSET leon_tail_meshsets[3] = {
#include "enemy/leon/leon_tail_meshsets.inc"
};

static NJS_POINT3 leon_tail_points[13] = {
#include "enemy/leon/leon_tail_points.inc"
};

static NJS_VECTOR leon_tail_normals[13] = {
#include "enemy/leon/leon_tail_normals.inc"
};

static NJS_MODEL leon_tail_model = {
#include "enemy/leon/leon_tail_model.inc"
};

static NJS_OBJECT leon_tail = {
#include "enemy/leon/leon_tail.inc"
};

static NJS_MATERIAL leon_thigh_back_minus_x_materials[1] = {
#include "enemy/leon/leon_thigh_back_minus_x_materials.inc"
};

static Sint16 leon_thigh_back_minus_x_meshes0[9] = {
#include "enemy/leon/leon_thigh_back_minus_x_meshes0.inc"
};

static NJS_COLOR leon_thigh_back_minus_x_uvs0[8] = {
#include "enemy/leon/leon_thigh_back_minus_x_uvs0.inc"
};

static NJS_MESHSET leon_thigh_back_minus_x_meshsets[1] = {
#include "enemy/leon/leon_thigh_back_minus_x_meshsets.inc"
};

static NJS_POINT3 leon_thigh_back_minus_x_points[6] = {
#include "enemy/leon/leon_thigh_back_minus_x_points.inc"
};

static NJS_VECTOR leon_thigh_back_minus_x_normals[6] = {
#include "enemy/leon/leon_thigh_back_minus_x_normals.inc"
};

static NJS_MODEL leon_thigh_back_minus_x_model = {
#include "enemy/leon/leon_thigh_back_minus_x_model.inc"
};

static NJS_OBJECT leon_thigh_back_minus_x = {
#include "enemy/leon/leon_thigh_back_minus_x.inc"
};

static NJS_MATERIAL leon_shin_back_minus_x_materials[3] = {
#include "enemy/leon/leon_shin_back_minus_x_materials.inc"
};

static Sint16 leon_shin_back_minus_x_meshes0[9] = {
#include "enemy/leon/leon_shin_back_minus_x_meshes0.inc"
};

static Sint16 leon_shin_back_minus_x_meshes1[9] = {
#include "enemy/leon/leon_shin_back_minus_x_meshes1.inc"
};

static Sint16 leon_shin_back_minus_x_meshes2[17] = {
#include "enemy/leon/leon_shin_back_minus_x_meshes2.inc"
};

static NJS_COLOR leon_shin_back_minus_x_uvs0[8] = {
#include "enemy/leon/leon_shin_back_minus_x_uvs0.inc"
};

static NJS_COLOR leon_shin_back_minus_x_uvs1[8] = {
#include "enemy/leon/leon_shin_back_minus_x_uvs1.inc"
};

static NJS_COLOR leon_shin_back_minus_x_uvs2[14] = {
#include "enemy/leon/leon_shin_back_minus_x_uvs2.inc"
};

static NJS_MESHSET leon_shin_back_minus_x_meshsets[3] = {
#include "enemy/leon/leon_shin_back_minus_x_meshsets.inc"
};

static NJS_POINT3 leon_shin_back_minus_x_points[15] = {
#include "enemy/leon/leon_shin_back_minus_x_points.inc"
};

static NJS_VECTOR leon_shin_back_minus_x_normals[15] = {
#include "enemy/leon/leon_shin_back_minus_x_normals.inc"
};

static NJS_MODEL leon_shin_back_minus_x_model = {
#include "enemy/leon/leon_shin_back_minus_x_model.inc"
};

static NJS_OBJECT leon_shin_back_minus_x = {
#include "enemy/leon/leon_shin_back_minus_x.inc"
};

static NJS_MATERIAL leon_foot_back_minus_x_materials[2] = {
#include "enemy/leon/leon_foot_back_minus_x_materials.inc"
};

static Sint16 leon_foot_back_minus_x_meshes0[15] = {
#include "enemy/leon/leon_foot_back_minus_x_meshes0.inc"
};

static Sint16 leon_foot_back_minus_x_meshes1[24] = {
#include "enemy/leon/leon_foot_back_minus_x_meshes1.inc"
};

static NJS_COLOR leon_foot_back_minus_x_uvs0[14] = {
#include "enemy/leon/leon_foot_back_minus_x_uvs0.inc"
};

static NJS_COLOR leon_foot_back_minus_x_uvs1[20] = {
#include "enemy/leon/leon_foot_back_minus_x_uvs1.inc"
};

static NJS_MESHSET leon_foot_back_minus_x_meshsets[2] = {
#include "enemy/leon/leon_foot_back_minus_x_meshsets.inc"
};

static NJS_POINT3 leon_foot_back_minus_x_points[18] = {
#include "enemy/leon/leon_foot_back_minus_x_points.inc"
};

static NJS_VECTOR leon_foot_back_minus_x_normals[18] = {
#include "enemy/leon/leon_foot_back_minus_x_normals.inc"
};

static NJS_MODEL leon_foot_back_minus_x_model = {
#include "enemy/leon/leon_foot_back_minus_x_model.inc"
};

static NJS_OBJECT leon_foot_back_minus_x = {
#include "enemy/leon/leon_foot_back_minus_x.inc"
};

static NJS_OBJECT leon_foot_bone_back_minus_x = {
#include "enemy/leon/leon_foot_bone_back_minus_x.inc"
};

static NJS_OBJECT leon_shin_bone_back_minus_x = {
#include "enemy/leon/leon_shin_bone_back_minus_x.inc"
};

static NJS_OBJECT leon_thigh_bone_back_minus_x = {
#include "enemy/leon/leon_thigh_bone_back_minus_x.inc"
};

static NJS_OBJECT leon_hip_back_minus_x = {
#include "enemy/leon/leon_hip_back_minus_x.inc"
};

static NJS_MATERIAL leon_thigh_front_minus_x_materials[1] = {
#include "enemy/leon/leon_thigh_front_minus_x_materials.inc"
};

static Sint16 leon_thigh_front_minus_x_meshes0[9] = {
#include "enemy/leon/leon_thigh_front_minus_x_meshes0.inc"
};

static NJS_COLOR leon_thigh_front_minus_x_uvs0[8] = {
#include "enemy/leon/leon_thigh_front_minus_x_uvs0.inc"
};

static NJS_MESHSET leon_thigh_front_minus_x_meshsets[1] = {
#include "enemy/leon/leon_thigh_front_minus_x_meshsets.inc"
};

static NJS_POINT3 leon_thigh_front_minus_x_points[6] = {
#include "enemy/leon/leon_thigh_front_minus_x_points.inc"
};

static NJS_VECTOR leon_thigh_front_minus_x_normals[6] = {
#include "enemy/leon/leon_thigh_front_minus_x_normals.inc"
};

static NJS_MODEL leon_thigh_front_minus_x_model = {
#include "enemy/leon/leon_thigh_front_minus_x_model.inc"
};

static NJS_OBJECT leon_thigh_front_minus_x = {
#include "enemy/leon/leon_thigh_front_minus_x.inc"
};

static NJS_MATERIAL leon_shin_front_minus_x_materials[3] = {
#include "enemy/leon/leon_shin_front_minus_x_materials.inc"
};

static Sint16 leon_shin_front_minus_x_meshes0[17] = {
#include "enemy/leon/leon_shin_front_minus_x_meshes0.inc"
};

static Sint16 leon_shin_front_minus_x_meshes1[9] = {
#include "enemy/leon/leon_shin_front_minus_x_meshes1.inc"
};

static Sint16 leon_shin_front_minus_x_meshes2[9] = {
#include "enemy/leon/leon_shin_front_minus_x_meshes2.inc"
};

static NJS_COLOR leon_shin_front_minus_x_uvs0[14] = {
#include "enemy/leon/leon_shin_front_minus_x_uvs0.inc"
};

static NJS_COLOR leon_shin_front_minus_x_uvs1[8] = {
#include "enemy/leon/leon_shin_front_minus_x_uvs1.inc"
};

static NJS_COLOR leon_shin_front_minus_x_uvs2[8] = {
#include "enemy/leon/leon_shin_front_minus_x_uvs2.inc"
};

static NJS_MESHSET leon_shin_front_minus_x_meshsets[3] = {
#include "enemy/leon/leon_shin_front_minus_x_meshsets.inc"
};

static NJS_POINT3 leon_shin_front_minus_x_points[15] = {
#include "enemy/leon/leon_shin_front_minus_x_points.inc"
};

static NJS_VECTOR leon_shin_front_minus_x_normals[15] = {
#include "enemy/leon/leon_shin_front_minus_x_normals.inc"
};

static NJS_MODEL leon_shin_front_minus_x_model = {
#include "enemy/leon/leon_shin_front_minus_x_model.inc"
};

static NJS_OBJECT leon_shin_front_minus_x = {
#include "enemy/leon/leon_shin_front_minus_x.inc"
};

static NJS_MATERIAL leon_foot_front_minus_x_materials[2] = {
#include "enemy/leon/leon_foot_front_minus_x_materials.inc"
};

static Sint16 leon_foot_front_minus_x_meshes0[15] = {
#include "enemy/leon/leon_foot_front_minus_x_meshes0.inc"
};

static Sint16 leon_foot_front_minus_x_meshes1[24] = {
#include "enemy/leon/leon_foot_front_minus_x_meshes1.inc"
};

static NJS_COLOR leon_foot_front_minus_x_uvs0[14] = {
#include "enemy/leon/leon_foot_front_minus_x_uvs0.inc"
};

static NJS_COLOR leon_foot_front_minus_x_uvs1[20] = {
#include "enemy/leon/leon_foot_front_minus_x_uvs1.inc"
};

static NJS_MESHSET leon_foot_front_minus_x_meshsets[2] = {
#include "enemy/leon/leon_foot_front_minus_x_meshsets.inc"
};

static NJS_POINT3 leon_foot_front_minus_x_points[18] = {
#include "enemy/leon/leon_foot_front_minus_x_points.inc"
};

static NJS_VECTOR leon_foot_front_minus_x_normals[18] = {
#include "enemy/leon/leon_foot_front_minus_x_normals.inc"
};

static NJS_MODEL leon_foot_front_minus_x_model = {
#include "enemy/leon/leon_foot_front_minus_x_model.inc"
};

static NJS_OBJECT leon_foot_front_minus_x = {
#include "enemy/leon/leon_foot_front_minus_x.inc"
};

static NJS_OBJECT leon_foot_bone_front_minus_x = {
#include "enemy/leon/leon_foot_bone_front_minus_x.inc"
};

static NJS_OBJECT leon_shin_bone_front_minus_x = {
#include "enemy/leon/leon_shin_bone_front_minus_x.inc"
};

static NJS_OBJECT leon_thigh_bone_front_minus_x = {
#include "enemy/leon/leon_thigh_bone_front_minus_x.inc"
};

static NJS_OBJECT leon_hip_front_minus_x = {
#include "enemy/leon/leon_hip_front_minus_x.inc"
};

static NJS_MATERIAL leon_thigh_back_plus_x_materials[1] = {
#include "enemy/leon/leon_thigh_back_plus_x_materials.inc"
};

static Sint16 leon_thigh_back_plus_x_meshes0[9] = {
#include "enemy/leon/leon_thigh_back_plus_x_meshes0.inc"
};

static NJS_COLOR leon_thigh_back_plus_x_uvs0[8] = {
#include "enemy/leon/leon_thigh_back_plus_x_uvs0.inc"
};

static NJS_MESHSET leon_thigh_back_plus_x_meshsets[1] = {
#include "enemy/leon/leon_thigh_back_plus_x_meshsets.inc"
};

static NJS_POINT3 leon_thigh_back_plus_x_points[6] = {
#include "enemy/leon/leon_thigh_back_plus_x_points.inc"
};

static NJS_VECTOR leon_thigh_back_plus_x_normals[6] = {
#include "enemy/leon/leon_thigh_back_plus_x_normals.inc"
};

static NJS_MODEL leon_thigh_back_plus_x_model = {
#include "enemy/leon/leon_thigh_back_plus_x_model.inc"
};

static NJS_OBJECT leon_thigh_back_plus_x = {
#include "enemy/leon/leon_thigh_back_plus_x.inc"
};

static NJS_MATERIAL leon_shin_back_plus_x_materials[3] = {
#include "enemy/leon/leon_shin_back_plus_x_materials.inc"
};

static Sint16 leon_shin_back_plus_x_meshes0[9] = {
#include "enemy/leon/leon_shin_back_plus_x_meshes0.inc"
};

static Sint16 leon_shin_back_plus_x_meshes1[9] = {
#include "enemy/leon/leon_shin_back_plus_x_meshes1.inc"
};

static Sint16 leon_shin_back_plus_x_meshes2[17] = {
#include "enemy/leon/leon_shin_back_plus_x_meshes2.inc"
};

static NJS_COLOR leon_shin_back_plus_x_uvs0[8] = {
#include "enemy/leon/leon_shin_back_plus_x_uvs0.inc"
};

static NJS_COLOR leon_shin_back_plus_x_uvs1[8] = {
#include "enemy/leon/leon_shin_back_plus_x_uvs1.inc"
};

static NJS_COLOR leon_shin_back_plus_x_uvs2[14] = {
#include "enemy/leon/leon_shin_back_plus_x_uvs2.inc"
};

static NJS_MESHSET leon_shin_back_plus_x_meshsets[3] = {
#include "enemy/leon/leon_shin_back_plus_x_meshsets.inc"
};

static NJS_POINT3 leon_shin_back_plus_x_points[15] = {
#include "enemy/leon/leon_shin_back_plus_x_points.inc"
};

static NJS_VECTOR leon_shin_back_plus_x_normals[15] = {
#include "enemy/leon/leon_shin_back_plus_x_normals.inc"
};

static NJS_MODEL leon_shin_back_plus_x_model = {
#include "enemy/leon/leon_shin_back_plus_x_model.inc"
};

static NJS_OBJECT leon_shin_back_plus_x = {
#include "enemy/leon/leon_shin_back_plus_x.inc"
};

static NJS_MATERIAL leon_foot_back_plus_x_materials[2] = {
#include "enemy/leon/leon_foot_back_plus_x_materials.inc"
};

static Sint16 leon_foot_back_plus_x_meshes0[15] = {
#include "enemy/leon/leon_foot_back_plus_x_meshes0.inc"
};

static Sint16 leon_foot_back_plus_x_meshes1[24] = {
#include "enemy/leon/leon_foot_back_plus_x_meshes1.inc"
};

static NJS_COLOR leon_foot_back_plus_x_uvs0[14] = {
#include "enemy/leon/leon_foot_back_plus_x_uvs0.inc"
};

static NJS_COLOR leon_foot_back_plus_x_uvs1[20] = {
#include "enemy/leon/leon_foot_back_plus_x_uvs1.inc"
};

static NJS_MESHSET leon_foot_back_plus_x_meshsets[2] = {
#include "enemy/leon/leon_foot_back_plus_x_meshsets.inc"
};

static NJS_POINT3 leon_foot_back_plus_x_points[18] = {
#include "enemy/leon/leon_foot_back_plus_x_points.inc"
};

static NJS_VECTOR leon_foot_back_plus_x_normals[18] = {
#include "enemy/leon/leon_foot_back_plus_x_normals.inc"
};

static NJS_MODEL leon_foot_back_plus_x_model = {
#include "enemy/leon/leon_foot_back_plus_x_model.inc"
};

static NJS_OBJECT leon_foot_back_plus_x = {
#include "enemy/leon/leon_foot_back_plus_x.inc"
};

static NJS_OBJECT leon_foot_bone_back_plus_x = {
#include "enemy/leon/leon_foot_bone_back_plus_x.inc"
};

static NJS_OBJECT leon_shin_bone_back_plus_x = {
#include "enemy/leon/leon_shin_bone_back_plus_x.inc"
};

static NJS_OBJECT leon_thigh_bone_back_plus_x = {
#include "enemy/leon/leon_thigh_bone_back_plus_x.inc"
};

static NJS_OBJECT leon_hip_back_plus_x = {
#include "enemy/leon/leon_hip_back_plus_x.inc"
};

static NJS_MATERIAL leon_thigh_front_plus_x_materials[1] = {
#include "enemy/leon/leon_thigh_front_plus_x_materials.inc"
};

static Sint16 leon_thigh_front_plus_x_meshes0[9] = {
#include "enemy/leon/leon_thigh_front_plus_x_meshes0.inc"
};

static NJS_COLOR leon_thigh_front_plus_x_uvs0[8] = {
#include "enemy/leon/leon_thigh_front_plus_x_uvs0.inc"
};

static NJS_MESHSET leon_thigh_front_plus_x_meshsets[1] = {
#include "enemy/leon/leon_thigh_front_plus_x_meshsets.inc"
};

static NJS_POINT3 leon_thigh_front_plus_x_points[6] = {
#include "enemy/leon/leon_thigh_front_plus_x_points.inc"
};

static NJS_VECTOR leon_thigh_front_plus_x_normals[6] = {
#include "enemy/leon/leon_thigh_front_plus_x_normals.inc"
};

static NJS_MODEL leon_thigh_front_plus_x_model = {
#include "enemy/leon/leon_thigh_front_plus_x_model.inc"
};

static NJS_OBJECT leon_thigh_front_plus_x = {
#include "enemy/leon/leon_thigh_front_plus_x.inc"
};

static NJS_MATERIAL leon_shin_front_plus_x_materials[3] = {
#include "enemy/leon/leon_shin_front_plus_x_materials.inc"
};

static Sint16 leon_shin_front_plus_x_meshes0[17] = {
#include "enemy/leon/leon_shin_front_plus_x_meshes0.inc"
};

static Sint16 leon_shin_front_plus_x_meshes1[9] = {
#include "enemy/leon/leon_shin_front_plus_x_meshes1.inc"
};

static Sint16 leon_shin_front_plus_x_meshes2[9] = {
#include "enemy/leon/leon_shin_front_plus_x_meshes2.inc"
};

static NJS_COLOR leon_shin_front_plus_x_uvs0[14] = {
#include "enemy/leon/leon_shin_front_plus_x_uvs0.inc"
};

static NJS_COLOR leon_shin_front_plus_x_uvs1[8] = {
#include "enemy/leon/leon_shin_front_plus_x_uvs1.inc"
};

static NJS_COLOR leon_shin_front_plus_x_uvs2[8] = {
#include "enemy/leon/leon_shin_front_plus_x_uvs2.inc"
};

static NJS_MESHSET leon_shin_front_plus_x_meshsets[3] = {
#include "enemy/leon/leon_shin_front_plus_x_meshsets.inc"
};

static NJS_POINT3 leon_shin_front_plus_x_points[15] = {
#include "enemy/leon/leon_shin_front_plus_x_points.inc"
};

static NJS_VECTOR leon_shin_front_plus_x_normals[15] = {
#include "enemy/leon/leon_shin_front_plus_x_normals.inc"
};

static NJS_MODEL leon_shin_front_plus_x_model = {
#include "enemy/leon/leon_shin_front_plus_x_model.inc"
};

static NJS_OBJECT leon_shin_front_plus_x = {
#include "enemy/leon/leon_shin_front_plus_x.inc"
};

static NJS_MATERIAL leon_foot_front_plus_x_materials[2] = {
#include "enemy/leon/leon_foot_front_plus_x_materials.inc"
};

static Sint16 leon_foot_front_plus_x_meshes0[15] = {
#include "enemy/leon/leon_foot_front_plus_x_meshes0.inc"
};

static Sint16 leon_foot_front_plus_x_meshes1[24] = {
#include "enemy/leon/leon_foot_front_plus_x_meshes1.inc"
};

static NJS_COLOR leon_foot_front_plus_x_uvs0[14] = {
#include "enemy/leon/leon_foot_front_plus_x_uvs0.inc"
};

static NJS_COLOR leon_foot_front_plus_x_uvs1[20] = {
#include "enemy/leon/leon_foot_front_plus_x_uvs1.inc"
};

static NJS_MESHSET leon_foot_front_plus_x_meshsets[2] = {
#include "enemy/leon/leon_foot_front_plus_x_meshsets.inc"
};

static NJS_POINT3 leon_foot_front_plus_x_points[18] = {
#include "enemy/leon/leon_foot_front_plus_x_points.inc"
};

static NJS_VECTOR leon_foot_front_plus_x_normals[18] = {
#include "enemy/leon/leon_foot_front_plus_x_normals.inc"
};

static NJS_MODEL leon_foot_front_plus_x_model = {
#include "enemy/leon/leon_foot_front_plus_x_model.inc"
};

static NJS_OBJECT leon_foot_front_plus_x = {
#include "enemy/leon/leon_foot_front_plus_x.inc"
};

static NJS_OBJECT leon_foot_bone_front_plus_x = {
#include "enemy/leon/leon_foot_bone_front_plus_x.inc"
};

static NJS_OBJECT leon_shin_bone_front_plus_x = {
#include "enemy/leon/leon_shin_bone_front_plus_x.inc"
};

static NJS_OBJECT leon_thigh_bone_front_plus_x = {
#include "enemy/leon/leon_thigh_bone_front_plus_x.inc"
};

static NJS_OBJECT leon_hip_front_plus_x = {
#include "enemy/leon/leon_hip_front_plus_x.inc"
};

static NJS_MATERIAL leon_crest_materials[1] = {
#include "enemy/leon/leon_crest_materials.inc"
};

static Sint16 leon_crest_meshes0[24] = {
#include "enemy/leon/leon_crest_meshes0.inc"
};

static NJS_COLOR leon_crest_uvs0[20] = {
#include "enemy/leon/leon_crest_uvs0.inc"
};

static NJS_MESHSET leon_crest_meshsets[1] = {
#include "enemy/leon/leon_crest_meshsets.inc"
};

static NJS_POINT3 leon_crest_points[8] = {
#include "enemy/leon/leon_crest_points.inc"
};

static NJS_VECTOR leon_crest_normals[8] = {
#include "enemy/leon/leon_crest_normals.inc"
};

static NJS_MODEL leon_crest_model = {
#include "enemy/leon/leon_crest_model.inc"
};

static NJS_OBJECT leon_crest = {
#include "enemy/leon/leon_crest.inc"
};

static NJS_MATERIAL leon_horn_minus_x_materials[1] = {
#include "enemy/leon/leon_horn_minus_x_materials.inc"
};

static Sint16 leon_horn_minus_x_meshes0[10] = {
#include "enemy/leon/leon_horn_minus_x_meshes0.inc"
};

static NJS_COLOR leon_horn_minus_x_uvs0[8] = {
#include "enemy/leon/leon_horn_minus_x_uvs0.inc"
};

static NJS_MESHSET leon_horn_minus_x_meshsets[1] = {
#include "enemy/leon/leon_horn_minus_x_meshsets.inc"
};

static NJS_POINT3 leon_horn_minus_x_points[6] = {
#include "enemy/leon/leon_horn_minus_x_points.inc"
};

static NJS_VECTOR leon_horn_minus_x_normals[6] = {
#include "enemy/leon/leon_horn_minus_x_normals.inc"
};

static NJS_MODEL leon_horn_minus_x_model = {
#include "enemy/leon/leon_horn_minus_x_model.inc"
};

static NJS_OBJECT leon_horn_minus_x = {
#include "enemy/leon/leon_horn_minus_x.inc"
};

static NJS_MATERIAL leon_horn_plus_x_materials[1] = {
#include "enemy/leon/leon_horn_plus_x_materials.inc"
};

static Sint16 leon_horn_plus_x_meshes0[10] = {
#include "enemy/leon/leon_horn_plus_x_meshes0.inc"
};

static NJS_COLOR leon_horn_plus_x_uvs0[8] = {
#include "enemy/leon/leon_horn_plus_x_uvs0.inc"
};

static NJS_MESHSET leon_horn_plus_x_meshsets[1] = {
#include "enemy/leon/leon_horn_plus_x_meshsets.inc"
};

static NJS_POINT3 leon_horn_plus_x_points[6] = {
#include "enemy/leon/leon_horn_plus_x_points.inc"
};

static NJS_VECTOR leon_horn_plus_x_normals[6] = {
#include "enemy/leon/leon_horn_plus_x_normals.inc"
};

static NJS_MODEL leon_horn_plus_x_model = {
#include "enemy/leon/leon_horn_plus_x_model.inc"
};

static NJS_OBJECT leon_horn_plus_x = {
#include "enemy/leon/leon_horn_plus_x.inc"
};

static NJS_MATERIAL leon_body_materials[4] = {
#include "enemy/leon/leon_body_materials.inc"
};

static Sint16 leon_body_meshes0[12] = {
#include "enemy/leon/leon_body_meshes0.inc"
};

static Sint16 leon_body_meshes1[17] = {
#include "enemy/leon/leon_body_meshes1.inc"
};

static Sint16 leon_body_meshes2[12] = {
#include "enemy/leon/leon_body_meshes2.inc"
};

static Sint16 leon_body_meshes3[7] = {
#include "enemy/leon/leon_body_meshes3.inc"
};

static NJS_COLOR leon_body_uvs0[10] = {
#include "enemy/leon/leon_body_uvs0.inc"
};

static NJS_COLOR leon_body_uvs1[14] = {
#include "enemy/leon/leon_body_uvs1.inc"
};

static NJS_COLOR leon_body_uvs2[10] = {
#include "enemy/leon/leon_body_uvs2.inc"
};

static NJS_COLOR leon_body_uvs3[6] = {
#include "enemy/leon/leon_body_uvs3.inc"
};

static NJS_MESHSET leon_body_meshsets[4] = {
#include "enemy/leon/leon_body_meshsets.inc"
};

static NJS_POINT3 leon_body_points[14] = {
#include "enemy/leon/leon_body_points.inc"
};

static NJS_VECTOR leon_body_normals[14] = {
#include "enemy/leon/leon_body_normals.inc"
};

static NJS_MODEL leon_body_model = {
#include "enemy/leon/leon_body_model.inc"
};

static NJS_OBJECT leon_body = {
#include "enemy/leon/leon_body.inc"
};

static NJS_OBJECT leon_waist = {
#include "enemy/leon/leon_waist.inc"
};

static NJS_OBJECT leon_object = {
#include "enemy/leon/leon_object.inc"
};

static NJS_MATERIAL leon_head_horn_materials[1] = {
#include "enemy/leon/leon_head_horn_materials.inc"
};

static Sint16 leon_head_horn_meshes0[10] = {
#include "enemy/leon/leon_head_horn_meshes0.inc"
};

static NJS_COLOR leon_head_horn_uvs0[8] = {
#include "enemy/leon/leon_head_horn_uvs0.inc"
};

static NJS_MESHSET leon_head_horn_meshsets[1] = {
#include "enemy/leon/leon_head_horn_meshsets.inc"
};

static NJS_POINT3 leon_head_horn_points[8] = {
#include "enemy/leon/leon_head_horn_points.inc"
};

static NJS_VECTOR leon_head_horn_normals[8] = {
#include "enemy/leon/leon_head_horn_normals.inc"
};

static NJS_MODEL leon_head_horn_model = {
#include "enemy/leon/leon_head_horn_model.inc"
};

static NJS_OBJECT leon_head_horn = {
#include "enemy/leon/leon_head_horn.inc"
};

static NJS_MATERIAL leon_eye_minus_x_materials[3] = {
#include "enemy/leon/leon_eye_minus_x_materials.inc"
};

static Sint16 leon_eye_minus_x_meshes0[27] = {
#include "enemy/leon/leon_eye_minus_x_meshes0.inc"
};

static Sint16 leon_eye_minus_x_meshes1[27] = {
#include "enemy/leon/leon_eye_minus_x_meshes1.inc"
};

static Sint16 leon_eye_minus_x_meshes2[6] = {
#include "enemy/leon/leon_eye_minus_x_meshes2.inc"
};

static NJS_COLOR leon_eye_minus_x_uvs0[24] = {
#include "enemy/leon/leon_eye_minus_x_uvs0.inc"
};

static NJS_COLOR leon_eye_minus_x_uvs1[22] = {
#include "enemy/leon/leon_eye_minus_x_uvs1.inc"
};

static NJS_COLOR leon_eye_minus_x_uvs2[5] = {
#include "enemy/leon/leon_eye_minus_x_uvs2.inc"
};

static NJS_MESHSET leon_eye_minus_x_meshsets[3] = {
#include "enemy/leon/leon_eye_minus_x_meshsets.inc"
};

static NJS_POINT3 leon_eye_minus_x_points[21] = {
#include "enemy/leon/leon_eye_minus_x_points.inc"
};

static NJS_VECTOR leon_eye_minus_x_normals[21] = {
#include "enemy/leon/leon_eye_minus_x_normals.inc"
};

static NJS_MODEL leon_eye_minus_x_model = {
#include "enemy/leon/leon_eye_minus_x_model.inc"
};

static NJS_OBJECT leon_eye_minus_x = {
#include "enemy/leon/leon_eye_minus_x.inc"
};

static NJS_MATERIAL leon_jaw_materials[2] = {
#include "enemy/leon/leon_jaw_materials.inc"
};

static Sint16 leon_jaw_meshes0[21] = {
#include "enemy/leon/leon_jaw_meshes0.inc"
};

static Sint16 leon_jaw_meshes1[11] = {
#include "enemy/leon/leon_jaw_meshes1.inc"
};

static NJS_COLOR leon_jaw_uvs0[18] = {
#include "enemy/leon/leon_jaw_uvs0.inc"
};

static NJS_COLOR leon_jaw_uvs1[9] = {
#include "enemy/leon/leon_jaw_uvs1.inc"
};

static NJS_MESHSET leon_jaw_meshsets[2] = {
#include "enemy/leon/leon_jaw_meshsets.inc"
};

static NJS_POINT3 leon_jaw_points[12] = {
#include "enemy/leon/leon_jaw_points.inc"
};

static NJS_VECTOR leon_jaw_normals[12] = {
#include "enemy/leon/leon_jaw_normals.inc"
};

static NJS_MODEL leon_jaw_model = {
#include "enemy/leon/leon_jaw_model.inc"
};

static NJS_OBJECT leon_jaw = {
#include "enemy/leon/leon_jaw.inc"
};

static NJS_MATERIAL leon_throat_materials[1] = {
#include "enemy/leon/leon_throat_materials.inc"
};

static Sint16 leon_throat_meshes0[21] = {
#include "enemy/leon/leon_throat_meshes0.inc"
};

static NJS_COLOR leon_throat_uvs0[18] = {
#include "enemy/leon/leon_throat_uvs0.inc"
};

static NJS_MESHSET leon_throat_meshsets[1] = {
#include "enemy/leon/leon_throat_meshsets.inc"
};

static NJS_POINT3 leon_throat_points[11] = {
#include "enemy/leon/leon_throat_points.inc"
};

static NJS_VECTOR leon_throat_normals[11] = {
#include "enemy/leon/leon_throat_normals.inc"
};

static NJS_MODEL leon_throat_model = {
#include "enemy/leon/leon_throat_model.inc"
};

static NJS_OBJECT leon_throat = {
#include "enemy/leon/leon_throat.inc"
};

static NJS_MATERIAL leon_snout_horn_materials[1] = {
#include "enemy/leon/leon_snout_horn_materials.inc"
};

static Sint16 leon_snout_horn_meshes0[10] = {
#include "enemy/leon/leon_snout_horn_meshes0.inc"
};

static NJS_COLOR leon_snout_horn_uvs0[8] = {
#include "enemy/leon/leon_snout_horn_uvs0.inc"
};

static NJS_MESHSET leon_snout_horn_meshsets[1] = {
#include "enemy/leon/leon_snout_horn_meshsets.inc"
};

static NJS_POINT3 leon_snout_horn_points[5] = {
#include "enemy/leon/leon_snout_horn_points.inc"
};

static NJS_VECTOR leon_snout_horn_normals[5] = {
#include "enemy/leon/leon_snout_horn_normals.inc"
};

static NJS_MODEL leon_snout_horn_model = {
#include "enemy/leon/leon_snout_horn_model.inc"
};

static NJS_OBJECT leon_snout_horn = {
#include "enemy/leon/leon_snout_horn.inc"
};

static NJS_MATERIAL leon_eye_plus_x_materials[3] = {
#include "enemy/leon/leon_eye_plus_x_materials.inc"
};

static Sint16 leon_eye_plus_x_meshes0[27] = {
#include "enemy/leon/leon_eye_plus_x_meshes0.inc"
};

static Sint16 leon_eye_plus_x_meshes1[27] = {
#include "enemy/leon/leon_eye_plus_x_meshes1.inc"
};

static Sint16 leon_eye_plus_x_meshes2[6] = {
#include "enemy/leon/leon_eye_plus_x_meshes2.inc"
};

static NJS_COLOR leon_eye_plus_x_uvs0[24] = {
#include "enemy/leon/leon_eye_plus_x_uvs0.inc"
};

static NJS_COLOR leon_eye_plus_x_uvs1[22] = {
#include "enemy/leon/leon_eye_plus_x_uvs1.inc"
};

static NJS_COLOR leon_eye_plus_x_uvs2[5] = {
#include "enemy/leon/leon_eye_plus_x_uvs2.inc"
};

static NJS_MESHSET leon_eye_plus_x_meshsets[3] = {
#include "enemy/leon/leon_eye_plus_x_meshsets.inc"
};

static NJS_POINT3 leon_eye_plus_x_points[21] = {
#include "enemy/leon/leon_eye_plus_x_points.inc"
};

static NJS_VECTOR leon_eye_plus_x_normals[21] = {
#include "enemy/leon/leon_eye_plus_x_normals.inc"
};

static NJS_MODEL leon_eye_plus_x_model = {
#include "enemy/leon/leon_eye_plus_x_model.inc"
};

static NJS_OBJECT leon_eye_plus_x = {
#include "enemy/leon/leon_eye_plus_x.inc"
};

static NJS_MATERIAL leon_head_materials[3] = {
#include "enemy/leon/leon_head_materials.inc"
};

static Sint16 leon_head_meshes0[12] = {
#include "enemy/leon/leon_head_meshes0.inc"
};

static Sint16 leon_head_meshes1[27] = {
#include "enemy/leon/leon_head_meshes1.inc"
};

static Sint16 leon_head_meshes2[27] = {
#include "enemy/leon/leon_head_meshes2.inc"
};

static NJS_COLOR leon_head_uvs0[10] = {
#include "enemy/leon/leon_head_uvs0.inc"
};

static NJS_COLOR leon_head_uvs1[24] = {
#include "enemy/leon/leon_head_uvs1.inc"
};

static NJS_COLOR leon_head_uvs2[22] = {
#include "enemy/leon/leon_head_uvs2.inc"
};

static NJS_MESHSET leon_head_meshsets[3] = {
#include "enemy/leon/leon_head_meshsets.inc"
};

static NJS_POINT3 leon_head_points[21] = {
#include "enemy/leon/leon_head_points.inc"
};

static NJS_VECTOR leon_head_normals[21] = {
#include "enemy/leon/leon_head_normals.inc"
};

static NJS_MODEL leon_head_model = {
#include "enemy/leon/leon_head_model.inc"
};

/* the head stands apart from leon_object's tree. leon_jaw_action draws it */
static NJS_OBJECT leon_head = {
#include "enemy/leon/leon_head.inc"
};

static NJS_MATERIAL leon_tongue_materials[1] = {
#include "enemy/leon/leon_tongue_materials.inc"
};

static Sint16 leon_tongue_meshes0[14] = {
#include "enemy/leon/leon_tongue_meshes0.inc"
};

static NJS_MESHSET leon_tongue_meshsets[1] = {
#include "enemy/leon/leon_tongue_meshsets.inc"
};

static NJS_POINT3 leon_tongue_points[8] = {
#include "enemy/leon/leon_tongue_points.inc"
};

static NJS_VECTOR leon_tongue_normals[8] = {
#include "enemy/leon/leon_tongue_normals.inc"
};

static NJS_MODEL leon_tongue_model = {
#include "enemy/leon/leon_tongue_model.inc"
};

static NJS_OBJECT leon_tongue = {
#include "enemy/leon/leon_tongue.inc"
};

static NJS_MATERIAL leon_tongue_tip_materials[1] = {
#include "enemy/leon/leon_tongue_tip_materials.inc"
};

static Sint16 leon_tongue_tip_meshes0[27] = {
#include "enemy/leon/leon_tongue_tip_meshes0.inc"
};

static NJS_MESHSET leon_tongue_tip_meshsets[1] = {
#include "enemy/leon/leon_tongue_tip_meshsets.inc"
};

static NJS_POINT3 leon_tongue_tip_points[10] = {
#include "enemy/leon/leon_tongue_tip_points.inc"
};

static NJS_VECTOR leon_tongue_tip_normals[10] = {
#include "enemy/leon/leon_tongue_tip_normals.inc"
};

static NJS_MODEL leon_tongue_tip_model = {
#include "enemy/leon/leon_tongue_tip_model.inc"
};

static NJS_OBJECT leon_tongue_tip = {
#include "enemy/leon/leon_tongue_tip.inc"
};

static NJS_MATERIAL leon_tail_segment_materials[2] = {
#include "enemy/leon/leon_tail_segment_materials.inc"
};

static Sint16 leon_tail_segment_meshes0[12] = {
#include "enemy/leon/leon_tail_segment_meshes0.inc"
};

static Sint16 leon_tail_segment_meshes1[14] = {
#include "enemy/leon/leon_tail_segment_meshes1.inc"
};

static NJS_COLOR leon_tail_segment_uvs0[10] = {
#include "enemy/leon/leon_tail_segment_uvs0.inc"
};

static NJS_COLOR leon_tail_segment_uvs1[12] = {
#include "enemy/leon/leon_tail_segment_uvs1.inc"
};

static NJS_MESHSET leon_tail_segment_meshsets[2] = {
#include "enemy/leon/leon_tail_segment_meshsets.inc"
};

static NJS_POINT3 leon_tail_segment_points[9] = {
#include "enemy/leon/leon_tail_segment_points.inc"
};

static NJS_VECTOR leon_tail_segment_normals[9] = {
#include "enemy/leon/leon_tail_segment_normals.inc"
};

static NJS_MODEL leon_tail_segment_model = {
#include "enemy/leon/leon_tail_segment_model.inc"
};

static NJS_OBJECT leon_tail_segment = {
#include "enemy/leon/leon_tail_segment.inc"
};

/* the number that ends a key array's name is its node's, counted depth
 * first from 0. the leon_jaw_ keys count in leon_head's tree, and the rest
 * in leon_object's */
static NJS_MKEY_F leon_lick_pos1[19] = {
#include "enemy/leon/leon_lick_pos1.inc"
};

static NJS_MKEY_A leon_lick_ang7[19] = {
#include "enemy/leon/leon_lick_ang7.inc"
};

static NJS_MKEY_A leon_lick_ang8[19] = {
#include "enemy/leon/leon_lick_ang8.inc"
};

static NJS_MKEY_A leon_lick_ang10[19] = {
#include "enemy/leon/leon_lick_ang10.inc"
};

static NJS_MKEY_A leon_lick_ang14[19] = {
#include "enemy/leon/leon_lick_ang14.inc"
};

static NJS_MKEY_A leon_lick_ang15[19] = {
#include "enemy/leon/leon_lick_ang15.inc"
};

static NJS_MKEY_A leon_lick_ang17[19] = {
#include "enemy/leon/leon_lick_ang17.inc"
};

static NJS_MKEY_A leon_lick_ang21[19] = {
#include "enemy/leon/leon_lick_ang21.inc"
};

static NJS_MKEY_A leon_lick_ang22[19] = {
#include "enemy/leon/leon_lick_ang22.inc"
};

static NJS_MKEY_A leon_lick_ang24[19] = {
#include "enemy/leon/leon_lick_ang24.inc"
};

static NJS_MKEY_A leon_lick_ang28[19] = {
#include "enemy/leon/leon_lick_ang28.inc"
};

static NJS_MKEY_A leon_lick_ang29[19] = {
#include "enemy/leon/leon_lick_ang29.inc"
};

static NJS_MKEY_A leon_lick_ang31[19] = {
#include "enemy/leon/leon_lick_ang31.inc"
};

static NJS_MDATA2 leon_lick_mdata[36] = {
#include "enemy/leon/leon_lick_mdata.inc"
};

static NJS_MOTION leon_lick_motion = {
#include "enemy/leon/leon_lick_motion.inc"
};

static NJS_ACTION leon_lick_action = {
#include "enemy/leon/leon_lick_action.inc"
};

static NJS_MKEY_A leon_jaw_ang0[19] = {
#include "enemy/leon/leon_jaw_ang0.inc"
};

static NJS_MKEY_A leon_jaw_ang4[12] = {
#include "enemy/leon/leon_jaw_ang4.inc"
};

static NJS_MDATA2 leon_jaw_mdata[7] = {
#include "enemy/leon/leon_jaw_mdata.inc"
};

static NJS_MOTION leon_jaw_motion = {
#include "enemy/leon/leon_jaw_motion.inc"
};

/* @hack type: an array of one, which keeps njPushMatrix's address in R10
 * in leon_draw_body (0x8c10424a). as a plain object, it moves to R11, and
 * summed over leon_draw_body and leon_draw_blend, 644 of 1215 words
 * differ */
static NJS_ACTION leon_jaw_action[1] = {
#include "enemy/leon/leon_jaw_action.inc"
};

static NJS_MKEY_F leon_walk_pos1[60] = {
#include "enemy/leon/leon_walk_pos1.inc"
};

static NJS_MKEY_A leon_walk_ang7[60] = {
#include "enemy/leon/leon_walk_ang7.inc"
};

static NJS_MKEY_A leon_walk_ang8[59] = {
#include "enemy/leon/leon_walk_ang8.inc"
};

static NJS_MKEY_A leon_walk_ang10[60] = {
#include "enemy/leon/leon_walk_ang10.inc"
};

static NJS_MKEY_A leon_walk_ang14[60] = {
#include "enemy/leon/leon_walk_ang14.inc"
};

static NJS_MKEY_A leon_walk_ang15[60] = {
#include "enemy/leon/leon_walk_ang15.inc"
};

static NJS_MKEY_A leon_walk_ang17[60] = {
#include "enemy/leon/leon_walk_ang17.inc"
};

static NJS_MKEY_A leon_walk_ang21[60] = {
#include "enemy/leon/leon_walk_ang21.inc"
};

static NJS_MKEY_A leon_walk_ang22[60] = {
#include "enemy/leon/leon_walk_ang22.inc"
};

static NJS_MKEY_A leon_walk_ang24[60] = {
#include "enemy/leon/leon_walk_ang24.inc"
};

static NJS_MKEY_A leon_walk_ang28[60] = {
#include "enemy/leon/leon_walk_ang28.inc"
};

static NJS_MKEY_A leon_walk_ang29[60] = {
#include "enemy/leon/leon_walk_ang29.inc"
};

static NJS_MKEY_A leon_walk_ang31[60] = {
#include "enemy/leon/leon_walk_ang31.inc"
};

static NJS_MDATA2 leon_walk_mdata[36] = {
#include "enemy/leon/leon_walk_mdata.inc"
};

static NJS_MOTION leon_walk_motion = {
#include "enemy/leon/leon_walk_motion.inc"
};

/* @hack type: an array of one, whose address leon_start loads into R4
 * (0x8c103ab6). as a plain object, it loads into R2, and summed over the
 * four functions that name it, 119 of 1044 words differ */
static NJS_ACTION leon_walk_action[1] = {
#include "enemy/leon/leon_walk_action.inc"
};

static NJS_TEXNAME e_leon_textures[18] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
};

NJS_TEXLIST e_leon_texlist = {
#include "enemy/leon/e_leon_texlist.inc"
};

/* the body, a shape 25 behind it, and a capsule along the tongue. leon_step
 * sets the body's attack and guard as it registers the leon. leon_draw_body
 * turns the last two off while the tongue is in, and lays the capsule along
 * the tongue */
/* clang-format off */
static struct collision_shape leon_shapes[3] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_PLAYERS), {-4.0F, 0.8F, -0.0F}, 10.0F,
     8.5F, 0.0F, {0, 0, 0x3c00}, 0, NULL},
    {SHAPE_CAPSULE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_PLAYERS), {-25.0F, 7.0F, 0.0F}, 2.0F, 1.0F, 0.0F,
     {0, 0, 0xc000}, 0, NULL},
    {SHAPE_CAPSULE | SHAPE_IN_WORLD | SHAPE_TURN_YZX | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 0.0F, 0.0F}, 2.0F, 10.0F, 0.0F, {0, 0, 0xc000}, 0, NULL},
};
/* clang-format on */

void leon_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    struct angles slope;

    collision_init(task, leon_shapes, COLLISION_ENEMY, 1, 3);
    work->collision->radius = 60.0F;
    record = enemy_alloc_record(task, work);
    work->mode = LEON_WAIT;
    work->submode = LEON_PLAYING;
    work->tick = 0;
    work->timer = 50;
    record->flags |= ENEMY_KEEP_ON_LEDGE;
    record->motion.slip = 0.0F;
    record->turn_speed = 0xe0;
    record->angles[LEON_HEAD_PITCH] = 0;
    record->angles[LEON_HEAD_YAW] = 0;
    TONGUE_LENGTH(record) = 1.0F;
    TONGUE_SPEED(record) = 0.0F;
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    HEAD_FRAME(record) = 0.0F;
    record->action = leon_walk_action;
    record->next_action = leon_walk_action;
    TAIL_LIFT(record) = LIFT_IN;
    BLEND(record) = 0.0F;
    FADE(record) = -1.0F;
    SPOT(record) = rand() * (3.99F * RAND_SCALE);
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 8.0F;
    record->probe_offset.z = 0.0F;
    record->top = 6.0F;
    record->radius = 15.0F;
    record->bottom = -8.0F;
    record->wall_bounce = 1.0F;
    record->floor_bounce = 0.2F;
    record->friction = 1.0F;
    record->unk_c4 = 1.0F;
    PIECE(record) = work->ang.z;
    work->pos.y = shadow_draw_slope(work, 0.0F, &slope) + 0.5F;
    work->ang.x = slope.x;
    work->ang.z = slope.z;
    SEARCH_RANGE(record) = work->scl.y + 100.0F;
    MOVE_RANGE(record) = work->scl.z + 100.0F;
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 0.0F;
    enemy_set_home(work, record);
    task->destructor = leon_destroy;
    task->step = leon_step;
    work->flags |= LEON_SOLID | LEON_HIDDEN;
    work->flags &= ~(LEON_TONGUE_OUT | LEON_LICKING);
    knuckles_find_piece(PIECE(record), &work->pos);
}

static void leon_creep(struct task_work *work, struct lockon_target *record)
{
    Float ahead = 0.1F;
    Float across = 0.0F;

    record->motion.accel.x +=
        (ahead * njCos(work->ang.y)) + (across * njSin(work->ang.y));
    record->motion.accel.z +=
        (ahead * njSin(-work->ang.y)) + (across * njCos(work->ang.y));
}

/* start LEON_BURST when enemy_take_hit returns TRUE on a step the leon's
 * collision record touched another */
static void leon_take_hit(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *collision;

    enemy_ignore((void *)player_work);
    /* @hack redundant: a discarded read of hit_shape, which the image makes and
     * never uses (MOV.W @R3,R3 at 0x8c103cbc). without it, 33 of 35 words
     * differ */
    (void)work->collision->hit_shape;
    collision = work->collision;
    enemy_ignore(collision->hit);
    if (!(collision->flags & COLLISION_HIT)) {
        return;
    }
    if (!enemy_take_hit(work, record)) {
        return;
    }
    work->mode = LEON_BURST;
    ENEMY_MARK_BEATEN(work);
    work->scl.z = 0.35F;
    work->flags &= ~LEON_SOLID;
}

/* spring scl.y back toward 1 at speed scl.z. each call damps scl.z by 0.2
 * of itself and pulls it by 0.32 of 1 - scl.y. scl.x becomes 2 - scl.y, and
 * neither is bounded */
static void leon_spring_scale(struct task_work *work)
{
    Float stretch = work->scl.y - 1.0F;

    work->scl.z += -(work->scl.z * 0.2F) - (stretch * 0.32F);
    work->scl.y += work->scl.z;
    work->scl.x = 2.0F - work->scl.y;
}

static void leon_walk(struct task_work *work, struct lockon_target *record)
{
    if (enemy_target_distance2(work, record) > 40.0F * 40.0F) {
        record->motion.speed.x = njCos(work->ang.y) * 0.5F;
        record->motion.speed.y += -0.18F;
        record->motion.speed.z = njSin(-work->ang.y) * 0.5F;
        PACE(record) = 2.5F;
    } else {
        record->motion.speed.x = njCos(work->ang.y) * 0.4F;
        record->motion.speed.y += -0.18F;
        record->motion.speed.z = njSin(-work->ang.y) * 0.4F;
        PACE(record) = 1.1F;
    }
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
}

/* shoot the tongue out as the action reaches TONGUE_FRAME. a pull that
 * grows with its length brings it back, and its return counts a lick */
static void leon_shoot_tongue(struct task_work *work,
                              struct lockon_target *record)
{
    if (!(work->flags & LEON_TONGUE_OUT) &&
        record->frame > TONGUE_FRAME - 0.5F && record->frame < TONGUE_FRAME) {
        TONGUE_SPEED(record) = 4.0F;
        work->flags |= LEON_TONGUE_OUT;
    }
    TONGUE_SPEED(record) -= TONGUE_LENGTH(record) * 0.0055F;
    TONGUE_LENGTH(record) += TONGUE_SPEED(record);
    if ((work->flags & LEON_TONGUE_OUT) && TONGUE_LENGTH(record) < 1.0F) {
        TONGUE_LENGTH(record) = 1.0F;
        TONGUE_SPEED(record) = 0.0F;
        work->flags &= ~LEON_TONGUE_OUT;
        LICKS(work)++;
    }
}

/* advance the action's frame, and phase, which sways the tail. while the
 * leon stalks, the frame advances by PACE a step. while it licks, the
 * action and the head's play together, and hold at TONGUE_FRAME while the
 * tongue is out. a lick ends as the head's action wraps */
static void leon_animate(struct task_work *work, struct lockon_target *record)
{
    NJS_ACTION *action = record->action;
    Float frames = (Float)action->motion->nbFrame;

    switch (work->mode) {
    case LEON_STALK:
        record->frame += PACE(record);
        HEAD_FRAME(record) = 0.0F;
        break;
    case LEON_LICK:
        if (!(work->flags & LEON_LICKING) && TASK_TIMER(work) > 20) {
            work->flags |= LEON_LICKING;
        }
        if ((work->flags & LEON_LICKING) &&
            (record->frame < TONGUE_FRAME ||
             !(work->flags & LEON_TONGUE_OUT))) {
            record->frame += 0.3F;
            HEAD_FRAME(record) += 0.3F;
        }
        if (HEAD_FRAME(record) >= frames) {
            HEAD_FRAME(record) -= frames;
            work->timer = 0;
            work->flags &= ~LEON_LICKING;
        }
        leon_shoot_tongue(work, record);
        break;
    }
    if (record->frame >= frames) {
        record->frame -= frames;
    }
    record->phase += 0x880;
}

/* turn the head toward the target, unless a lick plays */
static void leon_aim_head(struct task_work *work, struct lockon_target *record)
{
    NJS_VECTOR to;
    int ang;

    if (!(work->flags & LEON_LICKING)) {
        to.x = record->target.x - work->pos.x;
        to.y = record->target.y - work->pos.y;
        to.z = record->target.z - work->pos.z;
        /* ang is the turn from the target's pitch to the body's, and below
         * from aim_y to the body's heading. the head takes each negated.
         * adding 0x8000 as a Uint16 puts turns in signed order, so the
         * clamps keep the head within its _MIN and _MAX. adding it again
         * gives back the turn's low 16 bits */
        ang = angle_difference(
            (Angle)(atan2f(njSqrt((to.x * to.x) + (to.z * to.z)), to.y) *
                    ANGLE_PER_RADIAN) -
                0x4000,
            work->ang.z);
        ang = (Uint16)(ang + 0x8000);
        if (ang < 0x8000 - HEAD_PITCH_MAX) {
            ang = 0x8000 - HEAD_PITCH_MAX;
        } else if (ang > 0x8000 - HEAD_PITCH_MIN) {
            ang = 0x8000 - HEAD_PITCH_MIN;
        }
        ang = (Uint16)(ang + 0x8000);
        record->angles[LEON_HEAD_PITCH] = angle_approach(
            record->angles[LEON_HEAD_PITCH], -ang, HEAD_PITCH_SPEED);
        enemy_aim_at_target(work, record);
        ang = angle_difference(record->aim_y, work->ang.y);
        ang = (Uint16)(ang + 0x8000);
        if (ang < 0x8000 - HEAD_YAW_MAX) {
            ang = 0x8000 - HEAD_YAW_MAX;
        } else if (ang > 0x8000 - HEAD_YAW_MIN) {
            ang = 0x8000 - HEAD_YAW_MIN;
        }
        ang = (Uint16)(ang + 0x8000);
        record->angles[LEON_HEAD_YAW] =
            angle_approach(record->angles[LEON_HEAD_YAW], -ang, HEAD_YAW_SPEED);
    }
}

/* draw the leon on a step draw_in_view accepts. the head follows the
 * waist's place in the action, and shape 2 lies along the tongue. the
 * scale's spring, the tail and shapes 1 and 2 change only on a step that
 * draws */
static void leon_draw_body(struct task_work *work, struct lockon_target *record)
{
    NJS_ACTION *action = record->action;
    Angle ang;
    Float along;
    Float frame;
    NJS_MATRIX matrix;
    NJS_MATRIX jaw;
    NJS_POINT3 current;
    NJS_POINT3 next;
    Angle pitch;
    Angle yaw;
    Angle *head;
    Float size;
    int i;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    leon_spring_scale(work);
    njSetTexture(&e_leon_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_set_material(FADE(record), 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njRotateY(NULL, Z_TURN);
    njScale(NULL, work->scl.x * 1.5F, work->scl.y * 1.5F, work->scl.x * 1.5F);
    njTranslate(NULL, 0.0F, 0.0F, 3.0F);
    draw_action(action, record->frame);
    {
        NJS_POINT3 offset = {0.0F, 0.0F, 0.0F};

        pose_action(&_unit_matrix_, record->action, record->frame, 1, &matrix);
        njCalcPoint(&matrix, &offset, &current);
        frame = record->frame + 1.0F;
        if (frame >= (Float)record->action->motion->nbFrame) {
            frame -= (Float)record->action->motion->nbFrame;
        }
        pose_action(&_unit_matrix_, record->action, frame, 1, &matrix);
        njCalcPoint(&matrix, &offset, &next);
        along = njFraction(record->frame);
        njTranslate(NULL, (current.x * (1.0F - along)) + (next.x * along),
                    (current.y * (1.0F - along)) + (next.y * along),
                    (current.z * (1.0F - along)) + (next.z * along));
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 1.9F, 0.4F);
        TURN(njRotateY, record->angles[LEON_HEAD_YAW]);
        TURN(njRotateX, record->angles[LEON_HEAD_PITCH]);
        njTranslate(NULL, 0.0F, -4.8F, 0.8F);
        draw_action(leon_jaw_action, HEAD_FRAME(record));
        if (work->flags & LEON_TONGUE_OUT) {
            pose_action(&_unit_matrix_, leon_jaw_action, HEAD_FRAME(record), 0,
                        &jaw);
            njMultiMatrix(NULL, &jaw);
            njTranslate(NULL, 0.0F, -1.6F, 1.6F);
            njRotateX(NULL, 0x400);
            njPushMatrix(NULL);
            njTranslate(NULL, 0.0F, 0.0F, TONGUE_LENGTH(record) * 0.5F);
            njScale(NULL, 1.0F, 1.0F, TONGUE_LENGTH(record));
            draw_model(&leon_tongue_model);
            njPopMatrix(1);
            njPushMatrix(NULL);
            njTranslate(NULL, 0.0F, 0.0F,
                        (TONGUE_LENGTH(record) * 1.095F) + 1.0F);
            draw_model(&leon_tongue_tip_model);
            njPopMatrix(1);
        }
        njPopMatrix(1);
        /* @hack spelling: offset.x and offset.y in one chained store, which
         * leaves njTranslate's 0.4F to load straight into FR6 (0x8c104426).
         * as two stores, 0.4F takes FR12, njPushMatrix's address moves from
         * R10 to R11, and 386 of 698 words differ */
        offset.x = offset.y = 0.0F;
        offset.z = TONGUE_LENGTH(record) * 0.8F;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njRotateY(NULL, Z_TURN);
        njTranslate(NULL, (current.x * (1.0F - along)) + (next.x * along),
                    (current.y * (1.0F - along)) + (next.y * along),
                    (current.z * (1.0F - along)) + (next.z * along));
        njTranslate(NULL, 0.0F, 1.9F, 0.4F);
        TURN(njRotateY, record->angles[LEON_HEAD_YAW]);
        TURN(njRotateX, record->angles[LEON_HEAD_PITCH]);
        njTranslate(NULL, 0.0F, 1.8F, 6.2F);
        njCalcPoint(NULL, &offset, &work->collision->shapes[2].center);
        njPopMatrix(1);
    }
    work->collision->shapes[2].ang.x =
        record->angles[LEON_HEAD_PITCH] + work->ang.x + 0x4000;
    work->collision->shapes[2].ang.y =
        record->angles[LEON_HEAD_YAW] + work->ang.y + 0x4000;
    work->collision->shapes[2].ang.z = work->ang.z;
    work->collision->shapes[2].size_y = TONGUE_LENGTH(record) * 0.85F;
    /* the tail turns against the head. its root's pitch is TAIL_LIFT less a
     * quarter of the head's pitch. each segment turns from the one before
     * by an eighth of the head's yaw, negated. each share shifts the head's
     * angle plus 0x8000, as a Uint16, so that a negative angle shifts in
     * order. 0xe000 and 0xf000 carry the shifted 0x8000 out of the low 16
     * bits */
    /* @hack spelling: the head's angles read through head, which keeps their
     * address in R5 and 0x8000 in R6 (0x8c104684, 0x8c104688). through
     * record, the two trade and 7 of 698 words differ */
    head = record->angles;
    /* @hack spelling: ang worked out in three statements before pitch takes
     * it, which leave a 180-byte frame (0x8c1041f8). as one expression, as
     * leon_draw_blend writes it, the frame shrinks by 4 bytes and 516 of 698
     * words differ */
    ang = (Uint16)(head[LEON_HEAD_PITCH] + 0x8000) >> 2;
    ang = ang + 0xe000;
    ang = -(Uint16)ang;
    pitch = TAIL_LIFT(record) + ang;
    yaw = (Uint16)(((Uint16)(head[LEON_HEAD_YAW] + 0x8000) >> 3) + 0xf000);
    size = 2.0F;
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 2.2F, -5.5F);
    njPushMatrix(NULL);
    njScale(NULL, size, size, 1.0F);
    draw_model(&leon_tail_segment_model);
    njPopMatrix(1);
    if (work->flags & LEON_TONGUE_OUT) {
        CURL(record) -= 0.8F;
        if (CURL(record) < CURL_OUT) {
            CURL(record) = CURL_OUT;
        }
        TAIL_LIFT(record) -= 0x200;
        if (TAIL_LIFT(record) < LIFT_OUT) {
            TAIL_LIFT(record) = LIFT_OUT;
        }
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->collision->shapes[2].flags &= ~SHAPE_OFF;
    } else {
        CURL(record) += 0.4F;
        if (CURL(record) > CURL_IN) {
            CURL(record) = CURL_IN;
        }
        TAIL_LIFT(record) += 0x80;
        if (TAIL_LIFT(record) > LIFT_IN) {
            TAIL_LIFT(record) = LIFT_IN;
        }
        work->collision->shapes[1].flags |= SHAPE_OFF;
        work->collision->shapes[2].flags |= SHAPE_OFF;
    }
    yaw = -yaw;
    for (i = 0; i < 9; i++) {
        pitch += (Angle)((CURL(record) + (njSin(record->phase) * 0.6F)) *
                         ANGLE_PER_DEGREE);
        size -= 0.13F;
        njTranslate(NULL, 0.0F, 0.0F, -2.7F);
        TURN(njRotateY, yaw);
        TURN(njRotateX, pitch);
        njPushMatrix(NULL);
        njScale(NULL, size, size, 1.0F);
        draw_model(&leon_tail_segment_model);
        njPopMatrix(1);
    }
    njPopMatrix(1);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* draw the leon while action blends into next_action. the blend advances
 * only on a step that draws. once it passes 1, next_action plays from frame
 * 0. unlike leon_draw_body, this leaves shapes 1 and 2 as they are */
static void leon_draw_blend(struct task_work *work,
                            struct lockon_target *record)
{
    Angle ang;
    NJS_MATRIX matrix;
    NJS_MATRIX jaw;
    NJS_POINT3 current;
    NJS_POINT3 next;
    Float blend;
    Float rest;
    Angle pitch;
    Angle yaw;
    Float size;
    int i;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    leon_spring_scale(work);
    njSetTexture(&e_leon_texlist);
    draw_set_material(FADE(record), 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njRotateY(NULL, Z_TURN);
    njScale(NULL, work->scl.x * 1.5F, work->scl.y * 1.5F, work->scl.x * 1.5F);
    njTranslate(NULL, 0.0F, 0.0F, 3.0F);
    {
        NJS_MOTION_LINK link;
        NJS_ACTION_LINK action;

        action.motionlink = &link;
        action.object = record->action->object;
        link.motion[0] = record->action->motion;
        link.motion[1] = record->next_action->motion;
        link.frame[0] = record->frame;
        link.frame[1] = 0.0F;
        draw_action_link(&action, BLEND(record));
    }
    BLEND(record) += 0.07F;
    if (BLEND(record) > 1.0F) {
        BLEND(record) = 0.0F;
        record->frame = 0.0F;
        work->submode = LEON_PLAYING;
        record->action = record->next_action;
    }
    {
        NJS_POINT3 offset = {0.0F, 0.0F, 0.0F};

        pose_action(&_unit_matrix_, record->action, record->frame, 1, &matrix);
        njCalcPoint(&matrix, &offset, &current);
        pose_action(&_unit_matrix_, record->next_action, 0.0F, 1, &matrix);
        njCalcPoint(&matrix, &offset, &next);
    }
    blend = BLEND(record);
    rest = 1.0F - blend;
    njTranslate(NULL, (current.x * rest) + (next.x * blend),
                (current.y * rest) + (next.y * blend),
                (current.z * rest) + (next.z * blend));
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 1.9F, 0.4F);
    TURN(njRotateX, record->angles[LEON_HEAD_PITCH]);
    TURN(njRotateY, record->angles[LEON_HEAD_YAW]);
    njTranslate(NULL, 0.0F, -4.8F, 0.8F);
    draw_action(leon_jaw_action, HEAD_FRAME(record));
    if (work->flags & LEON_TONGUE_OUT) {
        pose_action(&_unit_matrix_, leon_jaw_action, HEAD_FRAME(record), 0,
                    &jaw);
        njMultiMatrix(NULL, &jaw);
        njTranslate(NULL, 0.0F, -1.6F, 1.6F);
        njRotateX(NULL, 0x400);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, TONGUE_LENGTH(record) * 0.5F);
        njScale(NULL, 1.0F, 1.0F, TONGUE_LENGTH(record));
        draw_model(&leon_tongue_model);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, (TONGUE_LENGTH(record) * 1.1F) + 1.0F);
        draw_model(&leon_tongue_tip_model);
        njPopMatrix(1);
    }
    njPopMatrix(1);
    /* the tail's angles, as leon_draw_body works them out */
    ang = -(Uint16)(((Uint16)(record->angles[LEON_HEAD_PITCH] + 0x8000) >> 2) +
                    0xe000);
    pitch = TAIL_LIFT(record) + ang;
    yaw = (Uint16)(((Uint16)(record->angles[LEON_HEAD_YAW] + 0x8000) >> 3) +
                   0xf000);
    size = 2.0F;
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 2.2F, -5.5F);
    njSetTexture(&e_leon_texlist);
    njPushMatrix(NULL);
    /* @hack spelling: the first segment's size written out, which loads
     * 2.0F into FR12 for it and for size (0x8c104ca8). the loop then keeps
     * 0.13F in FR12. as size, 145 of 517 words differ */
    njScale(NULL, 2.0F, 2.0F, 1.0F);
    draw_model(&leon_tail_segment_model);
    njPopMatrix(1);
    if (work->flags & LEON_TONGUE_OUT) {
        CURL(record) -= 0.8F;
        if (CURL(record) < CURL_OUT) {
            CURL(record) = CURL_OUT;
        }
        TAIL_LIFT(record) -= 0x200;
        if (TAIL_LIFT(record) < LIFT_OUT) {
            TAIL_LIFT(record) = LIFT_OUT;
        }
    } else {
        CURL(record) += 0.25F;
        if (CURL(record) > CURL_IN) {
            CURL(record) = CURL_IN;
        }
        TAIL_LIFT(record) += 0x20;
        if (TAIL_LIFT(record) > LIFT_IN) {
            TAIL_LIFT(record) = LIFT_IN;
        }
    }
    yaw = -yaw;
    for (i = 0; i < 9; i++) {
        pitch += (Angle)((CURL(record) + (njSin(record->phase) * 0.6F)) *
                         ANGLE_PER_DEGREE);
        size -= 0.13F;
        njTranslate(NULL, 0.0F, 0.0F, -2.7F);
        /* @hack spelling: yaw copied into ang for the test and the turn,
         * which keeps yaw in the frame and the loop's count in R13 (MOV
         * #9,R13 at 0x8c104d92). with yaw itself, 56 of 517 words differ */
        /* @hack redundant: an empty else, which puts the addresses of
         * njRotateX, draw_model and draw_set_material in R2, not R3
         * (0x8c104dda, 0x8c104df0, 0x8c104e08). without it, 6 of 517 words
         * differ */
        if ((ang = yaw) != 0) {
            njRotateY(NULL, ang);
        } else {
        }
        TURN(njRotateX, pitch);
        njPushMatrix(NULL);
        njScale(NULL, size, size, 1.0F);
        draw_model(&leon_tail_segment_model);
        njPopMatrix(1);
    }
    njPopMatrix(1);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* every call in leon.c expands leon_collide. SHC still keeps its body, at
 * 0x8c104e32, because it is external */
void leon_collide(struct task_work *work, struct lockon_target *record)
{
    enemy_collide(work, record);
    /* @hack redundant: a return where the function ends, which puts the address
     * of the call after each expansion in R3, not R2, as leon_draw_body's in
     * leon_wait (0x8c10561e). without it, summed over leon_collide and the
     * four steps that expand it, 8 of 288 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* fade the leon out while LEON_HIDDEN is set, and in while it is clear */
static void leon_move_fade(struct task_work *work, struct lockon_target *record)
{
    if (work->flags & LEON_HIDDEN) {
        FADE(record) -= 0.018F;
        if (FADE(record) < -1.0F) {
            FADE(record) = -1.0F;
        }
        /* @hack spelling: a return where an else would do, which gives the arm
         * an RTS of its own, with the clamp's store in its delay slot
         * (0x8c104ea2). as an else, 1 of 33 words differs */
        return;
    }
    FADE(record) += 0.012F;
    if (FADE(record) > 0.0F) {
        FADE(record) = 0.0F;
    }
}

/* @hack spelling: leon_move_fade through an inline function, which puts
 * enemy_save_pos's address in R2 and knuckles_find_piece's in R3 in
 * leon_step (0x8c105ae6, 0x8c105aee). called directly, 4 of 147 words
 * differ */
static void leon_fade(struct task_work *work, struct lockon_target *record)
{
    leon_move_fade(work, record);
    /* @hack redundant: a return where the function ends, which also puts
     * enemy_save_pos's address in R2 in leon_step (0x8c105ae6). without it,
     * 4 of 147 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void leon_edit_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 edge;
    NJS_POINT3 from;
    NJS_POINT3 to;
    int i;

    if (placement_editing(task)) {
        if (work->tick > 25) {
            njPrintC(NJM_LOCATION(8, 8), "< LEON >");
            njPrintC(NJM_LOCATION(17, 16), "->             ");
            njPrintC(NJM_LOCATION(17, 17), "-> ROT Y       ");
            njPrintC(NJM_LOCATION(17, 18), "-> EMERALD     ");
            njPrintC(NJM_LOCATION(17, 19), "->             ");
            njPrintC(NJM_LOCATION(17, 20), "-> SEARCH RANGE");
            njPrintC(NJM_LOCATION(17, 21), "-> MOVE RANGE ");
            edge.x = work->scl.y + 100.0F;
            edge.y = 3.0F;
            edge.z = 0.0F;
            for (i = 0; i < ENEMY_RANGE_SEGMENTS; i++) {
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN(njRotateY, i * ENEMY_RANGE_TURN);
                njCalcPoint(NULL, &edge, &from);
                njPopMatrix(1);
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN(njRotateY, (i + 1) * ENEMY_RANGE_TURN);
                njCalcPoint(NULL, &edge, &to);
                njPopMatrix(1);
                draw_line_v(&from, &to);
            }
        } else {
            njPrintC(NJM_LOCATION(8, 8), "<      >");
            njPrintC(NJM_LOCATION(17, 16), "->             ");
            njPrintC(NJM_LOCATION(17, 17), "-> ROT Y       ");
            njPrintC(NJM_LOCATION(17, 18), "-> EMERALD     ");
            njPrintC(NJM_LOCATION(17, 19), "->             ");
            njPrintC(NJM_LOCATION(17, 20), "->             ");
            njPrintC(NJM_LOCATION(17, 21), "-> MOVE RANGE ");
        }
        if (work->tick++ > 50) {
            work->tick = 0;
        }
    }
    edge.x = work->scl.z + 100.0F;
    edge.y = 0.0F;
    edge.z = 0.0F;
    for (i = 0; i < ENEMY_RANGE_SEGMENTS; i++) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, i * ENEMY_RANGE_TURN);
        njCalcPoint(NULL, &edge, &from);
        njPopMatrix(1);
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, (i + 1) * ENEMY_RANGE_TURN);
        njCalcPoint(NULL, &edge, &to);
        njPopMatrix(1);
        draw_line_v(&from, &to);
    }
    njSetTexture(&e_leon_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + Z_TURN);
    draw_action(leon_walk_action, 1.0F);
    njPopMatrix(1);
}

static void leon_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void leon_burst(struct task_work *work, struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    animal_create_random(15, work->pos.x, work->pos.y + 15.0F, work->pos.z);
}

static void leon_wait(struct task_work *work, struct lockon_target *record)
{
    leon_animate(work, record);
    leon_walk(work, record);
    leon_collide(work, record);
    leon_draw_body(work, record);
    if (enemy_player_distance2(work, 0) < STALK_RANGE * STALK_RANGE) {
        ENEMY_SET_MODE(work, record, LEON_STALK);
    }
}

static void leon_pick_target(struct task_work *work,
                             struct lockon_target *record)
{
    if (enemy_on_screen(&work->pos)) {
        work->flags |= LEON_ON_SCREEN;
    } else {
        work->flags &= ~(LEON_FOUND | LEON_ON_SCREEN);
    }
    if (work->flags & LEON_FOUND) {
        record->target = player_work[0]->pos;
    } else {
        enemy_target_near_player(work, record, 0, SPOT(record));
    }
    if (njDistanceP2P(&record->home, &work->pos) >
        MOVE_RANGE(record) + 100.0F) {
        record->target = record->home;
    }
}

/* @hack spelling: enemy_turn_to_target through an inline function, which
 * puts leon_aim_head's address in R2, not R3, in leon_stalk (0x8c105738).
 * the calls after it alternate R3 and R2 to match. called directly, 8 of
 * 130 words differ */
static void leon_face_target(struct task_work *work,
                             struct lockon_target *record)
{
    enemy_turn_to_target(work, record);
    /* @hack redundant: a return where the function ends, which also puts
     * leon_aim_head's address in R2 in leon_stalk (0x8c105738). without it,
     * 8 of 130 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void leon_stalk(struct task_work *work, struct lockon_target *record)
{
    leon_pick_target(work, record);
    leon_face_target(work, record);
    leon_aim_head(work, record);
    leon_creep(work, record);
    leon_walk(work, record);
    leon_collide(work, record);
    switch (work->submode) {
    case LEON_BLENDING:
        leon_animate(work, record);
        leon_draw_blend(work, record);
        break;
    case LEON_PLAYING:
        leon_animate(work, record);
        if (FADE(record) > -0.99F) {
            leon_draw_body(work, record);
        }
        break;
    }
    if (TASK_TIMER(work) > 60 &&
        enemy_target_distance2(work, record) <
            SEARCH_RANGE(record) * SEARCH_RANGE(record) &&
        (work->flags & LEON_ON_SCREEN) && (work->flags & LEON_HIDDEN)) {
        work->flags |= LEON_FOUND;
        work->flags &= ~LEON_HIDDEN;
        enemy_aim_at_player(work, record, 0);
        work->ang.y = record->aim_y;
    }
    if (!(work->flags & LEON_HIDDEN) && FADE(record) > -0.1F) {
        ENEMY_SET_MODE(work, record, LEON_LICK);
        LICKS(work) = 0;
        record->action = leon_walk_action;
        record->next_action = &leon_lick_action;
        work->submode = LEON_BLENDING;
        work->flags &= ~LEON_LICKING;
    }
}

static void leon_lick(struct task_work *work, struct lockon_target *record)
{
    record->target = player_work[0]->pos;
    leon_aim_head(work, record);
    leon_collide(work, record);
    switch (work->submode) {
    case LEON_BLENDING:
        leon_draw_blend(work, record);
        break;
    case LEON_PLAYING:
        leon_animate(work, record);
        leon_draw_body(work, record);
        if (LICKS(work) > 2 || !(work->flags & LEON_LICKING)) {
            ENEMY_SET_MODE(work, record, LEON_STALK);
            LICKS(work) = 0;
            record->next_action = leon_walk_action;
            work->submode = LEON_BLENDING;
            work->flags &= ~LEON_LICKING;
            work->flags |= LEON_HIDDEN;
        }
        break;
    }
}

static void leon_hit(struct task_work *work, struct lockon_target *record)
{
    leon_take_hit(work, record);
    leon_walk(work, record);
    leon_collide(work, record);
    leon_animate(work, record);
    leon_draw_body(work, record);
    if (VARS(work)->since_hit++ > 10) {
        work->mode = LEON_BURST;
    }
}

/* each step, knuckles_find_piece notes the leon's place as where its piece
 * of the master emerald lies. the piece goes to Knuckles as the leon
 * bursts. while the leon is wholly seen, its body can hurt players and be
 * hurt. otherwise the body's attack is 0 and its guard 3, the highest, and
 * it can do neither */
static void leon_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        leon_edit_draw(task);
        return;
    }
    /* @hack redundant: a discarded read of player 0's mode, which SHC drops
     * but which keeps the image's load of player_work[0] (MOV.L @R4,R4 at
     * 0x8c1059fc). without it, 82 of 147 words differ */
    (void)player->mode;
    if (placement_try_leave_by(task, LEON_LEAVE_RANGE)) {
        return;
    }
    leon_take_hit(work, record);
    switch (work->mode) {
    case LEON_WAIT:
        leon_wait(work, record);
        break;
    case LEON_STALK:
        leon_stalk(work, record);
        break;
    case LEON_LICK:
        leon_lick(work, record);
        break;
    case LEON_HIT:
        leon_hit(work, record);
        break;
    case LEON_BURST:
        knuckles_take_piece(PIECE(record), &work->pos);
        leon_burst(work, record);
        /* @hack spelling: the two calls as one expression, which keeps the call
         * to placement_kill a JSR (0x8c105a88). as two statements, SHC makes
         * it a tail jump, and 5 of 147 words differ */
        enemy_end(task, work, record), placement_kill(task);
        return;
    }
    leon_fade(work, record);
    enemy_save_pos(work, record);
    knuckles_find_piece(PIECE(record), &work->pos);
    work->timer++;
    if ((work->flags & LEON_SOLID) && FADE(record) > -0.8F) {
        collision_register(work);
        if (FADE(record) == 0.0F) {
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_ATTACK) | ATTACK(1);
            work->collision->shapes->levels =
                work->collision->shapes->levels & ~SHAPE_GUARD;
        } else {
            work->collision->shapes->levels =
                work->collision->shapes->levels & ~SHAPE_ATTACK;
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(3);
        }
    }
}
