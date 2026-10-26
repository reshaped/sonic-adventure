/* the saru, a monkey that walks a circle round where it was placed and
 * throws bombs or spiked balls at player 0. each throw is a child task, the
 * bomb, and a ball is a bomb with SARU_TOGEBALL in its submode. a bomb going
 * off makes the blast, a task of its own that draws the dome */
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
#include "draw/tree.h"
#include "effect/blast_dome.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "effect/spark.h"
#include "enemy/enemy.h"
#include "enemy/saru.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 4
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 4
#define TEXT_4_LENGTH 4
#define TEXT_5_LENGTH 3
#define TEXT_6_LENGTH 5
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 5
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 4
#define TEXT_15_LENGTH 5
#define TEXT_16_LENGTH 12
#include "enemy/saru/text.inc"

#define SE_CHEER 819
#define SE_THROW 820

/* saru_hand_minus_z, the hand the saru throws from, by its node number in
 * saru_object's tree */
#define SARU_HAND 14

/* the saru's work->mode. a walk on round its circle grows the saru's angle
 * round it, work->ang.x, and a walk back shrinks it */
enum saru_mode {
    SARU_WAIT = 0,
    SARU_HOP = 1,
    SARU_FIDGET = 2,
    SARU_TURN_ON = 3, /* a hop that turns it to walk on */
    SARU_WALK_ON = 4,
    SARU_TURN_BACK = 5, /* a hop that turns it to walk back */
    SARU_WALK_BACK = 6,
    SARU_FACE = 7, /* a hop that turns it to face player 0 */
    SARU_THROW = 9,
    SARU_CHEER = 10,
    SARU_BREAK = 12 /* an attack has beaten it: it breaks on its next step */
};

/* what the saru throws, in its work->submode. each bomb it throws copies
 * this into the bomb's own submode, where BOMB_BLINK_GAP replaces it once
 * the bomb lies */
enum saru_arms { SARU_BOMB, SARU_TOGEBALL, SARU_UNARMED };

/* the saru's bits in work->flags */
#define SARU_SOLID 0x100 /* it can be touched */
#define SARU_HOLDING 0x200 /* it holds what it throws */
/* a bomb the saru threw hit a player within the last SARU_HIT_TIME steps.
 * while this is set, the saru cheers where it would throw, hop or fidget */
#define SARU_HIT_PLAYER 0x400
/* saru_react has yet to turn the saru to face player 0 for that hit */
#define SARU_HIT_NEW 0x800
#define SARU_TO_GROUND 0x2000 /* it keeps 4 above the floor below it */

#define SARU_HIT_TIME 600

/* the range, squared, within which the camera or player 0 keeps the saru
 * placed. placement_try_leave_by takes it */
#define SARU_LEAVE_RANGE (700.0F * 700.0F)

/* the saru's bytes in work->vars */
struct saru_vars {
    /* ENEMY_MARK_BEATEN writes bytes 1 and 2, and the saru reads none */
    Uint8 unk_00[4];
    Sint32 since_hit; /* steps since its bomb last hit a player */
};

#define VARS(work) ((struct saru_vars *)&(work)->vars)

/* how weak the saru is, 0 to 255, which saru_start moves here from the
 * high byte of its set entry's ang.z */
#define WEAKNESS(work) ((work)->ang.z)

/* the bomb's work->mode. it flies, then lies blinking as its fuse burns,
 * then goes off. saru_bomb_draw and saru_bomb_touch can set it off
 * sooner */
enum bomb_mode { BOMB_FLY, BOMB_LIE, BOMB_BLAST, BOMB_END };

/* the submode a bomb starts to lie with. it blinks once its tick passes
 * its submode, which drops by 1 at each blink */
#define BOMB_BLINK_GAP 15

/* in the bomb's work->flags: a player has held it. from then on, it goes
 * off when it touches an item or an enemy */
#define BOMB_WAS_HELD 0x800

/* the blast's work->mode */
enum blast_mode { BLAST_INIT, BLAST_GROW, BLAST_END };

/* the steps the blast's dome grows for. it fades over the last half */
#define BLAST_TIME 60

/* the blast's bytes in work->vars */
struct blast_vars {
    Uint8 unk_00[4];
    Sint32 scroll;
    Float growth;
};

#define BLAST(work) ((struct blast_vars *)&(work)->vars)

/* record fields where the saru and its bomb keep state of their own */
#define FUSE(record) ((record)->motion.slip) /* the bomb's fuse length */
#define LIFT(record) ((record)->unk_78) /* how high its walk lifts it */
/* the scale of what the saru holds, which grows from 0 to 1 */
#define HELD_SCALE(record) ((record)->unk_7c)
/* the id of the master emerald piece the saru carries */
#define PIECE(record) ((record)->unk_4c)
/* the phase of the saru's sway as it waits */
#define SWAY(record) ((record)->angles[5])

/* start blending the record into action from frame. each BLEND_ADVANCE
 * adds speed to the blend */
#define BLEND_TO(record, action, speed, frame)                                 \
    ((record)->flags |= ENEMY_BLENDING, (record)->blend_speed = (speed),       \
     (record)->next_frame = (frame), (record)->next_action = (action))

/* once the blend passes 1, next_action at next_frame becomes the action,
 * and the blend ends */
#define BLEND_ADVANCE(record)                                                  \
    do {                                                                       \
        (record)->blend += (record)->blend_speed;                              \
        if ((record)->blend > 1.0F) {                                          \
            (record)->blend = 0.0F;                                            \
            (record)->frame = (record)->next_frame;                            \
            (record)->action = (record)->next_action;                          \
            (record)->flags &= ~ENEMY_BLENDING;                                \
        }                                                                      \
    } while (0)

static void saru_destroy(struct task *task);
static void saru_draw(struct task *task);
static void saru_step(struct task *task);

static NJS_TEXNAME e_saru_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};

NJS_TEXLIST e_saru_texlist = {
#include "enemy/saru/e_saru_texlist.inc"
};

/* the saru's tree. its motions and SARU_HAND number its 38 nodes depth
 * first, from saru_object at 0. the last, saru_tail_bone, is a hidden bone
 * off saru_spine with no model */
static NJS_OBJECT saru_tail_bone = {
#include "enemy/saru/saru_tail_bone.inc"
};

static NJS_MATERIAL saru_thigh_minus_z_materials[2] = {
#include "enemy/saru/saru_thigh_minus_z_materials.inc"
};

static Sint16 saru_thigh_minus_z_meshes0[12] = {
#include "enemy/saru/saru_thigh_minus_z_meshes0.inc"
};

static Sint16 saru_thigh_minus_z_meshes1[3] = {
#include "enemy/saru/saru_thigh_minus_z_meshes1.inc"
};

static NJS_COLOR saru_thigh_minus_z_uvs0[12] = {
#include "enemy/saru/saru_thigh_minus_z_uvs0.inc"
};

static NJS_MESHSET saru_thigh_minus_z_meshsets[2] = {
#include "enemy/saru/saru_thigh_minus_z_meshsets.inc"
};

static NJS_POINT3 saru_thigh_minus_z_points[6] = {
#include "enemy/saru/saru_thigh_minus_z_points.inc"
};

static NJS_VECTOR saru_thigh_minus_z_normals[6] = {
#include "enemy/saru/saru_thigh_minus_z_normals.inc"
};

static NJS_MODEL saru_thigh_minus_z_model = {
#include "enemy/saru/saru_thigh_minus_z_model.inc"
};

static NJS_OBJECT saru_thigh_minus_z = {
#include "enemy/saru/saru_thigh_minus_z.inc"
};

static NJS_MATERIAL saru_foot_minus_z_materials[4] = {
#include "enemy/saru/saru_foot_minus_z_materials.inc"
};

static Sint16 saru_foot_minus_z_meshes0[4] = {
#include "enemy/saru/saru_foot_minus_z_meshes0.inc"
};

static Sint16 saru_foot_minus_z_meshes1[7] = {
#include "enemy/saru/saru_foot_minus_z_meshes1.inc"
};

static Sint16 saru_foot_minus_z_meshes2[15] = {
#include "enemy/saru/saru_foot_minus_z_meshes2.inc"
};

static Sint16 saru_foot_minus_z_meshes3[12] = {
#include "enemy/saru/saru_foot_minus_z_meshes3.inc"
};

static Sint16 saru_foot_minus_z_meshes4[4] = {
#include "enemy/saru/saru_foot_minus_z_meshes4.inc"
};

static Sint16 saru_foot_minus_z_meshes5[3] = {
#include "enemy/saru/saru_foot_minus_z_meshes5.inc"
};

static NJS_COLOR saru_foot_minus_z_uvs2[15] = {
#include "enemy/saru/saru_foot_minus_z_uvs2.inc"
};

static NJS_COLOR saru_foot_minus_z_uvs3[12] = {
#include "enemy/saru/saru_foot_minus_z_uvs3.inc"
};

static NJS_COLOR saru_foot_minus_z_uvs4[4] = {
#include "enemy/saru/saru_foot_minus_z_uvs4.inc"
};

static NJS_COLOR saru_foot_minus_z_uvs5[3] = {
#include "enemy/saru/saru_foot_minus_z_uvs5.inc"
};

static NJS_MESHSET saru_foot_minus_z_meshsets[6] = {
#include "enemy/saru/saru_foot_minus_z_meshsets.inc"
};

static NJS_POINT3 saru_foot_minus_z_points[11] = {
#include "enemy/saru/saru_foot_minus_z_points.inc"
};

static NJS_VECTOR saru_foot_minus_z_normals[11] = {
#include "enemy/saru/saru_foot_minus_z_normals.inc"
};

static NJS_MODEL saru_foot_minus_z_model = {
#include "enemy/saru/saru_foot_minus_z_model.inc"
};

static NJS_OBJECT saru_foot_minus_z = {
#include "enemy/saru/saru_foot_minus_z.inc"
};

static NJS_MATERIAL saru_shin_minus_z_materials[1] = {
#include "enemy/saru/saru_shin_minus_z_materials.inc"
};

static Sint16 saru_shin_minus_z_meshes0[6] = {
#include "enemy/saru/saru_shin_minus_z_meshes0.inc"
};

static Sint16 saru_shin_minus_z_meshes1[12] = {
#include "enemy/saru/saru_shin_minus_z_meshes1.inc"
};

static NJS_COLOR saru_shin_minus_z_uvs0[6] = {
#include "enemy/saru/saru_shin_minus_z_uvs0.inc"
};

static NJS_COLOR saru_shin_minus_z_uvs1[12] = {
#include "enemy/saru/saru_shin_minus_z_uvs1.inc"
};

static NJS_MESHSET saru_shin_minus_z_meshsets[2] = {
#include "enemy/saru/saru_shin_minus_z_meshsets.inc"
};

static NJS_POINT3 saru_shin_minus_z_points[6] = {
#include "enemy/saru/saru_shin_minus_z_points.inc"
};

static NJS_VECTOR saru_shin_minus_z_normals[6] = {
#include "enemy/saru/saru_shin_minus_z_normals.inc"
};

static NJS_MODEL saru_shin_minus_z_model = {
#include "enemy/saru/saru_shin_minus_z_model.inc"
};

static NJS_OBJECT saru_shin_minus_z = {
#include "enemy/saru/saru_shin_minus_z.inc"
};

static NJS_OBJECT saru_shin_tip_minus_z = {
#include "enemy/saru/saru_shin_tip_minus_z.inc"
};

static NJS_OBJECT saru_shin_bone_minus_z = {
#include "enemy/saru/saru_shin_bone_minus_z.inc"
};

static NJS_OBJECT saru_thigh_bone_minus_z = {
#include "enemy/saru/saru_thigh_bone_minus_z.inc"
};

static NJS_OBJECT saru_hip_minus_z = {
#include "enemy/saru/saru_hip_minus_z.inc"
};

static NJS_MATERIAL saru_thigh_plus_z_materials[2] = {
#include "enemy/saru/saru_thigh_plus_z_materials.inc"
};

static Sint16 saru_thigh_plus_z_meshes0[12] = {
#include "enemy/saru/saru_thigh_plus_z_meshes0.inc"
};

static Sint16 saru_thigh_plus_z_meshes1[3] = {
#include "enemy/saru/saru_thigh_plus_z_meshes1.inc"
};

static NJS_COLOR saru_thigh_plus_z_uvs0[12] = {
#include "enemy/saru/saru_thigh_plus_z_uvs0.inc"
};

static NJS_MESHSET saru_thigh_plus_z_meshsets[2] = {
#include "enemy/saru/saru_thigh_plus_z_meshsets.inc"
};

static NJS_POINT3 saru_thigh_plus_z_points[6] = {
#include "enemy/saru/saru_thigh_plus_z_points.inc"
};

static NJS_VECTOR saru_thigh_plus_z_normals[6] = {
#include "enemy/saru/saru_thigh_plus_z_normals.inc"
};

static NJS_MODEL saru_thigh_plus_z_model = {
#include "enemy/saru/saru_thigh_plus_z_model.inc"
};

static NJS_OBJECT saru_thigh_plus_z = {
#include "enemy/saru/saru_thigh_plus_z.inc"
};

static NJS_MATERIAL saru_foot_plus_z_materials[4] = {
#include "enemy/saru/saru_foot_plus_z_materials.inc"
};

static Sint16 saru_foot_plus_z_meshes0[4] = {
#include "enemy/saru/saru_foot_plus_z_meshes0.inc"
};

static Sint16 saru_foot_plus_z_meshes1[7] = {
#include "enemy/saru/saru_foot_plus_z_meshes1.inc"
};

static Sint16 saru_foot_plus_z_meshes2[15] = {
#include "enemy/saru/saru_foot_plus_z_meshes2.inc"
};

static Sint16 saru_foot_plus_z_meshes3[12] = {
#include "enemy/saru/saru_foot_plus_z_meshes3.inc"
};

static Sint16 saru_foot_plus_z_meshes4[4] = {
#include "enemy/saru/saru_foot_plus_z_meshes4.inc"
};

static Sint16 saru_foot_plus_z_meshes5[3] = {
#include "enemy/saru/saru_foot_plus_z_meshes5.inc"
};

static NJS_COLOR saru_foot_plus_z_uvs2[15] = {
#include "enemy/saru/saru_foot_plus_z_uvs2.inc"
};

static NJS_COLOR saru_foot_plus_z_uvs3[12] = {
#include "enemy/saru/saru_foot_plus_z_uvs3.inc"
};

static NJS_COLOR saru_foot_plus_z_uvs4[4] = {
#include "enemy/saru/saru_foot_plus_z_uvs4.inc"
};

static NJS_COLOR saru_foot_plus_z_uvs5[3] = {
#include "enemy/saru/saru_foot_plus_z_uvs5.inc"
};

static NJS_MESHSET saru_foot_plus_z_meshsets[6] = {
#include "enemy/saru/saru_foot_plus_z_meshsets.inc"
};

static NJS_POINT3 saru_foot_plus_z_points[11] = {
#include "enemy/saru/saru_foot_plus_z_points.inc"
};

static NJS_VECTOR saru_foot_plus_z_normals[11] = {
#include "enemy/saru/saru_foot_plus_z_normals.inc"
};

static NJS_MODEL saru_foot_plus_z_model = {
#include "enemy/saru/saru_foot_plus_z_model.inc"
};

static NJS_OBJECT saru_foot_plus_z = {
#include "enemy/saru/saru_foot_plus_z.inc"
};

static NJS_MATERIAL saru_shin_plus_z_materials[1] = {
#include "enemy/saru/saru_shin_plus_z_materials.inc"
};

static Sint16 saru_shin_plus_z_meshes0[6] = {
#include "enemy/saru/saru_shin_plus_z_meshes0.inc"
};

static Sint16 saru_shin_plus_z_meshes1[12] = {
#include "enemy/saru/saru_shin_plus_z_meshes1.inc"
};

static NJS_COLOR saru_shin_plus_z_uvs0[6] = {
#include "enemy/saru/saru_shin_plus_z_uvs0.inc"
};

static NJS_COLOR saru_shin_plus_z_uvs1[12] = {
#include "enemy/saru/saru_shin_plus_z_uvs1.inc"
};

static NJS_MESHSET saru_shin_plus_z_meshsets[2] = {
#include "enemy/saru/saru_shin_plus_z_meshsets.inc"
};

static NJS_POINT3 saru_shin_plus_z_points[6] = {
#include "enemy/saru/saru_shin_plus_z_points.inc"
};

static NJS_VECTOR saru_shin_plus_z_normals[6] = {
#include "enemy/saru/saru_shin_plus_z_normals.inc"
};

static NJS_MODEL saru_shin_plus_z_model = {
#include "enemy/saru/saru_shin_plus_z_model.inc"
};

static NJS_OBJECT saru_shin_plus_z = {
#include "enemy/saru/saru_shin_plus_z.inc"
};

static NJS_OBJECT saru_shin_tip_plus_z = {
#include "enemy/saru/saru_shin_tip_plus_z.inc"
};

static NJS_OBJECT saru_shin_bone_plus_z = {
#include "enemy/saru/saru_shin_bone_plus_z.inc"
};

static NJS_OBJECT saru_thigh_bone_plus_z = {
#include "enemy/saru/saru_thigh_bone_plus_z.inc"
};

static NJS_OBJECT saru_hip_plus_z = {
#include "enemy/saru/saru_hip_plus_z.inc"
};

static NJS_MATERIAL saru_upper_arm_plus_z_materials[3] = {
#include "enemy/saru/saru_upper_arm_plus_z_materials.inc"
};

static Sint16 saru_upper_arm_plus_z_meshes0[12] = {
#include "enemy/saru/saru_upper_arm_plus_z_meshes0.inc"
};

static Sint16 saru_upper_arm_plus_z_meshes1[3] = {
#include "enemy/saru/saru_upper_arm_plus_z_meshes1.inc"
};

static Sint16 saru_upper_arm_plus_z_meshes2[3] = {
#include "enemy/saru/saru_upper_arm_plus_z_meshes2.inc"
};

static NJS_COLOR saru_upper_arm_plus_z_uvs0[12] = {
#include "enemy/saru/saru_upper_arm_plus_z_uvs0.inc"
};

static NJS_COLOR saru_upper_arm_plus_z_uvs1[3] = {
#include "enemy/saru/saru_upper_arm_plus_z_uvs1.inc"
};

static NJS_MESHSET saru_upper_arm_plus_z_meshsets[3] = {
#include "enemy/saru/saru_upper_arm_plus_z_meshsets.inc"
};

static NJS_POINT3 saru_upper_arm_plus_z_points[6] = {
#include "enemy/saru/saru_upper_arm_plus_z_points.inc"
};

static NJS_VECTOR saru_upper_arm_plus_z_normals[6] = {
#include "enemy/saru/saru_upper_arm_plus_z_normals.inc"
};

static NJS_MODEL saru_upper_arm_plus_z_model = {
#include "enemy/saru/saru_upper_arm_plus_z_model.inc"
};

static NJS_OBJECT saru_upper_arm_plus_z = {
#include "enemy/saru/saru_upper_arm_plus_z.inc"
};

static NJS_MATERIAL saru_hand_plus_z_materials[3] = {
#include "enemy/saru/saru_hand_plus_z_materials.inc"
};

static Sint16 saru_hand_plus_z_meshes0[6] = {
#include "enemy/saru/saru_hand_plus_z_meshes0.inc"
};

static Sint16 saru_hand_plus_z_meshes1[4] = {
#include "enemy/saru/saru_hand_plus_z_meshes1.inc"
};

static Sint16 saru_hand_plus_z_meshes2[33] = {
#include "enemy/saru/saru_hand_plus_z_meshes2.inc"
};

static Sint16 saru_hand_plus_z_meshes3[4] = {
#include "enemy/saru/saru_hand_plus_z_meshes3.inc"
};

static Sint16 saru_hand_plus_z_meshes4[16] = {
#include "enemy/saru/saru_hand_plus_z_meshes4.inc"
};

static NJS_COLOR saru_hand_plus_z_uvs0[6] = {
#include "enemy/saru/saru_hand_plus_z_uvs0.inc"
};

static NJS_COLOR saru_hand_plus_z_uvs1[4] = {
#include "enemy/saru/saru_hand_plus_z_uvs1.inc"
};

static NJS_COLOR saru_hand_plus_z_uvs2[33] = {
#include "enemy/saru/saru_hand_plus_z_uvs2.inc"
};

static NJS_COLOR saru_hand_plus_z_uvs3[4] = {
#include "enemy/saru/saru_hand_plus_z_uvs3.inc"
};

static NJS_COLOR saru_hand_plus_z_uvs4[16] = {
#include "enemy/saru/saru_hand_plus_z_uvs4.inc"
};

static NJS_MESHSET saru_hand_plus_z_meshsets[5] = {
#include "enemy/saru/saru_hand_plus_z_meshsets.inc"
};

static NJS_POINT3 saru_hand_plus_z_points[17] = {
#include "enemy/saru/saru_hand_plus_z_points.inc"
};

static NJS_VECTOR saru_hand_plus_z_normals[17] = {
#include "enemy/saru/saru_hand_plus_z_normals.inc"
};

static NJS_MODEL saru_hand_plus_z_model = {
#include "enemy/saru/saru_hand_plus_z_model.inc"
};

static NJS_OBJECT saru_hand_plus_z = {
#include "enemy/saru/saru_hand_plus_z.inc"
};

static NJS_MATERIAL saru_forearm_plus_z_materials[1] = {
#include "enemy/saru/saru_forearm_plus_z_materials.inc"
};

static Sint16 saru_forearm_plus_z_meshes0[6] = {
#include "enemy/saru/saru_forearm_plus_z_meshes0.inc"
};

static Sint16 saru_forearm_plus_z_meshes1[12] = {
#include "enemy/saru/saru_forearm_plus_z_meshes1.inc"
};

static NJS_COLOR saru_forearm_plus_z_uvs0[6] = {
#include "enemy/saru/saru_forearm_plus_z_uvs0.inc"
};

static NJS_COLOR saru_forearm_plus_z_uvs1[12] = {
#include "enemy/saru/saru_forearm_plus_z_uvs1.inc"
};

static NJS_MESHSET saru_forearm_plus_z_meshsets[2] = {
#include "enemy/saru/saru_forearm_plus_z_meshsets.inc"
};

static NJS_POINT3 saru_forearm_plus_z_points[6] = {
#include "enemy/saru/saru_forearm_plus_z_points.inc"
};

static NJS_VECTOR saru_forearm_plus_z_normals[6] = {
#include "enemy/saru/saru_forearm_plus_z_normals.inc"
};

static NJS_MODEL saru_forearm_plus_z_model = {
#include "enemy/saru/saru_forearm_plus_z_model.inc"
};

static NJS_OBJECT saru_forearm_plus_z = {
#include "enemy/saru/saru_forearm_plus_z.inc"
};

static NJS_OBJECT saru_forearm_tip_plus_z = {
#include "enemy/saru/saru_forearm_tip_plus_z.inc"
};

static NJS_OBJECT saru_forearm_bone_plus_z = {
#include "enemy/saru/saru_forearm_bone_plus_z.inc"
};

static NJS_OBJECT saru_upper_arm_bone_plus_z = {
#include "enemy/saru/saru_upper_arm_bone_plus_z.inc"
};

static NJS_OBJECT saru_shoulder_plus_z = {
#include "enemy/saru/saru_shoulder_plus_z.inc"
};

static NJS_MATERIAL saru_upper_arm_minus_z_materials[3] = {
#include "enemy/saru/saru_upper_arm_minus_z_materials.inc"
};

static Sint16 saru_upper_arm_minus_z_meshes0[12] = {
#include "enemy/saru/saru_upper_arm_minus_z_meshes0.inc"
};

static Sint16 saru_upper_arm_minus_z_meshes1[3] = {
#include "enemy/saru/saru_upper_arm_minus_z_meshes1.inc"
};

static Sint16 saru_upper_arm_minus_z_meshes2[3] = {
#include "enemy/saru/saru_upper_arm_minus_z_meshes2.inc"
};

static NJS_COLOR saru_upper_arm_minus_z_uvs0[12] = {
#include "enemy/saru/saru_upper_arm_minus_z_uvs0.inc"
};

static NJS_COLOR saru_upper_arm_minus_z_uvs1[3] = {
#include "enemy/saru/saru_upper_arm_minus_z_uvs1.inc"
};

static NJS_MESHSET saru_upper_arm_minus_z_meshsets[3] = {
#include "enemy/saru/saru_upper_arm_minus_z_meshsets.inc"
};

static NJS_POINT3 saru_upper_arm_minus_z_points[6] = {
#include "enemy/saru/saru_upper_arm_minus_z_points.inc"
};

static NJS_VECTOR saru_upper_arm_minus_z_normals[6] = {
#include "enemy/saru/saru_upper_arm_minus_z_normals.inc"
};

static NJS_MODEL saru_upper_arm_minus_z_model = {
#include "enemy/saru/saru_upper_arm_minus_z_model.inc"
};

static NJS_OBJECT saru_upper_arm_minus_z = {
#include "enemy/saru/saru_upper_arm_minus_z.inc"
};

static NJS_MATERIAL saru_hand_minus_z_materials[3] = {
#include "enemy/saru/saru_hand_minus_z_materials.inc"
};

static Sint16 saru_hand_minus_z_meshes0[6] = {
#include "enemy/saru/saru_hand_minus_z_meshes0.inc"
};

static Sint16 saru_hand_minus_z_meshes1[4] = {
#include "enemy/saru/saru_hand_minus_z_meshes1.inc"
};

static Sint16 saru_hand_minus_z_meshes2[33] = {
#include "enemy/saru/saru_hand_minus_z_meshes2.inc"
};

static Sint16 saru_hand_minus_z_meshes3[4] = {
#include "enemy/saru/saru_hand_minus_z_meshes3.inc"
};

static Sint16 saru_hand_minus_z_meshes4[16] = {
#include "enemy/saru/saru_hand_minus_z_meshes4.inc"
};

static NJS_COLOR saru_hand_minus_z_uvs0[6] = {
#include "enemy/saru/saru_hand_minus_z_uvs0.inc"
};

static NJS_COLOR saru_hand_minus_z_uvs1[4] = {
#include "enemy/saru/saru_hand_minus_z_uvs1.inc"
};

static NJS_COLOR saru_hand_minus_z_uvs2[33] = {
#include "enemy/saru/saru_hand_minus_z_uvs2.inc"
};

static NJS_COLOR saru_hand_minus_z_uvs3[4] = {
#include "enemy/saru/saru_hand_minus_z_uvs3.inc"
};

static NJS_COLOR saru_hand_minus_z_uvs4[16] = {
#include "enemy/saru/saru_hand_minus_z_uvs4.inc"
};

static NJS_MESHSET saru_hand_minus_z_meshsets[5] = {
#include "enemy/saru/saru_hand_minus_z_meshsets.inc"
};

static NJS_POINT3 saru_hand_minus_z_points[17] = {
#include "enemy/saru/saru_hand_minus_z_points.inc"
};

static NJS_VECTOR saru_hand_minus_z_normals[17] = {
#include "enemy/saru/saru_hand_minus_z_normals.inc"
};

static NJS_MODEL saru_hand_minus_z_model = {
#include "enemy/saru/saru_hand_minus_z_model.inc"
};

static NJS_OBJECT saru_hand_minus_z = {
#include "enemy/saru/saru_hand_minus_z.inc"
};

static NJS_MATERIAL saru_forearm_minus_z_materials[1] = {
#include "enemy/saru/saru_forearm_minus_z_materials.inc"
};

static Sint16 saru_forearm_minus_z_meshes0[6] = {
#include "enemy/saru/saru_forearm_minus_z_meshes0.inc"
};

static Sint16 saru_forearm_minus_z_meshes1[12] = {
#include "enemy/saru/saru_forearm_minus_z_meshes1.inc"
};

static NJS_COLOR saru_forearm_minus_z_uvs0[6] = {
#include "enemy/saru/saru_forearm_minus_z_uvs0.inc"
};

static NJS_COLOR saru_forearm_minus_z_uvs1[12] = {
#include "enemy/saru/saru_forearm_minus_z_uvs1.inc"
};

static NJS_MESHSET saru_forearm_minus_z_meshsets[2] = {
#include "enemy/saru/saru_forearm_minus_z_meshsets.inc"
};

static NJS_POINT3 saru_forearm_minus_z_points[6] = {
#include "enemy/saru/saru_forearm_minus_z_points.inc"
};

static NJS_VECTOR saru_forearm_minus_z_normals[6] = {
#include "enemy/saru/saru_forearm_minus_z_normals.inc"
};

static NJS_MODEL saru_forearm_minus_z_model = {
#include "enemy/saru/saru_forearm_minus_z_model.inc"
};

static NJS_OBJECT saru_forearm_minus_z = {
#include "enemy/saru/saru_forearm_minus_z.inc"
};

static NJS_OBJECT saru_forearm_tip_minus_z = {
#include "enemy/saru/saru_forearm_tip_minus_z.inc"
};

static NJS_OBJECT saru_forearm_bone_minus_z = {
#include "enemy/saru/saru_forearm_bone_minus_z.inc"
};

static NJS_OBJECT saru_upper_arm_bone_minus_z = {
#include "enemy/saru/saru_upper_arm_bone_minus_z.inc"
};

static NJS_OBJECT saru_shoulder_minus_z = {
#include "enemy/saru/saru_shoulder_minus_z.inc"
};

static NJS_MATERIAL saru_ears_materials[2] = {
#include "enemy/saru/saru_ears_materials.inc"
};

static Sint16 saru_ears_meshes0[30] = {
#include "enemy/saru/saru_ears_meshes0.inc"
};

static Sint16 saru_ears_meshes1[42] = {
#include "enemy/saru/saru_ears_meshes1.inc"
};

static NJS_COLOR saru_ears_uvs0[30] = {
#include "enemy/saru/saru_ears_uvs0.inc"
};

static NJS_COLOR saru_ears_uvs1[42] = {
#include "enemy/saru/saru_ears_uvs1.inc"
};

static NJS_MESHSET saru_ears_meshsets[2] = {
#include "enemy/saru/saru_ears_meshsets.inc"
};

static NJS_POINT3 saru_ears_points[16] = {
#include "enemy/saru/saru_ears_points.inc"
};

static NJS_VECTOR saru_ears_normals[16] = {
#include "enemy/saru/saru_ears_normals.inc"
};

static NJS_MODEL saru_ears_model = {
#include "enemy/saru/saru_ears_model.inc"
};

static NJS_OBJECT saru_ears = {
#include "enemy/saru/saru_ears.inc"
};

static NJS_MATERIAL saru_head_materials[4] = {
#include "enemy/saru/saru_head_materials.inc"
};

static Sint16 saru_head_meshes0[36] = {
#include "enemy/saru/saru_head_meshes0.inc"
};

static Sint16 saru_head_meshes1[48] = {
#include "enemy/saru/saru_head_meshes1.inc"
};

static Sint16 saru_head_meshes2[12] = {
#include "enemy/saru/saru_head_meshes2.inc"
};

static Sint16 saru_head_meshes3[18] = {
#include "enemy/saru/saru_head_meshes3.inc"
};

static Sint16 saru_head_meshes4[18] = {
#include "enemy/saru/saru_head_meshes4.inc"
};

static NJS_COLOR saru_head_uvs0[36] = {
#include "enemy/saru/saru_head_uvs0.inc"
};

static NJS_COLOR saru_head_uvs1[48] = {
#include "enemy/saru/saru_head_uvs1.inc"
};

static NJS_COLOR saru_head_uvs2[12] = {
#include "enemy/saru/saru_head_uvs2.inc"
};

static NJS_COLOR saru_head_uvs3[18] = {
#include "enemy/saru/saru_head_uvs3.inc"
};

static NJS_COLOR saru_head_uvs4[18] = {
#include "enemy/saru/saru_head_uvs4.inc"
};

static NJS_MESHSET saru_head_meshsets[5] = {
#include "enemy/saru/saru_head_meshsets.inc"
};

static NJS_POINT3 saru_head_points[28] = {
#include "enemy/saru/saru_head_points.inc"
};

static NJS_VECTOR saru_head_normals[28] = {
#include "enemy/saru/saru_head_normals.inc"
};

static NJS_MODEL saru_head_model = {
#include "enemy/saru/saru_head_model.inc"
};

static NJS_OBJECT saru_head = {
#include "enemy/saru/saru_head.inc"
};

static NJS_OBJECT saru_head_tip = {
#include "enemy/saru/saru_head_tip.inc"
};

static NJS_OBJECT saru_head_bone = {
#include "enemy/saru/saru_head_bone.inc"
};

static NJS_OBJECT saru_neck = {
#include "enemy/saru/saru_neck.inc"
};

static NJS_MATERIAL saru_body_materials[3] = {
#include "enemy/saru/saru_body_materials.inc"
};

static Sint16 saru_body_meshes0[15] = {
#include "enemy/saru/saru_body_meshes0.inc"
};

static Sint16 saru_body_meshes1[20] = {
#include "enemy/saru/saru_body_meshes1.inc"
};

static Sint16 saru_body_meshes2[15] = {
#include "enemy/saru/saru_body_meshes2.inc"
};

static NJS_COLOR saru_body_uvs0[15] = {
#include "enemy/saru/saru_body_uvs0.inc"
};

static NJS_COLOR saru_body_uvs1[20] = {
#include "enemy/saru/saru_body_uvs1.inc"
};

static NJS_COLOR saru_body_uvs2[15] = {
#include "enemy/saru/saru_body_uvs2.inc"
};

static NJS_MESHSET saru_body_meshsets[3] = {
#include "enemy/saru/saru_body_meshsets.inc"
};

static NJS_POINT3 saru_body_points[12] = {
#include "enemy/saru/saru_body_points.inc"
};

static NJS_VECTOR saru_body_normals[12] = {
#include "enemy/saru/saru_body_normals.inc"
};

static NJS_MODEL saru_body_model = {
#include "enemy/saru/saru_body_model.inc"
};

static NJS_OBJECT saru_body = {
#include "enemy/saru/saru_body.inc"
};

static NJS_OBJECT saru_spine_tip = {
#include "enemy/saru/saru_spine_tip.inc"
};

static NJS_OBJECT saru_spine = {
#include "enemy/saru/saru_spine.inc"
};

NJS_OBJECT saru_object = {
#include "enemy/saru/saru_object.inc"
};

static NJS_MATERIAL saru_body_prism_materials[1] = {
#include "enemy/saru/saru_body_prism_materials.inc"
};

static Sint16 saru_body_prism_meshes0[4] = {
#include "enemy/saru/saru_body_prism_meshes0.inc"
};

static Sint16 saru_body_prism_meshes1[12] = {
#include "enemy/saru/saru_body_prism_meshes1.inc"
};

static NJS_COLOR saru_body_prism_uvs0[3] = {
#include "enemy/saru/saru_body_prism_uvs0.inc"
};

static NJS_COLOR saru_body_prism_uvs1[12] = {
#include "enemy/saru/saru_body_prism_uvs1.inc"
};

static NJS_MESHSET saru_body_prism_meshsets[2] = {
#include "enemy/saru/saru_body_prism_meshsets.inc"
};

static NJS_POINT3 saru_body_prism_points[6] = {
#include "enemy/saru/saru_body_prism_points.inc"
};

static NJS_VECTOR saru_body_prism_normals[6] = {
#include "enemy/saru/saru_body_prism_normals.inc"
};

NJS_MODEL saru_body_prism_model = {
#include "enemy/saru/saru_body_prism_model.inc"
};

static NJS_OBJECT saru_body_prism = {
#include "enemy/saru/saru_body_prism.inc"
};

static NJS_MATERIAL saru_arm_prism_materials[2] = {
#include "enemy/saru/saru_arm_prism_materials.inc"
};

static Sint16 saru_arm_prism_meshes0[12] = {
#include "enemy/saru/saru_arm_prism_meshes0.inc"
};

static Sint16 saru_arm_prism_meshes1[4] = {
#include "enemy/saru/saru_arm_prism_meshes1.inc"
};

static NJS_COLOR saru_arm_prism_uvs0[12] = {
#include "enemy/saru/saru_arm_prism_uvs0.inc"
};

static NJS_MESHSET saru_arm_prism_meshsets[2] = {
#include "enemy/saru/saru_arm_prism_meshsets.inc"
};

static NJS_POINT3 saru_arm_prism_points[6] = {
#include "enemy/saru/saru_arm_prism_points.inc"
};

static NJS_VECTOR saru_arm_prism_normals[6] = {
#include "enemy/saru/saru_arm_prism_normals.inc"
};

NJS_MODEL saru_arm_prism_model = {
#include "enemy/saru/saru_arm_prism_model.inc"
};

static NJS_OBJECT saru_arm_prism = {
#include "enemy/saru/saru_arm_prism.inc"
};

static NJS_MKEY_F saru_jump_pos0[16] = {
#include "enemy/saru/saru_jump_pos0.inc"
};

static NJS_MKEY_A saru_jump_ang1[16] = {
#include "enemy/saru/saru_jump_ang1.inc"
};

static NJS_MKEY_A saru_jump_ang5[16] = {
#include "enemy/saru/saru_jump_ang5.inc"
};

static NJS_MKEY_A saru_jump_ang10[16] = {
#include "enemy/saru/saru_jump_ang10.inc"
};

static NJS_MKEY_A saru_jump_ang11[16] = {
#include "enemy/saru/saru_jump_ang11.inc"
};

static NJS_MKEY_A saru_jump_ang14[16] = {
#include "enemy/saru/saru_jump_ang14.inc"
};

static NJS_MKEY_A saru_jump_ang17[16] = {
#include "enemy/saru/saru_jump_ang17.inc"
};

static NJS_MKEY_A saru_jump_ang18[16] = {
#include "enemy/saru/saru_jump_ang18.inc"
};

static NJS_MKEY_A saru_jump_ang21[16] = {
#include "enemy/saru/saru_jump_ang21.inc"
};

static NJS_MKEY_A saru_jump_ang24[16] = {
#include "enemy/saru/saru_jump_ang24.inc"
};

static NJS_MKEY_A saru_jump_ang25[16] = {
#include "enemy/saru/saru_jump_ang25.inc"
};

static NJS_MKEY_A saru_jump_ang28[16] = {
#include "enemy/saru/saru_jump_ang28.inc"
};

static NJS_MKEY_A saru_jump_ang31[16] = {
#include "enemy/saru/saru_jump_ang31.inc"
};

static NJS_MKEY_A saru_jump_ang32[16] = {
#include "enemy/saru/saru_jump_ang32.inc"
};

static NJS_MKEY_A saru_jump_ang35[16] = {
#include "enemy/saru/saru_jump_ang35.inc"
};

static NJS_MDATA2 saru_jump_mdata[38] = {
#include "enemy/saru/saru_jump_mdata.inc"
};

static NJS_MOTION saru_jump_motion = {
#include "enemy/saru/saru_jump_motion.inc"
};

NJS_ACTION saru_jump_action = {
#include "enemy/saru/saru_jump_action.inc"
};

static NJS_MKEY_F saru_flip_pos0[16] = {
#include "enemy/saru/saru_flip_pos0.inc"
};

static NJS_MKEY_A saru_flip_ang1[16] = {
#include "enemy/saru/saru_flip_ang1.inc"
};

static NJS_MKEY_A saru_flip_ang5[16] = {
#include "enemy/saru/saru_flip_ang5.inc"
};

static NJS_MKEY_A saru_flip_ang10[16] = {
#include "enemy/saru/saru_flip_ang10.inc"
};

static NJS_MKEY_A saru_flip_ang11[16] = {
#include "enemy/saru/saru_flip_ang11.inc"
};

static NJS_MKEY_A saru_flip_ang17[16] = {
#include "enemy/saru/saru_flip_ang17.inc"
};

static NJS_MKEY_A saru_flip_ang18[16] = {
#include "enemy/saru/saru_flip_ang18.inc"
};

static NJS_MKEY_A saru_flip_ang24[16] = {
#include "enemy/saru/saru_flip_ang24.inc"
};

static NJS_MKEY_A saru_flip_ang25[16] = {
#include "enemy/saru/saru_flip_ang25.inc"
};

static NJS_MKEY_A saru_flip_ang28[16] = {
#include "enemy/saru/saru_flip_ang28.inc"
};

static NJS_MKEY_A saru_flip_ang31[16] = {
#include "enemy/saru/saru_flip_ang31.inc"
};

static NJS_MKEY_A saru_flip_ang32[16] = {
#include "enemy/saru/saru_flip_ang32.inc"
};

static NJS_MKEY_A saru_flip_ang35[16] = {
#include "enemy/saru/saru_flip_ang35.inc"
};

static NJS_MDATA2 saru_flip_mdata[38] = {
#include "enemy/saru/saru_flip_mdata.inc"
};

static NJS_MOTION saru_flip_motion = {
#include "enemy/saru/saru_flip_motion.inc"
};

NJS_ACTION saru_flip_action = {
#include "enemy/saru/saru_flip_action.inc"
};

static NJS_MKEY_F saru_walk_pos0[16] = {
#include "enemy/saru/saru_walk_pos0.inc"
};

static NJS_MKEY_A saru_walk_ang1[16] = {
#include "enemy/saru/saru_walk_ang1.inc"
};

static NJS_MKEY_A saru_walk_ang5[16] = {
#include "enemy/saru/saru_walk_ang5.inc"
};

static NJS_MKEY_A saru_walk_ang10[16] = {
#include "enemy/saru/saru_walk_ang10.inc"
};

static NJS_MKEY_A saru_walk_ang11[16] = {
#include "enemy/saru/saru_walk_ang11.inc"
};

static NJS_MKEY_A saru_walk_ang14[16] = {
#include "enemy/saru/saru_walk_ang14.inc"
};

static NJS_MKEY_A saru_walk_ang17[16] = {
#include "enemy/saru/saru_walk_ang17.inc"
};

static NJS_MKEY_A saru_walk_ang18[16] = {
#include "enemy/saru/saru_walk_ang18.inc"
};

static NJS_MKEY_A saru_walk_ang21[16] = {
#include "enemy/saru/saru_walk_ang21.inc"
};

static NJS_MKEY_A saru_walk_ang24[16] = {
#include "enemy/saru/saru_walk_ang24.inc"
};

static NJS_MKEY_A saru_walk_ang25[16] = {
#include "enemy/saru/saru_walk_ang25.inc"
};

static NJS_MKEY_A saru_walk_ang28[16] = {
#include "enemy/saru/saru_walk_ang28.inc"
};

static NJS_MKEY_A saru_walk_ang31[16] = {
#include "enemy/saru/saru_walk_ang31.inc"
};

static NJS_MKEY_A saru_walk_ang32[16] = {
#include "enemy/saru/saru_walk_ang32.inc"
};

static NJS_MKEY_A saru_walk_ang35[16] = {
#include "enemy/saru/saru_walk_ang35.inc"
};

static NJS_MDATA2 saru_walk_mdata[38] = {
#include "enemy/saru/saru_walk_mdata.inc"
};

static NJS_MOTION saru_walk_motion = {
#include "enemy/saru/saru_walk_motion.inc"
};

NJS_ACTION saru_walk_action = {
#include "enemy/saru/saru_walk_action.inc"
};

static NJS_MKEY_F saru_throw_pos0[11] = {
#include "enemy/saru/saru_throw_pos0.inc"
};

static NJS_MKEY_A saru_throw_ang1[13] = {
#include "enemy/saru/saru_throw_ang1.inc"
};

static NJS_MKEY_A saru_throw_ang5[13] = {
#include "enemy/saru/saru_throw_ang5.inc"
};

static NJS_MKEY_A saru_throw_ang10[13] = {
#include "enemy/saru/saru_throw_ang10.inc"
};

static NJS_MKEY_A saru_throw_ang11[13] = {
#include "enemy/saru/saru_throw_ang11.inc"
};

static NJS_MKEY_A saru_throw_ang14[13] = {
#include "enemy/saru/saru_throw_ang14.inc"
};

static NJS_MKEY_A saru_throw_ang17[13] = {
#include "enemy/saru/saru_throw_ang17.inc"
};

static NJS_MKEY_A saru_throw_ang18[13] = {
#include "enemy/saru/saru_throw_ang18.inc"
};

static NJS_MKEY_A saru_throw_ang21[13] = {
#include "enemy/saru/saru_throw_ang21.inc"
};

static NJS_MKEY_A saru_throw_ang24[13] = {
#include "enemy/saru/saru_throw_ang24.inc"
};

static NJS_MKEY_A saru_throw_ang25[13] = {
#include "enemy/saru/saru_throw_ang25.inc"
};

static NJS_MKEY_A saru_throw_ang28[9] = {
#include "enemy/saru/saru_throw_ang28.inc"
};

static NJS_MKEY_A saru_throw_ang31[13] = {
#include "enemy/saru/saru_throw_ang31.inc"
};

static NJS_MKEY_A saru_throw_ang32[13] = {
#include "enemy/saru/saru_throw_ang32.inc"
};

static NJS_MKEY_A saru_throw_ang35[13] = {
#include "enemy/saru/saru_throw_ang35.inc"
};

static NJS_MDATA2 saru_throw_mdata[38] = {
#include "enemy/saru/saru_throw_mdata.inc"
};

static NJS_MOTION saru_throw_motion = {
#include "enemy/saru/saru_throw_motion.inc"
};

NJS_ACTION saru_throw_action = {
#include "enemy/saru/saru_throw_action.inc"
};

static NJS_MKEY_F saru_hop_pos0[16] = {
#include "enemy/saru/saru_hop_pos0.inc"
};

static NJS_MKEY_A saru_hop_ang1[16] = {
#include "enemy/saru/saru_hop_ang1.inc"
};

static NJS_MKEY_A saru_hop_ang5[16] = {
#include "enemy/saru/saru_hop_ang5.inc"
};

static NJS_MKEY_A saru_hop_ang10[16] = {
#include "enemy/saru/saru_hop_ang10.inc"
};

static NJS_MKEY_A saru_hop_ang11[16] = {
#include "enemy/saru/saru_hop_ang11.inc"
};

static NJS_MKEY_A saru_hop_ang17[16] = {
#include "enemy/saru/saru_hop_ang17.inc"
};

static NJS_MKEY_A saru_hop_ang18[16] = {
#include "enemy/saru/saru_hop_ang18.inc"
};

static NJS_MKEY_A saru_hop_ang24[16] = {
#include "enemy/saru/saru_hop_ang24.inc"
};

static NJS_MKEY_A saru_hop_ang25[16] = {
#include "enemy/saru/saru_hop_ang25.inc"
};

static NJS_MKEY_A saru_hop_ang28[16] = {
#include "enemy/saru/saru_hop_ang28.inc"
};

static NJS_MKEY_A saru_hop_ang31[16] = {
#include "enemy/saru/saru_hop_ang31.inc"
};

static NJS_MKEY_A saru_hop_ang32[16] = {
#include "enemy/saru/saru_hop_ang32.inc"
};

static NJS_MKEY_A saru_hop_ang35[16] = {
#include "enemy/saru/saru_hop_ang35.inc"
};

static NJS_MDATA2 saru_hop_mdata[38] = {
#include "enemy/saru/saru_hop_mdata.inc"
};

static NJS_MOTION saru_hop_motion = {
#include "enemy/saru/saru_hop_motion.inc"
};

NJS_ACTION saru_hop_action = {
#include "enemy/saru/saru_hop_action.inc"
};

static NJS_MKEY_F saru_fidget_pos0[2] = {
#include "enemy/saru/saru_fidget_pos0.inc"
};

static NJS_MKEY_A saru_fidget_ang0[2] = {
#include "enemy/saru/saru_fidget_ang0.inc"
};

static NJS_MKEY_F saru_fidget_pos1[2] = {
#include "enemy/saru/saru_fidget_pos1.inc"
};

static NJS_MKEY_A saru_fidget_ang1[2] = {
#include "enemy/saru/saru_fidget_ang1.inc"
};

static NJS_MKEY_F saru_fidget_pos2[2] = {
#include "enemy/saru/saru_fidget_pos2.inc"
};

static NJS_MKEY_F saru_fidget_pos3[2] = {
#include "enemy/saru/saru_fidget_pos3.inc"
};

static NJS_MKEY_F saru_fidget_pos4[2] = {
#include "enemy/saru/saru_fidget_pos4.inc"
};

static NJS_MKEY_A saru_fidget_ang4[2] = {
#include "enemy/saru/saru_fidget_ang4.inc"
};

static NJS_MKEY_F saru_fidget_pos5[2] = {
#include "enemy/saru/saru_fidget_pos5.inc"
};

static NJS_MKEY_A saru_fidget_ang5[7] = {
#include "enemy/saru/saru_fidget_ang5.inc"
};

static NJS_MKEY_F saru_fidget_pos6[2] = {
#include "enemy/saru/saru_fidget_pos6.inc"
};

static NJS_MKEY_F saru_fidget_pos8[2] = {
#include "enemy/saru/saru_fidget_pos8.inc"
};

static NJS_MKEY_F saru_fidget_pos9[2] = {
#include "enemy/saru/saru_fidget_pos9.inc"
};

static NJS_MKEY_A saru_fidget_ang9[2] = {
#include "enemy/saru/saru_fidget_ang9.inc"
};

static NJS_MKEY_F saru_fidget_pos10[2] = {
#include "enemy/saru/saru_fidget_pos10.inc"
};

static NJS_MKEY_A saru_fidget_ang10[2] = {
#include "enemy/saru/saru_fidget_ang10.inc"
};

static NJS_MKEY_F saru_fidget_pos11[2] = {
#include "enemy/saru/saru_fidget_pos11.inc"
};

static NJS_MKEY_A saru_fidget_ang11[7] = {
#include "enemy/saru/saru_fidget_ang11.inc"
};

static NJS_MKEY_F saru_fidget_pos12[2] = {
#include "enemy/saru/saru_fidget_pos12.inc"
};

static NJS_MKEY_F saru_fidget_pos13[2] = {
#include "enemy/saru/saru_fidget_pos13.inc"
};

static NJS_MKEY_A saru_fidget_ang13[2] = {
#include "enemy/saru/saru_fidget_ang13.inc"
};

static NJS_MKEY_F saru_fidget_pos14[2] = {
#include "enemy/saru/saru_fidget_pos14.inc"
};

static NJS_MKEY_A saru_fidget_ang14[7] = {
#include "enemy/saru/saru_fidget_ang14.inc"
};

static NJS_MKEY_F saru_fidget_pos15[2] = {
#include "enemy/saru/saru_fidget_pos15.inc"
};

static NJS_MKEY_F saru_fidget_pos16[2] = {
#include "enemy/saru/saru_fidget_pos16.inc"
};

static NJS_MKEY_A saru_fidget_ang16[2] = {
#include "enemy/saru/saru_fidget_ang16.inc"
};

static NJS_MKEY_F saru_fidget_pos17[2] = {
#include "enemy/saru/saru_fidget_pos17.inc"
};

static NJS_MKEY_A saru_fidget_ang17[2] = {
#include "enemy/saru/saru_fidget_ang17.inc"
};

static NJS_MKEY_F saru_fidget_pos18[2] = {
#include "enemy/saru/saru_fidget_pos18.inc"
};

static NJS_MKEY_A saru_fidget_ang18[2] = {
#include "enemy/saru/saru_fidget_ang18.inc"
};

static NJS_MKEY_F saru_fidget_pos19[2] = {
#include "enemy/saru/saru_fidget_pos19.inc"
};

static NJS_MKEY_F saru_fidget_pos20[2] = {
#include "enemy/saru/saru_fidget_pos20.inc"
};

static NJS_MKEY_A saru_fidget_ang20[2] = {
#include "enemy/saru/saru_fidget_ang20.inc"
};

static NJS_MKEY_F saru_fidget_pos21[2] = {
#include "enemy/saru/saru_fidget_pos21.inc"
};

static NJS_MKEY_A saru_fidget_ang21[2] = {
#include "enemy/saru/saru_fidget_ang21.inc"
};

static NJS_MKEY_F saru_fidget_pos22[2] = {
#include "enemy/saru/saru_fidget_pos22.inc"
};

static NJS_MKEY_F saru_fidget_pos23[2] = {
#include "enemy/saru/saru_fidget_pos23.inc"
};

static NJS_MKEY_A saru_fidget_ang23[2] = {
#include "enemy/saru/saru_fidget_ang23.inc"
};

static NJS_MKEY_F saru_fidget_pos24[2] = {
#include "enemy/saru/saru_fidget_pos24.inc"
};

static NJS_MKEY_A saru_fidget_ang24[2] = {
#include "enemy/saru/saru_fidget_ang24.inc"
};

static NJS_MKEY_F saru_fidget_pos25[2] = {
#include "enemy/saru/saru_fidget_pos25.inc"
};

static NJS_MKEY_A saru_fidget_ang25[2] = {
#include "enemy/saru/saru_fidget_ang25.inc"
};

static NJS_MKEY_F saru_fidget_pos26[2] = {
#include "enemy/saru/saru_fidget_pos26.inc"
};

static NJS_MKEY_A saru_fidget_ang26[2] = {
#include "enemy/saru/saru_fidget_ang26.inc"
};

static NJS_MKEY_F saru_fidget_pos27[2] = {
#include "enemy/saru/saru_fidget_pos27.inc"
};

static NJS_MKEY_A saru_fidget_ang27[2] = {
#include "enemy/saru/saru_fidget_ang27.inc"
};

static NJS_MKEY_F saru_fidget_pos28[2] = {
#include "enemy/saru/saru_fidget_pos28.inc"
};

static NJS_MKEY_A saru_fidget_ang28[2] = {
#include "enemy/saru/saru_fidget_ang28.inc"
};

static NJS_MKEY_F saru_fidget_pos29[2] = {
#include "enemy/saru/saru_fidget_pos29.inc"
};

static NJS_MKEY_A saru_fidget_ang29[2] = {
#include "enemy/saru/saru_fidget_ang29.inc"
};

static NJS_MKEY_F saru_fidget_pos30[2] = {
#include "enemy/saru/saru_fidget_pos30.inc"
};

static NJS_MKEY_A saru_fidget_ang30[2] = {
#include "enemy/saru/saru_fidget_ang30.inc"
};

static NJS_MKEY_F saru_fidget_pos31[2] = {
#include "enemy/saru/saru_fidget_pos31.inc"
};

static NJS_MKEY_A saru_fidget_ang31[2] = {
#include "enemy/saru/saru_fidget_ang31.inc"
};

static NJS_MKEY_F saru_fidget_pos32[2] = {
#include "enemy/saru/saru_fidget_pos32.inc"
};

static NJS_MKEY_A saru_fidget_ang32[2] = {
#include "enemy/saru/saru_fidget_ang32.inc"
};

static NJS_MKEY_F saru_fidget_pos33[2] = {
#include "enemy/saru/saru_fidget_pos33.inc"
};

static NJS_MKEY_A saru_fidget_ang33[2] = {
#include "enemy/saru/saru_fidget_ang33.inc"
};

static NJS_MKEY_F saru_fidget_pos34[2] = {
#include "enemy/saru/saru_fidget_pos34.inc"
};

static NJS_MKEY_A saru_fidget_ang34[2] = {
#include "enemy/saru/saru_fidget_ang34.inc"
};

static NJS_MKEY_F saru_fidget_pos35[2] = {
#include "enemy/saru/saru_fidget_pos35.inc"
};

static NJS_MKEY_A saru_fidget_ang35[2] = {
#include "enemy/saru/saru_fidget_ang35.inc"
};

static NJS_MKEY_F saru_fidget_pos36[2] = {
#include "enemy/saru/saru_fidget_pos36.inc"
};

static NJS_MKEY_A saru_fidget_ang36[2] = {
#include "enemy/saru/saru_fidget_ang36.inc"
};

static NJS_MKEY_F saru_fidget_pos37[2] = {
#include "enemy/saru/saru_fidget_pos37.inc"
};

static NJS_MKEY_A saru_fidget_ang37[2] = {
#include "enemy/saru/saru_fidget_ang37.inc"
};

static NJS_MDATA2 saru_fidget_mdata[38] = {
#include "enemy/saru/saru_fidget_mdata.inc"
};

static NJS_MOTION saru_fidget_motion = {
#include "enemy/saru/saru_fidget_motion.inc"
};

NJS_ACTION saru_fidget_action = {
#include "enemy/saru/saru_fidget_action.inc"
};

static NJS_TEXNAME e_bomb_textures[2] = {{TEXT_13, 0, 0}, {TEXT_14, 0, 0}};

NJS_TEXLIST e_bomb_texlist = {
#include "enemy/saru/e_bomb_texlist.inc"
};

static NJS_MATERIAL saru_bomb_fuse_materials[1] = {
#include "enemy/saru/saru_bomb_fuse_materials.inc"
};

static Sint16 saru_bomb_fuse_meshes0[4] = {
#include "enemy/saru/saru_bomb_fuse_meshes0.inc"
};

static Sint16 saru_bomb_fuse_meshes1[24] = {
#include "enemy/saru/saru_bomb_fuse_meshes1.inc"
};

static NJS_COLOR saru_bomb_fuse_uvs0[3] = {
#include "enemy/saru/saru_bomb_fuse_uvs0.inc"
};

static NJS_COLOR saru_bomb_fuse_uvs1[24] = {
#include "enemy/saru/saru_bomb_fuse_uvs1.inc"
};

static NJS_MESHSET saru_bomb_fuse_meshsets[2] = {
#include "enemy/saru/saru_bomb_fuse_meshsets.inc"
};

static NJS_POINT3 saru_bomb_fuse_points[9] = {
#include "enemy/saru/saru_bomb_fuse_points.inc"
};

static NJS_VECTOR saru_bomb_fuse_normals[9] = {
#include "enemy/saru/saru_bomb_fuse_normals.inc"
};

static NJS_MODEL saru_bomb_fuse_model = {
#include "enemy/saru/saru_bomb_fuse_model.inc"
};

static NJS_OBJECT saru_bomb_fuse = {
#include "enemy/saru/saru_bomb_fuse.inc"
};

static NJS_MATERIAL saru_bomb_materials[1] = {
#include "enemy/saru/saru_bomb_materials.inc"
};

static Sint16 saru_bomb_meshes0[34] = {
#include "enemy/saru/saru_bomb_meshes0.inc"
};

static Sint16 saru_bomb_meshes1[64] = {
#include "enemy/saru/saru_bomb_meshes1.inc"
};

static NJS_COLOR saru_bomb_uvs0[28] = {
#include "enemy/saru/saru_bomb_uvs0.inc"
};

static NJS_COLOR saru_bomb_uvs1[64] = {
#include "enemy/saru/saru_bomb_uvs1.inc"
};

static NJS_MESHSET saru_bomb_meshsets[2] = {
#include "enemy/saru/saru_bomb_meshsets.inc"
};

static NJS_POINT3 saru_bomb_points[29] = {
#include "enemy/saru/saru_bomb_points.inc"
};

static NJS_VECTOR saru_bomb_normals[29] = {
#include "enemy/saru/saru_bomb_normals.inc"
};

static NJS_MODEL saru_bomb_model = {
#include "enemy/saru/saru_bomb_model.inc"
};

static NJS_OBJECT saru_bomb = {
#include "enemy/saru/saru_bomb.inc"
};

static NJS_TEXNAME togeball_textures[2] = {{TEXT_15, 0, 0}, {TEXT_16, 0, 0}};

NJS_TEXLIST togeball_togeball_texlist = {
#include "enemy/saru/togeball_togeball_texlist.inc"
};

static NJS_MATERIAL togeball_spikes_materials[1] = {
#include "enemy/saru/togeball_spikes_materials.inc"
};

static Sint16 togeball_spikes_meshes[126] = {
#include "enemy/saru/togeball_spikes_meshes.inc"
};

static NJS_COLOR togeball_spikes_uvs[105] = {
#include "enemy/saru/togeball_spikes_uvs.inc"
};

static NJS_MESHSET togeball_spikes_meshsets[1] = {
#include "enemy/saru/togeball_spikes_meshsets.inc"
};

static NJS_POINT3 togeball_spikes_points[84] = {
#include "enemy/saru/togeball_spikes_points.inc"
};

static NJS_VECTOR togeball_spikes_normals[84] = {
#include "enemy/saru/togeball_spikes_normals.inc"
};

static NJS_MODEL togeball_spikes_model = {
#include "enemy/saru/togeball_spikes_model.inc"
};

static NJS_OBJECT togeball_spikes = {
#include "enemy/saru/togeball_spikes.inc"
};

static NJS_MATERIAL togeball_materials[1] = {
#include "enemy/saru/togeball_materials.inc"
};

static Sint16 togeball_meshes[35] = {
#include "enemy/saru/togeball_meshes.inc"
};

static NJS_COLOR togeball_uvs[30] = {
#include "enemy/saru/togeball_uvs.inc"
};

static NJS_MESHSET togeball_meshsets[1] = {
#include "enemy/saru/togeball_meshsets.inc"
};

static NJS_POINT3 togeball_points[12] = {
#include "enemy/saru/togeball_points.inc"
};

static NJS_VECTOR togeball_normals[12] = {
#include "enemy/saru/togeball_normals.inc"
};

static NJS_MODEL togeball_model = {
#include "enemy/saru/togeball_model.inc"
};

NJS_OBJECT togeball_object = {
#include "enemy/saru/togeball_object.inc"
};

/* clang-format off */
static struct collision_shape saru_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
    PUSH_FIRM(PUSH_PLAYERS | PUSH_OBJECTS), {0.0F, 0.0F, 0.0F}, 4.5F, 0.0F,
    0.0F, {0}, 0, NULL};
/* clang-format on */

/* the bomb's shapes. collision_init takes the first three:
 * - a cylinder, on for a bomb until it goes off, and off for a ball;
 * - the blast, off until the bomb goes off;
 * - the cylinder an attack hurts, which sets the bomb off.
 * nothing reads the fourth */
/* clang-format off */
static struct collision_shape saru_bomb_shapes[4] = {
    {SHAPE_CYLINDER | SHAPE_OFF | SHAPE_NO_PUSH, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 7.2F, 3.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF,
     ATTACK(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
     SHAPE_HURTS_OBJECTS,
     PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | LOCKON_NO_NUMBER, SHAPE_HURTS_PLAYERS,
     PUSH_ALL, {0.0F, 0.0F, 0.0F}, 3.0F, 3.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER,
     ATTACK(2) | GUARD(1) | SHAPE_HURTS_PLAYERS, PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 0.0F, 0.0F}, 3.5F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static NJS_MODEL *saru_pieces[] = {
    &saru_foot_minus_z_model, &saru_foot_plus_z_model, &saru_hand_plus_z_model,
    &saru_hand_minus_z_model, &saru_ears_model,        &saru_ears_model,
    &saru_head_model,         &saru_body_model,        NULL,
};

static struct dust_params saru_dust = {
    0.13F,
    0.04F,
    0.23F,
    0.06F,
    0.01F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.5F, 0.9F, 0.9F, 1.0F},
};

static void saru_dust_ring(struct task_work *work, struct lockon_target *record)
{
    NJS_POINT3 offset = {1.0F, -2.0F, 0.0F};
    NJS_POINT3 pos;
    NJS_VECTOR speed = {0.5F, 0.0F, 0.0F};
    NJS_VECTOR out;
    Angle ang;
    int i;

    for (i = 0, ang = 0; i < 8; i++, ang += 0x2000) {
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, ang);
        njCalcVector(NULL, &speed, &out);
        njCalcPoint(NULL, &offset, &pos);
        njPopMatrix(1);
        saru_dust.speed = out;
        njAddVector(&pos, &work->pos);
        saru_dust.pos = pos;
        dust_create_from(&saru_dust);
    }
}

static void saru_bomb_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.speed.y += record->motion.accel.y - 0.06F;
    VECTOR_ADD(&work->pos, &record->motion.speed);
    VECTOR_CLEAR(&record->motion.accel);
}

static void saru_blast_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BLAST_INIT:
        work->timer = BLAST_TIME;
        BLAST(work)->scroll = 0;
        BLAST(work)->growth = 4.0F;
        work->scl.x = 2.0F;
        work->scl.z = 1.0F;
        blast_dome_init();
        work->mode = BLAST_GROW;
        break;
    case BLAST_GROW:
        if (--TASK_TIMER(work) == 0) {
            work->mode = BLAST_END;
            break;
        }
        if (TASK_TIMER(work) < BLAST_TIME / 2) {
            work->scl.z -= 0.033F;
        }
        BLAST(work)->growth += 0.1F;
        BLAST(work)->growth *= 0.8F;
        work->scl.x += BLAST(work)->growth;
        BLAST(work)->scroll += 10;
        break;
    case BLAST_END:
        task_kill(task);
        return;
    }
    njPushMatrix(NULL);
    blast_dome_scroll(BLAST(work)->scroll);
    blast_dome_blend(work->scl.z);
    blast_dome_set_alpha(work->scl.z);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njSetTexture(&blast_dome_texlist);
    blast_dome_draw();
    njPopMatrix(1);
    collision_register(work);
}

static void saru_blast_create(NJS_POINT3 *pos)
{
    struct task *task = task_create(TASK_WORK, 5, saru_blast_step);

    if (task != NULL) {
        struct task_work *work = task->work;

        work->pos = *pos;
    }
}

/* @hack spelling: the body a bare block, not a do-while (0), which lets
 * saru_bomb_touch and saru_bomb_lie end in a tail branch to
 * saru_blast_create (BRA at 0x8c1098a2, 0x8c109bc2); in a do-while, the
 * call is a BSR and 43 of 489 words differ */
#define BOMB_EXPLODE(work, record)                                             \
    {                                                                          \
        ENEMY_SET_MODE((work), (record), BOMB_BLAST);                          \
        (work)->scl.x = 5.0F;                                                  \
        (work)->scl.y = 0.0F;                                                  \
        explosion_create_at((work)->pos.x, (work)->pos.y + 3.0F,               \
                            (work)->pos.z, 4.0F);                              \
        (work)->collision->shapes[1].flags &= ~SHAPE_OFF;                      \
        (work)->collision->shapes[0].flags |= SHAPE_OFF;                       \
        saru_blast_create(&(work)->pos);                                       \
    }

/* draw the bomb or the ball, but only in view, on a frame's first step.
 * its scale springs and a bomb's fuse burns only then. the ground counts
 * only then too: a ball on it goes off, and a bomb that meets it faster
 * than 0.3 kicks up dust */
static void saru_bomb_draw(struct task_work *work, struct lockon_target *record)
{
    Float scale_x = work->scl.x;
    Float scale_y = work->scl.y;
    NJS_POINT3 pos;
    NJS_VECTOR speed;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    enemy_spring_scale(work, record);
    if (record->flags & ENEMY_ON_GROUND) {
        if (work->submode == SARU_BOMB) {
            if (njAbs(record->motion.speed.y) > 0.3F) {
                record->scale_speed = 0.4F;
                saru_dust_ring(work, record);
            }
        } else {
            BOMB_EXPLODE(work, record);
        }
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    if (work->submode == SARU_BOMB) {
        njSetTexture(&e_bomb_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, scale_x, scale_y, scale_x);
        draw_model(&saru_bomb_model);
        njTranslate(NULL, 0.0F, 2.3F, 0.0F);
        njScale(NULL, 1.0F, FUSE(record) * 1.25F, 1.0F);
        draw_model(&saru_bomb_fuse_model);
        njPopMatrix(1);
        FUSE(record) -= 0.003F;
        pos.x = work->pos.x;
        pos.y = work->pos.y + (FUSE(record) * 2.5F) + 2.3F;
        pos.z = work->pos.z;
        speed.x = (0.5F - (rand() * RAND_SCALE)) * 1.2F;
        speed.y = ((rand() * RAND_SCALE) + 0.2F) * 0.9F;
        speed.z = (0.5F - (rand() * RAND_SCALE)) * 1.2F;
        spark_create(&pos, &speed, 0.6F);
    } else {
        njSetTexture(&togeball_togeball_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, scale_x, scale_y, scale_x);
        draw_tree(&togeball_object);
        njPopMatrix(1);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* set the bomb off when an attack hurts it. a ball also goes off when it
 * touches anything or has CARRY_GROUND set. a bomb, ball or not, that a
 * player has held goes off when it touches an item or an enemy */
static void saru_bomb_touch(struct task_work *work,
                            struct lockon_target *record)
{
    struct collision_work *collision = work->collision;
    int flags = work->flags;
    int kind;

    /* @bug logic: these tests read submode as what was thrown, but a
     * lying bomb keeps its blink gap there, from BOMB_BLINK_GAP down (MOV.B
     * at 0x8c109930). while the gap is 1, which is SARU_TOGEBALL's value,
     * the lying bomb goes off as a ball would (CMP/EQ #1 at 0x8c1097ba,
     * 0x8c1097c6) */
    if (((flags & CARRY_GROUND) && work->submode == SARU_TOGEBALL) ||
        ((flags & COLLISION_TOUCHING) && work->submode == SARU_TOGEBALL) ||
        (flags & COLLISION_DAMAGED)) {
        BOMB_EXPLODE(work, record);
    }
    flags = work->flags;
    if ((flags & BOMB_WAS_HELD) && (flags & COLLISION_TOUCHING)) {
        kind = collision->hit->kind;
        if (kind == COLLISION_ITEM || kind == COLLISION_ENEMY) {
            BOMB_EXPLODE(work, record);
        }
    }
}

static void saru_bomb_fly(struct task_work *work, struct lockon_target *record)
{
    if (TASK_TIMER(work) > 20) {
        saru_bomb_touch(work, record);
    }
    saru_bomb_move(work, record);
    enemy_collide(work, record);
    saru_bomb_draw(work, record);
    if (TASK_TIMER(work) > 120) {
        ENEMY_SET_MODE(work, record, BOMB_LIE);
        work->submode = BOMB_BLINK_GAP;
    }
}

/* draw the lying bomb, and burn its fuse, which sparks. a lying ball draws
 * as a bomb too. a blink leaves the bomb undrawn for that step. the blink
 * sets a flash material, but the reset at the end clears it before
 * anything draws */
static void saru_bomb_draw_lying(struct task_work *work,
                                 struct lockon_target *record)
{
    NJS_POINT3 pos;
    NJS_VECTOR speed;

    njSetTexture(&e_bomb_texlist);
    if (work->tick > work->submode) {
        work->tick = 0;
        if (work->submode > 0) {
            work->submode--;
        }
        draw_set_material(0.5F, 2.0F, -2.0F, -2.0F);
    } else {
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_model(&saru_bomb_model);
        njTranslate(NULL, 0.0F, 2.3F, 0.0F);
        njScale(NULL, 1.0F, FUSE(record) * 1.25F, 1.0F);
        draw_model(&saru_bomb_fuse_model);
        njPopMatrix(1);
    }
    FUSE(record) -= 0.003F;
    if (FUSE(record) < 0.0F) {
        FUSE(record) = 0.0F;
    }
    pos.x = work->pos.x;
    pos.y = work->pos.y + (FUSE(record) * 2.5F) + 2.3F;
    pos.z = work->pos.z;
    speed.x = (0.5F - (rand() * RAND_SCALE)) * 1.2F;
    speed.y = ((rand() * RAND_SCALE) + 0.2F) * 0.9F;
    speed.z = (0.5F - (rand() * RAND_SCALE)) * 1.2F;
    spark_create(&pos, &speed, 0.6F);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void saru_bomb_lie(struct task_work *work, struct lockon_target *record)
{
    saru_bomb_touch(work, record);
    /* @hack spelling: the held test nested in the motion test, which loads
     * enemy_collide's address into R3 (0x8c109b54); joined by &&, R2, and 2
     * of 87 words differ */
    if (njScalor(&record->motion.speed) >= 0.05F ||
        !(record->flags & ENEMY_ON_GROUND)) {
        if (!(work->flags & CARRY_HELD)) {
            saru_bomb_move(work, record);
            enemy_collide(work, record);
        }
    }
    saru_bomb_draw_lying(work, record);
    if (TASK_TIMER(work) > 150) {
        BOMB_EXPLODE(work, record);
    }
}

/* the bomb's blast shape grows by 1 a step to a radius of 25, and holds
 * there until the bomb ends. the red and yellow flash shows nothing: its
 * material is set and reset with nothing drawn between */
static void saru_bomb_blast(struct task_work *work,
                            struct lockon_target *record)
{
    struct collision_shape *shape = &work->collision->shapes[1];

    if (TASK_TIMER(work) % 2) {
        draw_set_material(work->scl.y, 1.0F, 0.0F, 0.0F);
    } else {
        draw_set_material(work->scl.y, 0.5F, 0.5F, 0.0F);
    }
    if (work->scl.x < 25.0F) {
        work->scl.x += 1.0F;
        shape->radius += 1.0F;
    } else {
        work->scl.y -= 0.05F;
        if (work->scl.y < -1.0F) {
            ENEMY_SET_MODE(work, record, BOMB_END);
        }
    }
    work->ang.y += 0x100;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void saru_bomb_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

/* the bomb's step. when its blast hits a player, it marks the saru that
 * threw it, its parent, with SARU_HIT_PLAYER and SARU_HIT_NEW */
static void saru_bomb_step(struct task *task)
{
    /* @hack redundant: register on work and record, which puts task in R12
     * and record in R13 (MOV R4,R12 at 0x8c109cfc, MOV.L @(36,R12),R13 at
     * 0x8c109d08); as plain locals, the two trade and 13 of 90 words
     * differ */
    register struct task_work *work = task->work;
    register struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode == 0) {
        switch (work->mode) {
        case BOMB_FLY:
            saru_bomb_fly(work, record);
            break;
        case BOMB_LIE:
            saru_bomb_lie(work, record);
            break;
        case BOMB_BLAST:
            saru_bomb_blast(work, record);
            /* @hack spelling: the kind's test nested in the hit's, which
             * reads work->collision and its hit again (0x8c109d6e,
             * 0x8c109d70); joined by &&, SHC reads them once and 60 of 90
             * words differ */
            if (work->collision->hit != NULL) {
                if (work->collision->hit->kind == COLLISION_PLAYER) {
                    task->parent->work->flags |= SARU_HIT_PLAYER | SARU_HIT_NEW;
                    VARS(task->parent->work)->since_hit = 0;
                }
            }
            break;
        case BOMB_END:
            enemy_end(task, work, record);
            task_kill(task);
            break;
        /* @hack redundant: an empty default, which loads enemy_save_pos's
         * address into R2 (0x8c109d9e); without it, R3, and 2 of 90 words
         * differ */
        default:
            break;
        }
        enemy_save_pos(work, record);
        if (!(work->flags & CARRY_HELD)) {
            collision_register(work);
            work->timer++;
            work->tick++;
        } else {
            work->flags |= BOMB_WAS_HELD;
        }
    }
}

static void saru_bomb_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    NJS_VECTOR speed;

    collision_init(task, saru_bomb_shapes, COLLISION_ENEMY, 1, 3);
    /* @hack spelling: work->collision reached through task->work, not work,
     * which reloads task->work for each of the three (MOV.L @(32,R12),R3 at
     * 0x8c109e22, 0x8c109e36, 0x8c109e44); through work, the function runs
     * 26 bytes long and 185 of 204 words differ */
    task->work->collision->flags &= ~COLLISION_TARGET;
    if (work->submode == SARU_BOMB) {
        task->work->collision->shapes[0].flags &= ~SHAPE_OFF;
    }
    task->work->collision->radius = 40.0F;
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_SHADOW;
    speed.x = njSqrt(enemy_player_distance2(work, 0)) * 0.013F;
    speed.y = 2.0F;
    speed.z = 0.0F;
    if (speed.x > 1.2F) {
        speed.x = 1.2F;
    }
    carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
    work->flags &= ~BOMB_WAS_HELD;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, work->ang.x + work->ang.y);
    njCalcVector(NULL, &speed, &record->motion.speed);
    njPopMatrix(1);
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 0.0F;
    work->tick = 0;
    work->timer = 0;
    /* @hack redundant: the fuse set to 0, which the 1.0F below replaces
     * before any read. it keeps the image's store of FR15 (0x8c109ef2);
     * without it, the function runs 28 bytes long and 78 of 204 words
     * differ */
    FUSE(record) = 0.0F;
    record->turn_speed = 0xe0;
    work->ang.x = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    work->ang.z = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    VECTOR_CLEAR(&record->probe_offset);
    record->top = 3.0F;
    record->radius = 3.0F;
    record->bottom = -3.0F;
    record->wall_bounce = 0.7F;
    record->floor_bounce = 0.68F;
    record->friction = 0.5F;
    record->unk_c4 = 0.7F;
    record->shadow_size = 0.7F;
    record->shadow_ratio = 1.0F;
    FUSE(record) = 1.0F;
    enemy_set_home(work, record);
    task->destructor = saru_bomb_destroy;
    task->step = saru_bomb_step;
}

/* put the saru on its circle: scl.z from home, at the angle ang.x round
 * it. its height stays as it is */
static void saru_place(struct task_work *work, struct lockon_target *record)
{
    NJS_POINT3 offset;

    offset.x = work->scl.z;
    offset.y = work->pos.y;
    offset.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, work->ang.x);
    njCalcPoint(NULL, &offset, &work->pos);
    njPopMatrix(1);
    work->pos.x += record->home.x;
    work->pos.z += record->home.z;
}

static void saru_to_ground(struct task_work *work, struct lockon_target *record)
{
    if (work->flags & SARU_TO_GROUND) {
        work->pos.y = shadow_draw(work, 1.0F) + 4.0F;
    }
}

void saru_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    collision_init(task, &saru_shape, COLLISION_ENEMY, 1, 1);
    record = enemy_alloc_record(task, work);
    enemy_set_home(work, record);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    record->flags &= ~ENEMY_BLENDING;
    work->tick = rand() * RAND_SCALE * 20.0F;
    work->timer = 0;
    record->motion.slip = 0.0F;
    record->turn_speed = 0xe0;
    record->aim_y = work->ang.y;
    record->frame = 3.0F;
    record->next_frame = 0.0F;
    record->action = &saru_hop_action;
    record->next_action = record->action;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    record->motion.gravity = 0.1F;
    HELD_SCALE(record) = 0.0F;
    record->scale_speed = 0.0F;
    if (work->scl.z <= 1.0F) {
        work->scl.z = 1.0F;
    }
    if (work->scl.x >= 0.0F) {
        work->flags &= ~SARU_TO_GROUND;
    } else {
        work->flags |= SARU_TO_GROUND;
    }
    if (work->scl.y >= 0.0F) {
        work->submode = SARU_BOMB;
        if (work->scl.y >= 1000.0F) {
            work->submode = SARU_UNARMED;
        }
    } else {
        work->submode = SARU_TOGEBALL;
    }
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    PIECE(record) = (Uint8)work->ang.z;
    WEAKNESS(work) = (work->ang.z & 0xff00) >> 8;
    saru_place(work, record);
    task->destructor = saru_destroy;
    task->step = saru_step;
    task->draw = saru_draw;
    work->flags |= SARU_SOLID;
    work->flags &= ~(SARU_HOLDING | SARU_HIT_PLAYER | SARU_HIT_NEW);
    knuckles_find_piece(PIECE(record), &work->pos);
}

static void saru_take_hit(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *collision;

    /* @hack redundant: player_work handed to enemy_ignore, which keeps its
     * load into R4 (0x8c10a2a4); without it, 24 of 27 words differ */
    enemy_ignore((void *)player_work);
    /* @hack redundant: hit_shape read and dropped, which keeps its MOV.W
     * (0x8c10a2ae); without it, 24 of 27 words differ */
    (void)work->collision->hit_shape;
    collision = work->collision;
    /* @hack redundant: the hit handed to enemy_ignore, which keeps its load
     * into R4 in the delay slot of the COLLISION_HIT test's branch
     * (0x8c10a2b6); without it, 26 of 27 words differ */
    enemy_ignore(collision->hit);
    if (!(collision->flags & COLLISION_HIT)) {
        return;
    }
    if (!enemy_take_hit(work, record)) {
        return;
    }
    work->mode = SARU_BREAK;
    ENEMY_MARK_BEATEN(work);
}

/* draw the record's action, or while it blends, the blend, which
 * BLEND_ADVANCE then moves on */
#define DRAW_ACTION(record)                                                    \
    do {                                                                       \
        if ((record)->flags & ENEMY_BLENDING) {                                \
            NJS_MOTION_LINK link;                                              \
            NJS_ACTION_LINK action;                                            \
                                                                               \
            action.motionlink = &link;                                         \
            action.object = (record)->action->object;                          \
            link.motion[0] = (record)->action->motion;                         \
            link.motion[1] = (record)->next_action->motion;                    \
            link.frame[0] = (record)->frame;                                   \
            link.frame[1] = (record)->next_frame;                              \
            draw_action_link(&action, (record)->blend);                        \
            BLEND_ADVANCE(record);                                             \
        } else {                                                               \
            draw_action((record)->action, (record)->frame);                    \
        }                                                                      \
    } while (0)

/* the saru as it stands, which task_draw_all draws while the game is
 * paused. it advances nothing */
static void saru_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);
    Float scale_x = work->scl.x * 1.2F;
    Float scale_y = work->scl.y * 1.2F;

    if (record == NULL) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&e_saru_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.x + work->ang.y);
    njScale(NULL, scale_x, scale_y, scale_x);
    njRotateY(NULL, 0x8000);
    if (record->flags & ENEMY_BLENDING) {
        NJS_MOTION_LINK link;
        NJS_ACTION_LINK action;

        action.motionlink = &link;
        action.object = record->action->object;
        link.motion[0] = record->action->motion;
        link.motion[1] = record->next_action->motion;
        link.frame[0] = record->frame;
        link.frame[1] = record->next_frame;
        draw_action_link(&action, record->blend);
    } else {
        draw_action(record->action, record->frame);
    }
    njPopMatrix(1);
}

/* draw the saru standing, but only in view, on a frame's first step. only
 * then does it keep to the ground, spring its scale and advance its blend */
static void saru_draw_standing(struct task_work *work,
                               struct lockon_target *record)
{
    Float scale_x = work->scl.x * 1.2F;
    Float scale_y = work->scl.y * 1.2F;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    saru_to_ground(work, record);
    enemy_spring_scale(work, record);
    njSetTexture(&e_saru_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.x + work->ang.y);
    njScale(NULL, scale_x, scale_y, scale_x);
    njRotateY(NULL, 0x8000);
    DRAW_ACTION(record);
    njPopMatrix(1);
}

static void saru_draw_walking(struct task_work *work,
                              struct lockon_target *record)
{
    Float scale_x = work->scl.x * 1.2F;
    Float scale_y = work->scl.y * 1.2F;

    saru_to_ground(work, record);
    enemy_spring_scale(work, record);
    njSetTexture(&e_saru_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, 0.0F, LIFT(record) - 1.5F, 0.0F);
    TURN(njRotateY, work->ang.x + work->ang.y);
    njScale(NULL, scale_x, scale_y, scale_x);
    njRotateY(NULL, 0x8000);
    DRAW_ACTION(record);
    njPopMatrix(1);
}

/* draw the saru throwing. while it holds what it throws, that grows in its
 * hand, and past frame 9.5 of the throw it lets go of a new bomb */
static void saru_draw_throwing(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    Float scale_x = work->scl.x * 1.2F;
    Float scale_y = work->scl.y * 1.2F;
    NJS_POINT3 hand;

    enemy_spring_scale(work, record);
    njSetTexture(&e_saru_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    {
        NJS_MATRIX matrix;
        NJS_POINT3 offset = {0.0F, 0.0F, 0.0F};

        njPushMatrix(&_unit_matrix_);
        njScale(NULL, scale_x, scale_y, scale_x);
        pose_action(NULL, record->action, record->frame, SARU_HAND, &matrix);
        njCalcPoint(&matrix, &offset, &hand);
        njPopMatrix(1);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.x + work->ang.y);
    if (record->flags & ENEMY_BLENDING) {
        NJS_MOTION_LINK link;
        NJS_ACTION_LINK action;

        action.motionlink = &link;
        action.object = record->action->object;
        link.motion[0] = record->action->motion;
        link.motion[1] = record->next_action->motion;
        link.frame[0] = record->frame;
        link.frame[1] = record->next_frame;
        njScale(NULL, scale_x, scale_y, scale_x);
        njRotateY(NULL, 0x8000);
        draw_action_link(&action, record->blend);
        BLEND_ADVANCE(record);
    } else {
        njPushMatrix(NULL);
        njScale(NULL, scale_x, scale_y, scale_x);
        njRotateY(NULL, 0x8000);
        draw_action(record->action, record->frame);
        njPopMatrix(1);
        njRotateY(NULL, 0x8000);
        if (work->flags & SARU_HOLDING) {
            Float scale;

            HELD_SCALE(record) += 0.1F;
            if (HELD_SCALE(record) > 1.0F) {
                HELD_SCALE(record) = 1.0F;
            }
            scale = HELD_SCALE(record);
            njTranslateV(NULL, &hand);
            /* @hack redundant: the rise, its scale, times 1.0F, which keeps
             * the image's FMUL by the 1 in FR13 (0x8c10aa3c); without it,
             * FR12 and FR13 trade and 30 of 307 words differ */
            njTranslate(NULL, 0.0F, scale * 1.0F, 0.0F);
            njScale(NULL, scale, scale, scale);
            if (work->submode == SARU_BOMB) {
                njSetTexture(&e_bomb_texlist);
                draw_tree(&saru_bomb);
            } else {
                njSetTexture(&togeball_togeball_texlist);
                draw_tree(&togeball_object);
            }
            if (record->frame > 9.5F) {
                struct task *bomb;
                /* @hack redundant: a zero point set and never read, whose
                 * 12-byte template SHC copies into the frame (0x8c10aaba to
                 * 0x8c10aacc); without it, the constants lose the template,
                 * the function runs 50 bytes short and 114 of 307 words
                 * differ */
                NJS_POINT3 unused = {0.0F, 0.0F, 0.0F};

                work->flags &= ~SARU_HOLDING;
                HELD_SCALE(record) = 0.0F;
                bomb = task_create_child(TASK_WORK, saru_bomb_start, task);
                if (bomb != NULL) {
                    struct task_work *bomb_work = bomb->work;

                    bomb_work->submode = work->submode;
                    bomb_work->pos = work->pos;
                    bomb_work->ang = work->ang;
                }
            }
        }
    }
    njPopMatrix(1);
}

static void saru_edit(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 radius;
    NJS_POINT3 from;
    NJS_POINT3 to;
    NJS_POINT3 pos;
    int i;

    if (placement_editing(task)) {
        if (work->tick > 25) {
            njPrintC(NJM_LOCATION(8, 8), "< SARU >");
        } else {
            njPrintC(NJM_LOCATION(8, 8), "<      >");
            njPrintC(NJM_LOCATION(6, 9), "sclY >= 1000.0 DE BOMB NAGEMASEN");
        }
        if (work->tick++ > 50) {
            work->tick = 0;
        }
        njPrintC(NJM_LOCATION(17, 16), "-> POSITION    ");
        njPrintC(NJM_LOCATION(17, 17), "->             ");
        njPrintC(NJM_LOCATION(17, 18), "-> 00STRONG FFWEAK&EME");
        njPrintC(NJM_LOCATION(17, 19), "-> [+ NOCOLLI][- COLLI]");
        njPrintC(NJM_LOCATION(17, 20), "-> [+ BOMB][- TOGE]");
        njPrintC(NJM_LOCATION(17, 21), "-> RADIUS      ");
        radius.x = work->scl.z;
        radius.y = 0.0F;
        radius.z = 0.0F;
        for (i = 0; i < ENEMY_RANGE_SEGMENTS; i++) {
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, i * ENEMY_RANGE_TURN);
            njCalcPoint(NULL, &radius, &from);
            njPopMatrix(1);
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, (i + 1) * ENEMY_RANGE_TURN);
            njCalcPoint(NULL, &radius, &to);
            njPopMatrix(1);
            draw_line_v(&from, &to);
        }
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.x);
        njCalcPoint(NULL, &radius, &pos);
        njPopMatrix(1);
        njSetTexture(&e_saru_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &pos);
        TURN(njRotateY, work->ang.x + work->ang.y);
        njRotateY(NULL, 0x8000);
        njScale(NULL, 1.2F, 1.2F, 1.2F);
        draw_tree(&saru_object);
        njPopMatrix(1);
    } else {
        struct lockon_target *record = LOCKON_TARGET(task);

        saru_draw_standing(work, record);
    }
}

static void saru_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void saru_break(struct task_work *work, struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_break(work, saru_pieces, &e_saru_texlist);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    animal_create_random(3, work->pos.x, work->pos.y + 15.0F, work->pos.z);
}

/* set the heading the saru walks toward, an angle round its circle. with
 * player 0 more than scl.z + 140 from home, the heading is random. within
 * scl.z + 20, it is the circle's point furthest from player 0, and between
 * the two, the nearest */
static void saru_choose_heading(struct task_work *work,
                                struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR way;
    Float distance;
    Float range;

    way = player->pos;
    njSubVector(&way, &record->home);
    distance = (way.x * way.x) + (way.y * way.y) + (way.z * way.z);
    range = work->scl.z + 140.0F;
    if (distance > range * range) {
        record->heading =
            (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    } else {
        range = work->scl.z + 20.0F;
        if (distance > range * range) {
            record->heading = (Angle)(atan2f(player->pos.x - record->home.x,
                                             player->pos.z - record->home.z) *
                                      ANGLE_PER_RADIAN) -
                              0x4000;
        } else {
            record->heading = (Angle)(atan2f(player->pos.x - record->home.x,
                                             player->pos.z - record->home.z) *
                                      ANGLE_PER_RADIAN) +
                              0x4000;
        }
    }
}

/* the saru's reaction to player 0. within 25 of it, the saru picks a
 * heading and hops round to walk to it. further off, while SARU_HIT_PLAYER
 * and SARU_HIT_NEW are both set, it hops to face player 0 and clears
 * SARU_HIT_NEW. either hop returns TRUE. otherwise saru_react returns
 * FALSE, though within 200 it may first start a hop to face player 0. it
 * starts that hop once its timer passes its weakness, and the caller can
 * replace it with a mode of its own */
static Bool saru_react(struct task_work *work, struct lockon_target *record)
{
    Float distance = enemy_player_distance2(work, 0);
    /* @hack spelling: the hop action and ENEMY_BLENDING through locals,
     * which SHC loads once into R10 and R11 (0x8c10b0f2, 0x8c10b0f6);
     * written in place, the hop action loads from the pool at each use, the
     * frame shrinks by 4 bytes and 104 of 131 words differ */
    NJS_ACTION *hop = &saru_hop_action;
    int blending = ENEMY_BLENDING;

    if (distance < 25.0F * 25.0F) {
        saru_choose_heading(work, record);
        if (rand() * RAND_SCALE > 0.5F) {
            ENEMY_SET_MODE(work, record, SARU_TURN_ON);
        } else {
            ENEMY_SET_MODE(work, record, SARU_TURN_BACK);
        }
        record->flags |= blending;
        record->blend_speed = 0.5F;
        record->next_frame = 0.0F;
        record->next_action = hop;
        return TRUE;
    } else if (distance < 200.0F * 200.0F &&
               TASK_TIMER(work) > (Uint16)WEAKNESS(work)) {
        ENEMY_SET_MODE(work, record, SARU_FACE);
        record->flags |= blending;
        record->blend_speed = 0.1F;
        record->next_frame = 0.0F;
        record->next_action = hop;
    }
    if ((work->flags & SARU_HIT_PLAYER) && (work->flags & SARU_HIT_NEW)) {
        ENEMY_SET_MODE(work, record, SARU_FACE);
        record->flags |= blending;
        record->blend_speed = 0.1F;
        record->next_frame = 0.0F;
        record->next_action = hop;
        work->flags &= ~SARU_HIT_NEW;
        return TRUE;
    }
    return FALSE;
}

/* the saru waits crouched, swaying, until its timer passes its tick. then
 * it keeps its place with a chance of its weakness times 0.003. keeping
 * it, the saru cheers if its bomb has hit a player, and hops or fidgets if
 * not. otherwise it turns to walk on or back, at random */
static void saru_wait(struct task_work *work, struct lockon_target *record)
{
    Float chance = (Uint8)WEAKNESS(work) * 0.003F;
    Float roll;

    saru_take_hit(work, record);
    roll = rand() * RAND_SCALE;
    saru_draw_standing(work, record);
    if (saru_react(work, record)) {
        return;
    }
    if (!(record->flags & ENEMY_BLENDING)) {
        record->frame = (njSin(SWAY(record)) * 0.5F) + 3.0F;
        SWAY(record) += 0x600;
    }
    if (TASK_TIMER(work) > work->tick) {
        SWAY(record) = 0;
        if (roll < chance) {
            if (work->flags & SARU_HIT_PLAYER) {
                ENEMY_SET_MODE(work, record, SARU_CHEER);
                record->flags |= ENEMY_BLENDING;
                record->blend_speed = 0.085F;
                record->next_frame = 0.0F;
                sound_play(SE_CHEER, 0, 0, 0);
                if (rand() * RAND_SCALE > 0.5F) {
                    record->next_action = &saru_jump_action;
                } else {
                    record->next_action = &saru_flip_action;
                }
                return;
            }
            if (rand() * RAND_SCALE > 0.5F) {
                ENEMY_SET_MODE(work, record, SARU_HOP);
                BLEND_TO(record, &saru_hop_action, 0.3F, 0.0F);
            } else {
                ENEMY_SET_MODE(work, record, SARU_FIDGET);
                BLEND_TO(record, &saru_fidget_action, 0.085F, 0.0F);
            }
        } else if (roll < (chance * 0.5F) + 0.5F) {
            saru_choose_heading(work, record);
            ENEMY_SET_MODE(work, record, SARU_TURN_BACK);
        } else {
            saru_choose_heading(work, record);
            ENEMY_SET_MODE(work, record, SARU_TURN_ON);
        }
    }
}

static void saru_hop(struct task_work *work, struct lockon_target *record)
{
    saru_take_hit(work, record);
    saru_draw_standing(work, record);
    if (saru_react(work, record)) {
        return;
    }
    if (!(record->flags & ENEMY_BLENDING)) {
        record->frame += 0.45F;
        if (record->frame >= (Float)record->action->motion->nbFrame) {
            record->frame = (Float)record->action->motion->nbFrame - 0.1F;
            saru_dust_ring(work, record);
            work->tick = (rand() * RAND_SCALE * 30.0F) + 10.0F;
            ENEMY_SET_MODE(work, record, SARU_WAIT);
            BLEND_TO(record, &saru_hop_action, 0.1F, 3.0F);
        }
    }
}

static void saru_fidget(struct task_work *work, struct lockon_target *record)
{
    saru_take_hit(work, record);
    saru_draw_standing(work, record);
    if (!(record->flags & ENEMY_BLENDING)) {
        record->frame += 0.28F;
        if (record->frame >= (Float)record->action->motion->nbFrame) {
            record->frame = (Float)record->action->motion->nbFrame - 0.1F;
            work->tick = (rand() * RAND_SCALE * 40.0F) + 10.0F;
            ENEMY_SET_MODE(work, record, SARU_WAIT);
            BLEND_TO(record, &saru_hop_action, 0.085F, 3.0F);
        }
    }
}

static void saru_turn_on(struct task_work *work, struct lockon_target *record)
{
    saru_take_hit(work, record);
    saru_draw_standing(work, record);
    if (record->flags & ENEMY_BLENDING) {
        return;
    }
    record->frame += 0.45F;
    work->ang.y = angle_approach(work->ang.y, 0x4000, 0x400);
    if (record->frame >= (Float)record->action->motion->nbFrame) {
        record->frame = (Float)record->action->motion->nbFrame - 0.1F;
        saru_dust_ring(work, record);
        work->tick = (rand() * RAND_SCALE * 30.0F) + 10.0F;
        ENEMY_SET_MODE(work, record, SARU_WALK_ON);
        BLEND_TO(record, &saru_walk_action, 0.3F, 0.0F);
    }
}

static void saru_turn_back(struct task_work *work, struct lockon_target *record)
{
    saru_take_hit(work, record);
    saru_draw_standing(work, record);
    if (record->flags & ENEMY_BLENDING) {
        return;
    }
    record->frame += 0.45F;
    work->ang.y = angle_approach(work->ang.y, 0xc000, 0x400);
    if (record->frame >= (Float)record->action->motion->nbFrame) {
        record->frame = (Float)record->action->motion->nbFrame - 0.1F;
        saru_dust_ring(work, record);
        work->tick = (rand() * RAND_SCALE * 30.0F) + 10.0F;
        ENEMY_SET_MODE(work, record, SARU_WALK_BACK);
        BLEND_TO(record, &saru_walk_action, 0.3F, 0.0F);
    }
}

static void saru_face(struct task_work *work, struct lockon_target *record)
{
    saru_take_hit(work, record);
    enemy_aim_at_player(work, record, 0);
    if (!(record->flags & ENEMY_BLENDING)) {
        record->frame += 0.45F;
        work->ang.y =
            angle_approach(work->ang.y, record->aim_y - work->ang.x, 0x400);
        if (record->frame >= (Float)record->action->motion->nbFrame) {
            record->frame = (Float)record->action->motion->nbFrame - 0.1F;
            saru_dust_ring(work, record);
            if (work->flags & SARU_HIT_PLAYER) {
                ENEMY_SET_MODE(work, record, SARU_CHEER);
                record->flags |= ENEMY_BLENDING;
                record->blend_speed = 0.085F;
                record->next_frame = 0.0F;
                if (rand() * RAND_SCALE > 0.5F) {
                    record->next_action = &saru_jump_action;
                } else {
                    record->next_action = &saru_flip_action;
                }
            } else if (enemy_player_distance2(work, 0) < 150.0F * 150.0F &&
                       njAbs(player_work[0]->pos.y - work->pos.y) < 60.0F &&
                       work->submode != SARU_UNARMED) {
                ENEMY_SET_MODE(work, record, SARU_THROW);
                BLEND_TO(record, &saru_throw_action, 0.085F, 0.0F);
                work->flags |= SARU_HOLDING;
            } else if (rand() * RAND_SCALE > 0.5F) {
                ENEMY_SET_MODE(work, record, SARU_HOP);
                BLEND_TO(record, &saru_hop_action, 0.3F, 0.0F);
            } else {
                ENEMY_SET_MODE(work, record, SARU_FIDGET);
                BLEND_TO(record, &saru_fidget_action, 0.085F, 0.0F);
            }
        }
    }
    saru_draw_standing(work, record);
}

static void saru_walk_on(struct task_work *work, struct lockon_target *record)
{
    saru_take_hit(work, record);
    saru_draw_walking(work, record);
    if (!(record->flags & ENEMY_BLENDING)) {
        record->frame += 0.8F;
        work->ang.x += (Angle)(1.0F / (work->scl.z + 3.0F) * ANGLE_PER_RADIAN);
        saru_place(work, record);
        LIFT(record) = njAbs(njSin(record->phase)) * 6.0F;
        record->phase += 0x670;
        if (record->frame >= (Float)record->action->motion->nbFrame) {
            record->frame -= (Float)record->action->motion->nbFrame;
            record->phase = 0;
            saru_dust_ring(work, record);
            if (saru_react(work, record)) {
                return;
            }
            if (TASK_TIMER(work) > 120 &&
                enemy_player_distance2(work, 0) < 25.0F * 25.0F) {
                saru_choose_heading(work, record);
                ENEMY_SET_MODE(work, record, SARU_TURN_BACK);
                BLEND_TO(record, &saru_hop_action, 0.4F, 0.0F);
            } else if (TASK_TIMER(work) > (Uint16)WEAKNESS(work) ||
                       angle_distance(record->heading, work->ang.x) < 0x1000) {
                work->tick = (rand() * RAND_SCALE * 20.0F) + 10.0F;
                ENEMY_SET_MODE(work, record, SARU_FACE);
                BLEND_TO(record, &saru_hop_action, 0.1F, 0.0F);
            }
        }
    }
}

static void saru_walk_back(struct task_work *work, struct lockon_target *record)
{
    saru_take_hit(work, record);
    saru_draw_walking(work, record);
    if (!(record->flags & ENEMY_BLENDING)) {
        record->frame += 1.2F;
        work->ang.x -= (Angle)(1.0F / (work->scl.z + 3.0F) * ANGLE_PER_RADIAN);
        saru_place(work, record);
        LIFT(record) = njAbs(njSin(record->phase)) * 7.0F;
        record->phase += 0x9a8;
        if (record->frame >= (Float)record->action->motion->nbFrame) {
            record->frame -= (Float)record->action->motion->nbFrame;
            record->phase = 0;
            saru_dust_ring(work, record);
            sound_play_at(SE_LAND, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            if (saru_react(work, record)) {
                return;
            }
            if (TASK_TIMER(work) > 120 &&
                enemy_player_distance2(work, 0) < 25.0F * 25.0F) {
                saru_choose_heading(work, record);
                ENEMY_SET_MODE(work, record, SARU_TURN_ON);
                BLEND_TO(record, &saru_hop_action, 0.4F, 0.0F);
            } else if (TASK_TIMER(work) > (Uint16)WEAKNESS(work) ||
                       angle_distance(record->heading, work->ang.x) < 0x1000) {
                work->tick = (rand() * RAND_SCALE * 20.0F) + 10.0F;
                ENEMY_SET_MODE(work, record, SARU_FACE);
                BLEND_TO(record, &saru_hop_action, 0.1F, 0.0F);
            }
        }
    }
}

static void saru_cheer(struct task_work *work, struct lockon_target *record)
{
    saru_take_hit(work, record);
    saru_draw_standing(work, record);
    if (!(record->flags & ENEMY_BLENDING)) {
        record->frame += 0.4F;
        if (record->frame >= (Float)record->action->motion->nbFrame) {
            saru_dust_ring(work, record);
            if (rand() * RAND_SCALE < 0.5F) {
                record->frame = (Float)record->action->motion->nbFrame - 0.1F;
                work->tick = (rand() * RAND_SCALE * 10.0F) + 10.0F;
                ENEMY_SET_MODE(work, record, SARU_WAIT);
                BLEND_TO(record, &saru_hop_action, 0.1F, 3.0F);
            } else {
                sound_play(SE_CHEER, 0, 0, 0);
                record->frame -= (Float)record->action->motion->nbFrame;
            }
        }
    }
}

static void saru_throw(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;

    saru_take_hit(work, record);
    saru_draw_throwing(task, record);
    if (!(record->flags & ENEMY_BLENDING)) {
        record->frame += 0.3F;
        if (record->frame >= (Float)record->action->motion->nbFrame) {
            record->frame = (Float)record->action->motion->nbFrame - 2.0F;
            work->tick = WEAKNESS(work);
            ENEMY_SET_MODE(work, record, SARU_WAIT);
            BLEND_TO(record, &saru_hop_action, 0.07F, 3.0F);
            sound_play_at(SE_THROW, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
    }
}

static void saru_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        saru_edit(task);
        return;
    }
    if (placement_try_leave_by(task, SARU_LEAVE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case SARU_WAIT:
        saru_wait(work, record);
        break;
    case SARU_HOP:
        saru_hop(work, record);
        break;
    case SARU_FIDGET:
        saru_fidget(work, record);
        break;
    case SARU_TURN_ON:
        saru_turn_on(work, record);
        break;
    case SARU_WALK_ON:
        saru_walk_on(work, record);
        break;
    case SARU_TURN_BACK:
        saru_turn_back(work, record);
        break;
    case SARU_WALK_BACK:
        saru_walk_back(work, record);
        break;
    case SARU_FACE:
        saru_face(work, record);
        break;
    /* @hack spelling: SARU_CHEER's case ahead of SARU_THROW's, which writes
     * its call first (BSR at 0x8c10c3e2); in the modes' order, 4 of 152
     * words differ */
    case SARU_CHEER:
        saru_cheer(work, record);
        break;
    case SARU_THROW:
        saru_throw(task, record);
        break;
    case SARU_BREAK:
        knuckles_take_piece(PIECE(record), &work->pos);
        saru_break(work, record);
        /* @hack spelling: the two calls in one statement, which keeps
         * placement_kill a JSR ahead of the return (0x8c10c414); as two, it
         * becomes a tail jump and 5 of 152 words differ */
        enemy_end(task, work, record), placement_kill(task);
        return;
    /* @hack redundant: an empty default, which loads enemy_save_pos's address
     * into R2 (0x8c10c440); without it, R3, and 2 of 152 words differ */
    default:
        break;
    }
    enemy_save_pos(work, record);
    knuckles_find_piece(PIECE(record), &work->pos);
    work->timer++;
    if (VARS(work)->since_hit++ > SARU_HIT_TIME) {
        work->flags &= ~SARU_HIT_PLAYER;
    }
    if (work->flags & SARU_SOLID) {
        collision_register(work);
    }
    task_step_children(task);
}
