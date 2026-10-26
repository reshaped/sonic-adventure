/* Chaos 6's bombs, the satellites the hints name. in Sonic's fight Eggman
 * drops them, and they walk the deck. in Knuckles' fight a ring of them
 * circles the boss. a bomb on the deck near player 0, away from the fight's
 * center, rises, opens and sprays frost. the spray can freeze him, and for a
 * while the deck there gives him the least grip. a struck bomb can be
 * carried and thrown. one that flies into the chasing boss's mouth, or that
 * his inhale draws in, freezes him. one that comes near him otherwise blows
 * up, as does one lying on the deck as he lands from a leap */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos6/bomb.h"
#include "b_chaos6/boss.h"
#include "b_chaos6/snow.h"
#include "b_chaos6/textures.h"
#include "camera/camera.h"
#include "chaos/steer.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/hit.h"
#include "object/common.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define SE_BOMB_STRUCK 25
#define SE_BOMB_SPRAY 397
#define SE_FROST 398
#define SE_BOMB_LAND 399
#define SE_BOMB_BLAST 400

/* the deck's height. a ring's bombs start at SKY_Y. a leaving bomb is killed
 * once it rises to it. a bomb at or below GONE_Y is killed */
#define DECK_Y 752.0F
#define SKY_Y 1244.0F
#define GONE_Y 600.0F
/* a bomb starts its drop moving up at DROP_LIFT a step. its speed then
 * gains BOMB_GRAVITY a step. a bomb about to spray rises by -BOMB_GRAVITY a
 * step to SPRAY_Y */
#define DROP_LIFT 3.0F
#define BOMB_GRAVITY (-0.2F)
#define SPRAY_Y 769.0F
/* a bomb on the deck rises to spray only while it's farther than
 * CENTER_CLEAR from the fight's center in x and z */
#define CENTER_CLEAR 90.0F
/* a spray lasts SPRAY_TIME + 1 steps, unless it freezes player 0 first. it
 * can freeze him in its last FREEZE_TIME steps, while he is lower than
 * FREEZE_HEIGHT above the bomb. his squared distance from the bomb in x and
 * z must also be under FREEZE_NEAR_SQUARED */
#define SPRAY_TIME 90
#define FREEZE_TIME 50
#define FREEZE_NEAR_SQUARED 12.0F
#define FREEZE_HEIGHT 7.5F
/* a bomb waits BOMB_WAIT_TIME steps before it rises away. chaos6_bomb_run
 * kills a bomb BLAST_TIME steps into BOMB_BLAST, a mode it never meets */
#define BOMB_WAIT_TIME 180
#define BLAST_TIME 60
/* a struck bomb within BOSS_NEAR of the boss blows up, unless he's frozen.
 * one lying on the deck blows up too while the boss chases player 0 and is
 * within BOSS_NEAR of him. while the boss chases, one in flight within
 * MOUTH_NEAR of his mouth freezes him */
#define BOSS_NEAR 30.0F
#define MOUTH_NEAR 25.0F
/* a struck bomb's speed is STRUCK_SHARE of player 0's motion speed, with
 * none in y */
#define STRUCK_SHARE 0.3F
/* a spark lasts SPARK_TIME steps */
#define SPARK_TIME 60
/* the ring holds RING_SLOTS bombs, RING_RADIUS from the boss and SLOT_TURN
 * apart */
#define RING_SLOTS 6
#define RING_RADIUS 125.0F
#define SLOT_TURN (0x10000 / RING_SLOTS)
/* a bomb lying on the deck keeps within ARENA_RADIUS of the fight's center
 * in x and z */
#define ARENA_RADIUS 225.0F

/* the glow's texture, c6_ht16, by its index in chaos6_effect_texlist */
#define GLOW_TEXTURE 15

/* a frost fades in by FROST_FADE a step to FROST_ALPHA. it holds there for
 * FROST_HOLD_TIME steps, then fades out by FROST_FADE a step. each frost of
 * a spray lies FROST_SPREAD farther from the bomb than the last */
#define FROST_FADE 0.0125F
#define FROST_ALPHA 0.75F
#define FROST_HOLD_TIME 600
#define FROST_SPREAD (16.0F / 9.0F)
/* the ice lasts ICE_TIME steps. while player 0 stands on land near it, his
 * ground's flags become WORLD_GRIP_LEAST. he is near while his squared
 * distance from it in x and z is at most ICE_NEAR_SQUARED */
#define ICE_TIME 600
#define ICE_NEAR_SQUARED 20.0F

/* rows of bomb_actions */
enum bomb_action {
    BOMB_WALK_ACTION,
    BOMB_OPEN_ACTION,
    BOMB_SPRAY_ACTION,
    BOMB_CLOSE_ACTION,
    BOMB_UNPLAYED_ACTION,
    BOMB_TUMBLE_ACTION
};

/* a bomb's modes. chaos6_bomb_run steps the bomb until it's struck, and
 * chaos6_bomb_carried after. chaos6_bomb_run's cases for BOMB_STRUCK,
 * BOMB_LANDED and BOMB_BLAST never run, since striking the bomb makes
 * chaos6_bomb_carried its step. nothing in bomb.c sets BOMB_HOLD or
 * BOMB_STILL */
enum bomb_mode {
    BOMB_DROP, /* down to the deck */
    /* it takes a heading toward player 0, less 10 to 20 degrees, a speed and
     * a walk's length */
    BOMB_AIM,
    /* Eggman's bomb walks its heading, and a ring's bomb keeps to its slot.
     * either waits once the boss is in none of BOSS_START, BOSS_CHASE,
     * BOSS_STRIKE and BOSS_WEAVE */
    BOMB_WALK,
    BOMB_RISE, /* up to SPRAY_Y, then it sprays once it has opened */
    BOMB_SPRAY,
    BOMB_WAIT, /* still, then it rises away */
    BOMB_LEAVE, /* up to SKY_Y, where it's killed */
    /* still, until the boss is in BOSS_START, BOSS_CHASE, BOSS_STRIKE or
     * BOSS_WEAVE. then it drops */
    BOMB_HOLD,
    BOMB_STRUCK, /* knocked flying, until it first lands */
    BOMB_LANDED, /* once it has landed: lying, carried or thrown */
    BOMB_BLAST,
    BOMB_STILL /* in place, and it rises to spray as a walking bomb does */
};

/* a bomb's record, at its task's extra. each step until the bomb is
 * stopped, frame gains the speed in action's row of bomb_actions. once
 * frame reaches the last frame of the action's motion, next becomes the
 * action, from frame 0. the new row's next then becomes next */
struct bomb {
    Sint8 stopped; /* TRUE once a struck bomb lands, which holds frame */
    Uint8 unk_01;
    Sint16 action; /* a row of bomb_actions */
    Sint16 next;
    Sint16 spray; /* steps left to spray */
    Sint16 age; /* steps counted in BOMB_STRUCK, or in BOMB_BLAST */
    Uint8 unk_0a[2];
    Sint32 frosts; /* the frosts this spray has made so far */
    Float frame;
};

/* the steps a spark has lasted, or an ice patch has left, in its work's
 * vars */
struct count_vars {
    Uint8 unk_00[4];
    Sint32 steps;
};

#define COUNT_VARS(work) ((struct count_vars *)&(work)->vars)

struct frost_vars {
    Float alpha;
    Sint32 held; /* steps at FROST_ALPHA */
};

#define FROST_VARS(work) ((struct frost_vars *)&(work)->vars)

enum frost_mode { FROST_IN, FROST_HOLD, FROST_OUT };
enum ice_mode { ICE_START, ICE_RUN };
enum ring_mode { RING_START, RING_RUN };

#pragma section HEAD
/* a walking bomb's speed is walk_speed a step, raised by a random share of
 * up to 60%. a leaving bomb rises leave_speed a step */
static Float walk_speed = 0.8F;
static Float leave_speed = 0.8F;
/* a bomb on the deck rises to spray within spray_reach of player 0 in x and
 * z */
static Float spray_reach = 30.0F;
/* a frost puff starts with a speed of puff_speed_y in y, and within half
 * puff_spread either way in x and z */
static Float puff_speed_y = -0.75F;
static Float puff_spread = 0.3F;
/* a frost lies 1 above the highest floor at or below the bomb's height
 * plus frost_drop */
static Float frost_drop = -10.0F;
static Angle ring_turn = 24; /* the ring's turn a step */
#pragma section

static const Uint8 bomb_shape_count = 3;

static void chaos6_bomb_run(struct task *task);

static NJS_MATERIAL bomb_node26_materials[1] = {
#include "b_chaos6/bomb/bomb_node26_materials.inc"
};

static Sint16 bomb_node26_meshes0[30] = {
#include "b_chaos6/bomb/bomb_node26_meshes0.inc"
};

static NJS_COLOR bomb_node26_uvs0[28] = {
#include "b_chaos6/bomb/bomb_node26_uvs0.inc"
};

static NJS_MESHSET bomb_node26_meshsets[1] = {
#include "b_chaos6/bomb/bomb_node26_meshsets.inc"
};

static NJS_POINT3 bomb_node26_points[18] = {
#include "b_chaos6/bomb/bomb_node26_points.inc"
};

static NJS_VECTOR bomb_node26_normals[18] = {
#include "b_chaos6/bomb/bomb_node26_normals.inc"
};

static NJS_MODEL bomb_node26_model = {
#include "b_chaos6/bomb/bomb_node26_model.inc"
};

static NJS_OBJECT bomb_node26 = {
#include "b_chaos6/bomb/bomb_node26.inc"
};

static NJS_MATERIAL bomb_node25_materials[1] = {
#include "b_chaos6/bomb/bomb_node25_materials.inc"
};

static Sint16 bomb_node25_meshes0[17] = {
#include "b_chaos6/bomb/bomb_node25_meshes0.inc"
};

static NJS_COLOR bomb_node25_uvs0[14] = {
#include "b_chaos6/bomb/bomb_node25_uvs0.inc"
};

static NJS_MESHSET bomb_node25_meshsets[1] = {
#include "b_chaos6/bomb/bomb_node25_meshsets.inc"
};

static NJS_POINT3 bomb_node25_points[6] = {
#include "b_chaos6/bomb/bomb_node25_points.inc"
};

static NJS_VECTOR bomb_node25_normals[6] = {
#include "b_chaos6/bomb/bomb_node25_normals.inc"
};

static NJS_MODEL bomb_node25_model = {
#include "b_chaos6/bomb/bomb_node25_model.inc"
};

static NJS_OBJECT bomb_node25 = {
#include "b_chaos6/bomb/bomb_node25.inc"
};

static NJS_OBJECT bomb_node24 = {
#include "b_chaos6/bomb/bomb_node24.inc"
};

static NJS_OBJECT bomb_node23 = {
#include "b_chaos6/bomb/bomb_node23.inc"
};

static NJS_OBJECT bomb_node22 = {
#include "b_chaos6/bomb/bomb_node22.inc"
};

static NJS_MATERIAL bomb_node21_materials[1] = {
#include "b_chaos6/bomb/bomb_node21_materials.inc"
};

static Sint16 bomb_node21_meshes0[10] = {
#include "b_chaos6/bomb/bomb_node21_meshes0.inc"
};

static NJS_COLOR bomb_node21_uvs0[8] = {
#include "b_chaos6/bomb/bomb_node21_uvs0.inc"
};

static NJS_MESHSET bomb_node21_meshsets[1] = {
#include "b_chaos6/bomb/bomb_node21_meshsets.inc"
};

static NJS_POINT3 bomb_node21_points[5] = {
#include "b_chaos6/bomb/bomb_node21_points.inc"
};

static NJS_VECTOR bomb_node21_normals[5] = {
#include "b_chaos6/bomb/bomb_node21_normals.inc"
};

static NJS_MODEL bomb_node21_model = {
#include "b_chaos6/bomb/bomb_node21_model.inc"
};

static NJS_OBJECT bomb_node21 = {
#include "b_chaos6/bomb/bomb_node21.inc"
};

static NJS_MATERIAL bomb_node20_materials[1] = {
#include "b_chaos6/bomb/bomb_node20_materials.inc"
};

static Sint16 bomb_node20_meshes0[17] = {
#include "b_chaos6/bomb/bomb_node20_meshes0.inc"
};

static NJS_COLOR bomb_node20_uvs0[14] = {
#include "b_chaos6/bomb/bomb_node20_uvs0.inc"
};

static NJS_MESHSET bomb_node20_meshsets[1] = {
#include "b_chaos6/bomb/bomb_node20_meshsets.inc"
};

static NJS_POINT3 bomb_node20_points[6] = {
#include "b_chaos6/bomb/bomb_node20_points.inc"
};

static NJS_VECTOR bomb_node20_normals[6] = {
#include "b_chaos6/bomb/bomb_node20_normals.inc"
};

static NJS_MODEL bomb_node20_model = {
#include "b_chaos6/bomb/bomb_node20_model.inc"
};

static NJS_OBJECT bomb_node20 = {
#include "b_chaos6/bomb/bomb_node20.inc"
};

static NJS_OBJECT bomb_node19 = {
#include "b_chaos6/bomb/bomb_node19.inc"
};

static NJS_OBJECT bomb_node18 = {
#include "b_chaos6/bomb/bomb_node18.inc"
};

static NJS_OBJECT bomb_node17 = {
#include "b_chaos6/bomb/bomb_node17.inc"
};

static NJS_MATERIAL bomb_node16_materials[1] = {
#include "b_chaos6/bomb/bomb_node16_materials.inc"
};

static Sint16 bomb_node16_meshes0[10] = {
#include "b_chaos6/bomb/bomb_node16_meshes0.inc"
};

static NJS_COLOR bomb_node16_uvs0[8] = {
#include "b_chaos6/bomb/bomb_node16_uvs0.inc"
};

static NJS_MESHSET bomb_node16_meshsets[1] = {
#include "b_chaos6/bomb/bomb_node16_meshsets.inc"
};

static NJS_POINT3 bomb_node16_points[5] = {
#include "b_chaos6/bomb/bomb_node16_points.inc"
};

static NJS_VECTOR bomb_node16_normals[5] = {
#include "b_chaos6/bomb/bomb_node16_normals.inc"
};

static NJS_MODEL bomb_node16_model = {
#include "b_chaos6/bomb/bomb_node16_model.inc"
};

static NJS_OBJECT bomb_node16 = {
#include "b_chaos6/bomb/bomb_node16.inc"
};

static NJS_MATERIAL bomb_node15_materials[3] = {
#include "b_chaos6/bomb/bomb_node15_materials.inc"
};

static Sint16 bomb_node15_meshes0[18] = {
#include "b_chaos6/bomb/bomb_node15_meshes0.inc"
};

static Sint16 bomb_node15_meshes1[24] = {
#include "b_chaos6/bomb/bomb_node15_meshes1.inc"
};

static Sint16 bomb_node15_meshes2[19] = {
#include "b_chaos6/bomb/bomb_node15_meshes2.inc"
};

static NJS_COLOR bomb_node15_uvs0[16] = {
#include "b_chaos6/bomb/bomb_node15_uvs0.inc"
};

static NJS_COLOR bomb_node15_uvs1[20] = {
#include "b_chaos6/bomb/bomb_node15_uvs1.inc"
};

static NJS_COLOR bomb_node15_uvs2[16] = {
#include "b_chaos6/bomb/bomb_node15_uvs2.inc"
};

static NJS_MESHSET bomb_node15_meshsets[3] = {
#include "b_chaos6/bomb/bomb_node15_meshsets.inc"
};

static NJS_POINT3 bomb_node15_points[19] = {
#include "b_chaos6/bomb/bomb_node15_points.inc"
};

static NJS_VECTOR bomb_node15_normals[19] = {
#include "b_chaos6/bomb/bomb_node15_normals.inc"
};

static NJS_MODEL bomb_node15_model = {
#include "b_chaos6/bomb/bomb_node15_model.inc"
};

static NJS_OBJECT bomb_node15 = {
#include "b_chaos6/bomb/bomb_node15.inc"
};

static NJS_OBJECT bomb_node14 = {
#include "b_chaos6/bomb/bomb_node14.inc"
};

static NJS_OBJECT bomb_node13 = {
#include "b_chaos6/bomb/bomb_node13.inc"
};

static NJS_OBJECT bomb_node12 = {
#include "b_chaos6/bomb/bomb_node12.inc"
};

static NJS_MATERIAL bomb_node11_materials[3] = {
#include "b_chaos6/bomb/bomb_node11_materials.inc"
};

static Sint16 bomb_node11_meshes0[18] = {
#include "b_chaos6/bomb/bomb_node11_meshes0.inc"
};

static Sint16 bomb_node11_meshes1[18] = {
#include "b_chaos6/bomb/bomb_node11_meshes1.inc"
};

static Sint16 bomb_node11_meshes2[19] = {
#include "b_chaos6/bomb/bomb_node11_meshes2.inc"
};

static NJS_COLOR bomb_node11_uvs0[16] = {
#include "b_chaos6/bomb/bomb_node11_uvs0.inc"
};

static NJS_COLOR bomb_node11_uvs1[16] = {
#include "b_chaos6/bomb/bomb_node11_uvs1.inc"
};

static NJS_COLOR bomb_node11_uvs2[16] = {
#include "b_chaos6/bomb/bomb_node11_uvs2.inc"
};

static NJS_MESHSET bomb_node11_meshsets[3] = {
#include "b_chaos6/bomb/bomb_node11_meshsets.inc"
};

static NJS_POINT3 bomb_node11_points[19] = {
#include "b_chaos6/bomb/bomb_node11_points.inc"
};

static NJS_VECTOR bomb_node11_normals[19] = {
#include "b_chaos6/bomb/bomb_node11_normals.inc"
};

static NJS_MODEL bomb_node11_model = {
#include "b_chaos6/bomb/bomb_node11_model.inc"
};

static NJS_OBJECT bomb_node11 = {
#include "b_chaos6/bomb/bomb_node11.inc"
};

static NJS_OBJECT bomb_node10 = {
#include "b_chaos6/bomb/bomb_node10.inc"
};

static NJS_OBJECT bomb_node9 = {
#include "b_chaos6/bomb/bomb_node9.inc"
};

static NJS_OBJECT bomb_node8 = {
#include "b_chaos6/bomb/bomb_node8.inc"
};

static NJS_MATERIAL bomb_node7_materials[1] = {
#include "b_chaos6/bomb/bomb_node7_materials.inc"
};

static Sint16 bomb_node7_meshes0[21] = {
#include "b_chaos6/bomb/bomb_node7_meshes0.inc"
};

static NJS_COLOR bomb_node7_uvs0[18] = {
#include "b_chaos6/bomb/bomb_node7_uvs0.inc"
};

static NJS_MESHSET bomb_node7_meshsets[1] = {
#include "b_chaos6/bomb/bomb_node7_meshsets.inc"
};

static NJS_POINT3 bomb_node7_points[9] = {
#include "b_chaos6/bomb/bomb_node7_points.inc"
};

static NJS_VECTOR bomb_node7_normals[9] = {
#include "b_chaos6/bomb/bomb_node7_normals.inc"
};

static NJS_MODEL bomb_node7_model = {
#include "b_chaos6/bomb/bomb_node7_model.inc"
};

static NJS_OBJECT bomb_node7 = {
#include "b_chaos6/bomb/bomb_node7.inc"
};

static NJS_OBJECT bomb_node6 = {
#include "b_chaos6/bomb/bomb_node6.inc"
};

static NJS_OBJECT bomb_node5 = {
#include "b_chaos6/bomb/bomb_node5.inc"
};

static NJS_OBJECT bomb_node4 = {
#include "b_chaos6/bomb/bomb_node4.inc"
};

static NJS_MATERIAL bomb_node3_materials[6] = {
#include "b_chaos6/bomb/bomb_node3_materials.inc"
};

static Sint16 bomb_node3_meshes0[12] = {
#include "b_chaos6/bomb/bomb_node3_meshes0.inc"
};

static Sint16 bomb_node3_meshes1[23] = {
#include "b_chaos6/bomb/bomb_node3_meshes1.inc"
};

static Sint16 bomb_node3_meshes2[20] = {
#include "b_chaos6/bomb/bomb_node3_meshes2.inc"
};

static Sint16 bomb_node3_meshes3[24] = {
#include "b_chaos6/bomb/bomb_node3_meshes3.inc"
};

static Sint16 bomb_node3_meshes4[10] = {
#include "b_chaos6/bomb/bomb_node3_meshes4.inc"
};

static Sint16 bomb_node3_meshes5[5] = {
#include "b_chaos6/bomb/bomb_node3_meshes5.inc"
};

static NJS_COLOR bomb_node3_uvs0[10] = {
#include "b_chaos6/bomb/bomb_node3_uvs0.inc"
};

static NJS_COLOR bomb_node3_uvs1[20] = {
#include "b_chaos6/bomb/bomb_node3_uvs1.inc"
};

static NJS_COLOR bomb_node3_uvs2[16] = {
#include "b_chaos6/bomb/bomb_node3_uvs2.inc"
};

static NJS_COLOR bomb_node3_uvs3[20] = {
#include "b_chaos6/bomb/bomb_node3_uvs3.inc"
};

static NJS_COLOR bomb_node3_uvs4[8] = {
#include "b_chaos6/bomb/bomb_node3_uvs4.inc"
};

static NJS_COLOR bomb_node3_uvs5[4] = {
#include "b_chaos6/bomb/bomb_node3_uvs5.inc"
};

static NJS_MESHSET bomb_node3_meshsets[6] = {
#include "b_chaos6/bomb/bomb_node3_meshsets.inc"
};

static NJS_POINT3 bomb_node3_points[25] = {
#include "b_chaos6/bomb/bomb_node3_points.inc"
};

static NJS_VECTOR bomb_node3_normals[25] = {
#include "b_chaos6/bomb/bomb_node3_normals.inc"
};

static NJS_MODEL bomb_node3_model = {
#include "b_chaos6/bomb/bomb_node3_model.inc"
};

static NJS_OBJECT bomb_node3 = {
#include "b_chaos6/bomb/bomb_node3.inc"
};

static NJS_OBJECT bomb_node2 = {
#include "b_chaos6/bomb/bomb_node2.inc"
};

static NJS_OBJECT bomb_node1 = {
#include "b_chaos6/bomb/bomb_node1.inc"
};

static NJS_OBJECT bomb_node0 = {
#include "b_chaos6/bomb/bomb_node0.inc"
};

static NJS_MKEY_F bomb_open_pos0[7] = {
#include "b_chaos6/bomb/bomb_open_pos0.inc"
};

static NJS_MKEY_F bomb_open_pos4[15] = {
#include "b_chaos6/bomb/bomb_open_pos4.inc"
};

static NJS_MKEY_F bomb_open_pos8[11] = {
#include "b_chaos6/bomb/bomb_open_pos8.inc"
};

static NJS_MKEY_F bomb_open_pos12[11] = {
#include "b_chaos6/bomb/bomb_open_pos12.inc"
};

static NJS_MKEY_A bomb_open_ang18[7] = {
#include "b_chaos6/bomb/bomb_open_ang18.inc"
};

static NJS_MKEY_A bomb_open_ang23[7] = {
#include "b_chaos6/bomb/bomb_open_ang23.inc"
};

static NJS_MDATA2 bomb_open_mdata[27] = {
#include "b_chaos6/bomb/bomb_open_mdata.inc"
};

static NJS_MOTION bomb_open_motion = {
#include "b_chaos6/bomb/bomb_open_motion.inc"
};

static NJS_ACTION bomb_open_action = {
#include "b_chaos6/bomb/bomb_open_action.inc"
};

static NJS_MKEY_F bomb_spray_pos0[13] = {
#include "b_chaos6/bomb/bomb_spray_pos0.inc"
};

static NJS_MKEY_F bomb_spray_pos8[13] = {
#include "b_chaos6/bomb/bomb_spray_pos8.inc"
};

static NJS_MKEY_A bomb_spray_ang9[13] = {
#include "b_chaos6/bomb/bomb_spray_ang9.inc"
};

static NJS_MKEY_F bomb_spray_pos12[13] = {
#include "b_chaos6/bomb/bomb_spray_pos12.inc"
};

static NJS_MKEY_A bomb_spray_ang13[13] = {
#include "b_chaos6/bomb/bomb_spray_ang13.inc"
};

static NJS_MKEY_A bomb_spray_ang18[13] = {
#include "b_chaos6/bomb/bomb_spray_ang18.inc"
};

static NJS_MKEY_A bomb_spray_ang23[13] = {
#include "b_chaos6/bomb/bomb_spray_ang23.inc"
};

static NJS_MDATA2 bomb_spray_mdata[27] = {
#include "b_chaos6/bomb/bomb_spray_mdata.inc"
};

static NJS_MOTION bomb_spray_motion = {
#include "b_chaos6/bomb/bomb_spray_motion.inc"
};

static NJS_ACTION bomb_spray_action = {
#include "b_chaos6/bomb/bomb_spray_action.inc"
};

static NJS_MKEY_F bomb_close_pos0[15] = {
#include "b_chaos6/bomb/bomb_close_pos0.inc"
};

static NJS_MKEY_F bomb_close_pos8[15] = {
#include "b_chaos6/bomb/bomb_close_pos8.inc"
};

static NJS_MKEY_F bomb_close_pos12[15] = {
#include "b_chaos6/bomb/bomb_close_pos12.inc"
};

static NJS_MKEY_A bomb_close_ang18[15] = {
#include "b_chaos6/bomb/bomb_close_ang18.inc"
};

static NJS_MKEY_A bomb_close_ang23[15] = {
#include "b_chaos6/bomb/bomb_close_ang23.inc"
};

static NJS_MDATA2 bomb_close_mdata[27] = {
#include "b_chaos6/bomb/bomb_close_mdata.inc"
};

static NJS_MOTION bomb_close_motion = {
#include "b_chaos6/bomb/bomb_close_motion.inc"
};

static NJS_ACTION bomb_close_action = {
#include "b_chaos6/bomb/bomb_close_action.inc"
};

static NJS_MKEY_A bomb_walk_ang1[30] = {
#include "b_chaos6/bomb/bomb_walk_ang1.inc"
};

static NJS_MDATA2 bomb_walk_mdata[27] = {
#include "b_chaos6/bomb/bomb_walk_mdata.inc"
};

static NJS_MOTION bomb_walk_motion = {
#include "b_chaos6/bomb/bomb_walk_motion.inc"
};

static NJS_ACTION bomb_walk_action = {
#include "b_chaos6/bomb/bomb_walk_action.inc"
};

static NJS_MKEY_A bomb_tumble_ang1[40] = {
#include "b_chaos6/bomb/bomb_tumble_ang1.inc"
};

static NJS_MKEY_F bomb_tumble_pos4[40] = {
#include "b_chaos6/bomb/bomb_tumble_pos4.inc"
};

static NJS_MKEY_F bomb_tumble_pos8[40] = {
#include "b_chaos6/bomb/bomb_tumble_pos8.inc"
};

static NJS_MKEY_F bomb_tumble_pos12[40] = {
#include "b_chaos6/bomb/bomb_tumble_pos12.inc"
};

static NJS_MKEY_A bomb_tumble_ang18[40] = {
#include "b_chaos6/bomb/bomb_tumble_ang18.inc"
};

static NJS_MKEY_A bomb_tumble_ang23[40] = {
#include "b_chaos6/bomb/bomb_tumble_ang23.inc"
};

static NJS_MDATA2 bomb_tumble_mdata[27] = {
#include "b_chaos6/bomb/bomb_tumble_mdata.inc"
};

static NJS_MOTION bomb_tumble_motion = {
#include "b_chaos6/bomb/bomb_tumble_motion.inc"
};

static NJS_ACTION bomb_tumble_action = {
#include "b_chaos6/bomb/bomb_tumble_action.inc"
};

static NJS_MKEY_F bomb_unplayed_pos4[10] = {
#include "b_chaos6/bomb/bomb_unplayed_pos4.inc"
};

static NJS_MKEY_F bomb_unplayed_pos8[10] = {
#include "b_chaos6/bomb/bomb_unplayed_pos8.inc"
};

static NJS_MKEY_F bomb_unplayed_pos12[10] = {
#include "b_chaos6/bomb/bomb_unplayed_pos12.inc"
};

static NJS_MKEY_A bomb_unplayed_ang18[10] = {
#include "b_chaos6/bomb/bomb_unplayed_ang18.inc"
};

static NJS_MKEY_A bomb_unplayed_ang23[10] = {
#include "b_chaos6/bomb/bomb_unplayed_ang23.inc"
};

static NJS_MDATA2 bomb_unplayed_mdata[27] = {
#include "b_chaos6/bomb/bomb_unplayed_mdata.inc"
};

static NJS_MOTION bomb_unplayed_motion = {
#include "b_chaos6/bomb/bomb_unplayed_motion.inc"
};

static NJS_ACTION bomb_unplayed_action = {
#include "b_chaos6/bomb/bomb_unplayed_action.inc"
};

/* the glow drawn on a bomb until it's struck. the bomb's draw sets its
 * frame's texid to GLOW_TEXTURE */
static NJS_TEXANIM bomb_glow_frame = {
#include "b_chaos6/bomb/bomb_glow_frame.inc"
};
static NJS_SPRITE bomb_glow = {
#include "b_chaos6/bomb/bomb_glow.inc"
};

/* a bomb's actions, by enum bomb_action. the bomb plays a row by its action,
 * next and speed alone */
static struct player_action bomb_actions[6] = {
    {&bomb_walk_action, 7, PLAYER_ANIM_LOOP, BOMB_WALK_ACTION, 1.0F, 1.0F},
    {&bomb_open_action, 7, PLAYER_ANIM_ONCE, BOMB_SPRAY_ACTION, 1.0F, 1.0F},
    {&bomb_spray_action, 7, PLAYER_ANIM_LOOP, BOMB_SPRAY_ACTION, 1.0F, 1.0F},
    {&bomb_close_action, 7, PLAYER_ANIM_ONCE, BOMB_WALK_ACTION, 1.0F, 1.0F},
    {&bomb_unplayed_action, 7, PLAYER_ANIM_LOOP, BOMB_UNPLAYED_ACTION, 1.0F,
     1.0F},
    {&bomb_tumble_action, 7, PLAYER_ANIM_LOOP, BOMB_TUMBLE_ACTION, 1.0F, 1.0F},
};

/* shapes 0 and 1 are off until the bomb is struck, and shape 2 is off
 * after. shape 0, a wide cylinder, pushes nothing, and nothing pushes it.
 * shape 1, a thin one, pushes every group. shape 2 is the ball an attack
 * strikes. its attack is 0, and one shape hurts another only when its
 * attack exceeds the other's guard, so it hurts no player */
static struct collision_shape bomb_shapes[3] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.5F, 0.0F},
     20.0F,
     5.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 2.0F, 0.0F},
     0.3F,
     3.5F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     9.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the params every frost puff reads while it lives. chaos6_bomb_puff sets
 * pos and speed for each new puff. a puff copies those two only as it
 * starts, so the puffs already out keep theirs */
static struct dust_params puff_params = {
    1.5F,
    0.1F,
    0.1F,
    0.05F,
    -0.05F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 1.2F, 0.0F},
    {0.45F, 0.85F, 0.85F, 0.85F},
};

/* a frost on the deck, whose frame names c6_frost by its index in
 * chaos6_effect_texlist */
static NJS_TEXANIM frost_frame = {
#include "b_chaos6/bomb/frost_frame.inc"
};
static NJS_SPRITE frost_sprite = {
#include "b_chaos6/bomb/frost_sprite.inc"
};

Sint16 chaos6_bomb_count;
/* the ring's bombs by slot, or NULL for an empty slot */
static struct task *bomb_ring_slots[RING_SLOTS];
/* each slot's point this step. only x and z are read */
static NJS_POINT3 bomb_ring_points[RING_SLOTS];
/* the ring's task, which chaos6_bomb_ring_step keeps as it starts and
 * nothing reads */
static struct task *bomb_ring_task;

static void chaos6_bomb_puff(struct task_work *work);
static void chaos6_frost_create(NJS_POINT3 *center, int frosts);
static void chaos6_ice_step(struct task *task);
static void chaos6_keep_in_arena(struct task_work *work);

/* lightning about the struck bomb, the spark's parent, each step for
 * SPARK_TIME steps. once the parent or its work is gone, the spark makes no
 * lightning, but still counts its steps */
static void chaos6_bomb_spark_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;

    ++COUNT_VARS(work)->steps;
    if (COUNT_VARS(work)->steps > SPARK_TIME) {
        task_destroy(task);
        return;
    }
    if (task->parent == NULL) {
        return;
    }
    if (task->parent->work == NULL) {
        return;
    }
    pos = task->parent->work->pos;
    pos.x += 4.0F - (rand() * (8.0F * RAND_SCALE));
    pos.y += 4.0F - (rand() * (8.0F * RAND_SCALE));
    pos.z += 4.0F - (rand() * (8.0F * RAND_SCALE));
    lightning_create(&pos, 0.7F);
}

/* the bomb's destructor, which empties a ring's bomb's slot.
 * chaos6_bomb_count falls by 1, to no less than 0 */
static void chaos6_bomb_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (BOMB_VARS(work)->slot[0] >= 0) {
        bomb_ring_slots[BOMB_VARS(work)->slot[0]] = NULL;
    }
    --chaos6_bomb_count;
    if (chaos6_bomb_count < 0) {
        chaos6_bomb_count = 0;
    }
}

/* draw the bomb, and until it's struck, a glow facing the camera, added to
 * what is behind it */
static void chaos6_bomb_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct bomb *bomb = task->extra;
    Angle ang;

    njSetTexture(&chaos6_object_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + 3.75F, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, 1.5F, 1.5F, 1.5F);
    draw_action(bomb_actions[bomb->action].action, bomb->frame);
    njPopMatrix(1);
    if (task->step != chaos6_bomb_run) {
        return;
    }
    njFogDisable();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    draw_set_material(0.15F, 0.15F, 0.15F, 0.15F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    njTranslate(NULL, 0.0F, 3.75F, 5.0F);
    /* @hack spelling: the frame through its address, which pools
     * &bomb_glow_frame and stores texid through it at offset 16
     * (MOV.W R0,@(16,R2) at 0x0c904404); without it, 9 of 143 words differ */
    (&bomb_glow_frame)->texid = GLOW_TEXTURE;
    njDrawSprite3D(&bomb_glow, 0,
                   NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
    njFogEnable();
}

/* the step of a struck bomb, a small carried thing. it tumbles until it
 * first lands. after that it lies, or a player carries or throws it. a bomb
 * set to BOMB_BLAST blows up on its next step */
static void chaos6_bomb_carried(struct task *task)
{
    struct task_work *work = task->work;
    struct bomb *bomb = task->extra;
    /* @hack spelling: boss, a second name for boss_work, which keeps
     * njDistanceP2P's address in R8 (MOV.L at 0x0c9044de). through
     * boss_work, R8 holds 1 instead, and 103 of 255 words differ */
    struct task_work *boss;

    if (work->flags & CARRY_HELD) {
        work->ang.y = -player_work[0]->ang.y + NJM_DEG_ANG(90);
    }
    switch (work->mode) {
    case BOMB_STRUCK:
        ++bomb->age;
        if (work->flags & CARRY_GROUND) {
            bomb->age = 0;
            bomb->stopped = TRUE;
            work->mode = BOMB_LANDED;
            work->collision->shapes[0].flags &= ~SHAPE_OFF;
            work->collision->shapes[1].flags &= ~SHAPE_OFF;
        }
        break;
    case BOMB_LANDED:
        if (chaos6_inhale_pull(work)) {
            chaos6_set_mode(BOSS_FROZEN);
            work->pos = boss_points[0].pos;
            work->mode = BOMB_BLAST;
        } else if (work->flags & CARRY_GROUND) {
            chaos6_keep_in_arena(work);
            if (chaos6_landed) {
                work->mode = BOMB_BLAST;
            } else if (njDistanceP2P(&player_work[0]->pos, &boss_work->pos) <
                       BOSS_NEAR) {
                if (boss_work->mode == BOSS_CHASE) {
                    work->mode = BOMB_BLAST;
                }
            }
        }
        break;
    case BOMB_BLAST:
        bomb_create(&work->pos, 0.35F);
        sound_play(SE_BOMB_BLAST, 0, 0, 127);
        task_kill(task);
        return;
    }
    if (!bomb->stopped) {
        NJS_ACTION *action = bomb_actions[bomb->action].action;

        bomb->frame += bomb_actions[bomb->action].speed;
        if (bomb->frame >= action->motion->nbFrame - 1.0F) {
            bomb->frame = 0.0F;
            bomb->action = bomb->next;
            bomb->next = bomb_actions[bomb->action].next;
        }
    }
    boss = boss_work;
    if (boss->mode != BOSS_FROZEN) {
        if (njDistanceP2P(&work->pos, &boss->pos) < BOSS_NEAR) {
            work->mode = BOMB_BLAST;
        }
    }
    if (boss_work->mode == BOSS_CHASE) {
        if (njDistanceP2P(&work->pos, &boss_points[0].pos) < MOUTH_NEAR) {
            if (!(work->flags & (CARRY_HELD | CARRY_GROUND))) {
                chaos6_set_mode(BOSS_FROZEN);
                work->pos = boss_points[0].pos;
                work->mode = BOMB_BLAST;
            }
        }
    }
    if (work->pos.y <= GONE_Y) {
        task_kill(task);
        return;
    }
    carry_step_small(task);
    chaos6_bomb_draw(task);
    shadow_draw(work, 0.7F);
    task_step_children(task);
}

/* a bomb's step until it's struck. while player 0 holds something, the
 * bomb is no target. it dies while the set editor is open */
static void chaos6_bomb_run(struct task *task)
{
    struct task_work *player = player_work[0];
    struct task_motion *motion = task->motion;
    struct task_work *work = task->work;
    struct bomb *bomb = task->extra;
    struct task *ice;
    Float dx;
    Float dz;
    Float squared;
    Float speed;
    Angle ang;

    if (player->flags & PLAYER_HOLDING) {
        work->collision->flags &= ~COLLISION_TARGET;
    } else {
        work->collision->flags |= COLLISION_TARGET;
    }
    if (work->mode <= BOMB_LEAVE) {
        if (work->flags & COLLISION_DAMAGED) {
            /* @hack spelling: player_motion0, a local holding
             * player_motion[0], which loads it once into R4 for the speed's
             * copy (MOV.L @R4,R4 at 0x0c904820); read in place,
             * chaos6_bomb_run runs 64 bytes long and 508 of 733 words
             * differ */
            struct task_motion *player_motion0 = player_motion[0];

            motion->accel.x = motion->accel.y = motion->accel.z = 0.0F;
            motion->speed = player_motion0->speed;
            motion->speed.x *= STRUCK_SHARE;
            motion->speed.y *= STRUCK_SHARE;
            motion->speed.z *= STRUCK_SHARE;
            motion->speed.y = 0.0F;
            carry_set_kind(work, motion, CARRY_KIND_SMALL);
            work->collision->kind = COLLISION_OBJECT_MUTUAL;
            bomb->next = BOMB_TUMBLE_ACTION;
            work->mode = BOMB_STRUCK;
            task->step = chaos6_bomb_carried;
            work->collision->flags &= ~COLLISION_TARGET;
            work->collision->shapes[0].flags &= ~SHAPE_OFF;
            work->collision->shapes[1].flags &= ~SHAPE_OFF;
            work->collision->shapes[2].flags |= SHAPE_OFF;
            task_create_child(TASK_WORK, chaos6_bomb_spark_step, task);
            player_set_speed(0, 0.0F, 2.0F, 0.0F);
            vibration_start(0, 6, 30, 0);
            sound_play(SE_BOMB_STRUCK, 0, 0, 0);
            return;
        }
    }
    if (edit_mode == 1) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case BOMB_DROP:
        work->pos.y += motion->speed.y;
        motion->speed.y += BOMB_GRAVITY;
        if (work->pos.y <= DECK_Y) {
            work->pos.y = DECK_Y;
            motion->speed.y = 0.0F;
            work->mode = BOMB_AIM;
        }
        break;
    case BOMB_AIM:
        dx = player->pos.x - work->pos.x;
        dz = player->pos.z - work->pos.z;
        ang = (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN) +
              (Angle)(RANDOM(-20.0F, -10.0F) * ANGLE_PER_DEGREE);
        speed = walk_speed * ((rand() * (0.6F * RAND_SCALE)) + 1.0F);
        motion->speed.x = speed * njCos(ang);
        motion->speed.y = 0.0F;
        motion->speed.z = speed * njSin(ang);
        BOMB_VARS(work)->steps = (Sint32)RANDOM(60.0F, 180.0F);
        work->mode = BOMB_WALK;
        /* fall through */
    case BOMB_WALK:
        if (BOMB_VARS(work)->slot[0] < 0) {
            njAddVector(&work->pos, &motion->speed);
            --BOMB_VARS(work)->steps;
            if (BOMB_VARS(work)->steps <= 0) {
                work->mode = BOMB_AIM;
            }
        } else {
            work->pos.x = bomb_ring_points[BOMB_VARS(work)->slot[0]].x;
            work->pos.z = bomb_ring_points[BOMB_VARS(work)->slot[0]].z;
        }
        if (chaos6_bombs_paused()) {
            BOMB_VARS(work)->steps = 0;
            work->mode = BOMB_WAIT;
        }
        break;
    case BOMB_WAIT:
        ++BOMB_VARS(work)->steps;
        if (BOMB_VARS(work)->steps > BOMB_WAIT_TIME) {
            work->mode = BOMB_LEAVE;
        }
        break;
    case BOMB_LEAVE:
        work->pos.y += leave_speed;
        if (work->pos.y >= SKY_Y) {
            task_kill(task);
            return;
        }
        break;
    case BOMB_HOLD:
        if (!chaos6_bombs_paused()) {
            work->mode = BOMB_DROP;
        }
        break;
    case BOMB_STILL:
        break;
    }
    switch (work->mode) {
    case BOMB_STILL:
    case BOMB_WALK:
        if (work->pos.y <= DECK_Y) {
            dx = work->pos.x - chaos_params->center.x;
            dz = work->pos.z - chaos_params->center.z;
            squared = (dx * dx) + (dz * dz);
            if (squared > CENTER_CLEAR * CENTER_CLEAR) {
                dx = work->pos.x - player->pos.x;
                dz = work->pos.z - player->pos.z;
                squared = (dx * dx) + (dz * dz);
                if (squared < spray_reach * spray_reach) {
                    bomb->next = BOMB_OPEN_ACTION;
                    work->mode = BOMB_RISE;
                }
            }
        }
        collision_register(work);
        break;
    case BOMB_RISE:
        if (work->pos.y < SPRAY_Y) {
            work->pos.y -= BOMB_GRAVITY;
        } else if (bomb->action == BOMB_SPRAY_ACTION) {
            bomb->spray = SPRAY_TIME;
            bomb->frosts = 0;
            work->mode = BOMB_SPRAY;
            ice = task_create(TASK_WORK, 0, chaos6_ice_step);
            ice->work->pos = work->pos;
        }
        collision_register(work);
        break;
    case BOMB_SPRAY:
        sound_play_at_for(SE_BOMB_SPRAY, (int)work, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 6);
        if (!(bomb->spray & 3)) {
            chaos6_bomb_puff(work);
            chaos6_frost_create(&work->pos, bomb->frosts++);
        }
        if (bomb->spray < FREEZE_TIME) {
            dx = player->pos.x - work->pos.x;
            dz = player->pos.z - work->pos.z;
            squared = (dx * dx) + (dz * dz);
            if (player->pos.y < work->pos.y + FREEZE_HEIGHT &&
                squared < FREEZE_NEAR_SQUARED) {
                player_set_mode(0, PLAYER_MODE_FREEZE);
                bomb->next = BOMB_CLOSE_ACTION;
                BOMB_VARS(work)->steps = 0;
                work->mode = BOMB_WAIT;
                break;
            }
        }
        if (bomb->spray-- <= 0) {
            bomb->next = BOMB_CLOSE_ACTION;
            if (BOMB_VARS(work)->slot[0] < 0) {
                work->mode = BOMB_AIM;
            } else {
                work->mode = BOMB_LEAVE;
            }
            break;
        }
        collision_register(work);
        break;
    case BOMB_DROP:
    case BOMB_WAIT:
        collision_register(work);
        break;
    case BOMB_STRUCK:
        ++bomb->age;
        carry_step_small(task);
        if (work->flags & CARRY_GROUND) {
            bomb->age = 0;
            bomb->stopped = TRUE;
            work->collision->kind = COLLISION_OBJECT_MUTUAL;
            sound_play_at(SE_BOMB_LAND, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            work->mode = BOMB_LANDED;
        }
        break;
    case BOMB_LANDED:
        carry_step_small(task);
        if (chaos6_inhale_pull(work)) {
            chaos6_set_mode(BOSS_FROZEN);
            bomb->age = 0;
            work->mode = BOMB_BLAST;
        }
        break;
    case BOMB_BLAST:
        ++bomb->age;
        if (bomb->age > BLAST_TIME) {
            task_kill(task);
            return;
        }
        break;
    }
    if (!bomb->stopped) {
        struct player_action *entry = &bomb_actions[bomb->action];
        NJS_ACTION *action = entry->action;

        bomb->frame += entry->speed;
        if (bomb->frame >= action->motion->nbFrame - 1.0F) {
            bomb->frame = 0.0F;
            bomb->action = bomb->next;
            bomb->next = bomb_actions[bomb->action].next;
        }
    }
    if (work->pos.y <= GONE_Y) {
        task_kill(task);
        return;
    }
    chaos6_bomb_draw(task);
    shadow_draw(work, 0.7F);
    task_step_children(task);
}

void chaos6_bomb_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct bomb *bomb;

    collision_init(task, bomb_shapes, COLLISION_ENEMY, 1, bomb_shape_count);
    task->step = chaos6_bomb_run;
    task->draw = chaos6_bomb_draw;
    task->destructor = chaos6_bomb_destroy;
    work->collision->shapes[0].flags |= SHAPE_OFF;
    work->collision->shapes[1].flags |= SHAPE_OFF;
    bomb = task->extra = heap_calloc(1, sizeof(struct bomb));
    bomb->next = BOMB_WALK_ACTION;
    bomb->action = BOMB_WALK_ACTION;
    bomb->frame = 0.0F;
    motion->speed.y = DROP_LIFT;
    ++chaos6_bomb_count;
}

/* make a puff of frost at the bomb, which drifts down at puff_speed_y and
 * to a random side */
static void chaos6_bomb_puff(struct task_work *work)
{
    rand();
    puff_params.pos = work->pos;
    puff_params.speed.x = puff_spread * ((rand() * RAND_SCALE) - 0.5F);
    puff_params.speed.y = puff_speed_y;
    puff_params.speed.z = puff_spread * ((rand() * RAND_SCALE) - 0.5F);
    chaos6_snow_create_from(&puff_params);
}

/* draw the frost, flat on the floor under it and tilted by the floor's
 * slope, added to what is behind it */
static void chaos6_frost_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float alpha;
    Angle ang;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    alpha = FROST_VARS(work)->alpha;
    draw_set_material(alpha, alpha, alpha, alpha);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njRotateX(NULL, NJM_DEG_ANG(90));
    njDrawSprite3D(&frost_sprite, 0, NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
}

/* a frost's step. SE_FROST plays as it fades in */
static void chaos6_frost_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FROST_IN:
        sound_play_at_for(SE_FROST, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 10);
        FROST_VARS(work)->alpha += FROST_FADE;
        if (FROST_VARS(work)->alpha >= FROST_ALPHA) {
            work->mode = FROST_HOLD;
        }
        break;
    case FROST_HOLD:
        ++FROST_VARS(work)->held;
        if (FROST_VARS(work)->held >= FROST_HOLD_TIME) {
            work->mode = FROST_OUT;
        }
        break;
    case FROST_OUT:
        FROST_VARS(work)->alpha -= FROST_FADE;
        if (FROST_VARS(work)->alpha <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    chaos6_frost_draw(task);
}

/* a frost on the deck round center, the spraying bomb's position. frosts
 * counts the frosts its spray has made before this one. the frost lies
 * frosts times FROST_SPREAD from center in x and z, at a random heading. no
 * frost is made when task_create fails */
static void chaos6_frost_create(NJS_POINT3 *center, int frosts)
{
    struct task *task = task_create(TASK_WORK, 2, chaos6_frost_step);
    struct task_work *work;
    Float radius;
    Angle ang;

    if (task != NULL) {
        work = task->work;
        radius = frosts * FROST_SPREAD;
        ang = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
        work->pos.x = center->x + (radius * njCos(ang));
        work->pos.z = center->z + (radius * njSin(ang));
        work->pos.y = ground_below(work->pos.x, center->y + frost_drop,
                                   work->pos.z, &work->ang) +
                      1.0F;
        FROST_VARS(work)->alpha = 0.0F;
        task->draw = chaos6_frost_draw;
    }
}

/* the step of the ice patch a spray leaves at its bomb's position */
static void chaos6_ice_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    struct player *record;
    Float dx;
    Float dz;

    switch (work->mode) {
    case ICE_START:
        COUNT_VARS(work)->steps = ICE_TIME;
        work->mode = ICE_RUN;
        /* fall through */
    case ICE_RUN:
        if (!COUNT_VARS(work)->steps--) {
            task_destroy(task);
            return;
        }
        break;
    }
    player = player_work[0];
    record = player_motion[0]->record;
    dx = work->pos.x - player->pos.x;
    dz = work->pos.z - player->pos.z;
    if ((dx * dx) + (dz * dz) <= ICE_NEAR_SQUARED) {
        if (player->flags & PLAYER_ON_LAND) {
            record->ground_flags = WORLD_GRIP_LEAST;
        }
    }
}

/* the ring's first step empties every slot. each step, the ring turns
 * ring_turn, and its points follow the boss. a new bomb starts at SKY_Y over
 * its slot's point */
void chaos6_bomb_ring_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    Angle ang;
    struct task *bomb;
    struct task_work *bomb_work;

    switch (work->mode) {
    case RING_START:
        for (i = 0; i < RING_SLOTS; ++i) {
            bomb_ring_slots[i] = NULL;
        }
        bomb_ring_task = task;
        work->pos.y = 0.0F;
        work->mode = RING_RUN;
        break;
    case RING_RUN:
        break;
    }
    work->ang.y += ring_turn;
    ang = work->ang.y;
    for (i = 0; i < RING_SLOTS; ++i) {
        bomb_ring_points[i].x = boss_work->pos.x + (RING_RADIUS * njCos(ang));
        bomb_ring_points[i].y = 0.0F;
        bomb_ring_points[i].z = boss_work->pos.z + (RING_RADIUS * njSin(ang));
        if (!chaos6_bombs_paused()) {
            bomb = bomb_ring_slots[i];
            if (bomb == NULL) {
                bomb = task_create_child(TASK_WORK | TASK_MOTION,
                                         chaos6_bomb_step, task);
                if (bomb != NULL) {
                    bomb_work = bomb->work;
                    BOMB_VARS(bomb_work)->slot[0] = i;
                    bomb_work->pos.x = bomb_ring_points[i].x;
                    bomb_work->pos.y = SKY_Y;
                    bomb_work->pos.z = bomb_ring_points[i].z;
                    bomb_ring_slots[i] = bomb;
                }
            }
        }
        ang += SLOT_TURN;
    }
    task_step_children(task);
}

/* move work in x and z back onto the circle ARENA_RADIUS round the fight's
 * center, once it's on the circle or past it */
static void chaos6_keep_in_arena(struct task_work *work)
{
    Float dx = work->pos.x - chaos_params->center.x;
    Float dz = work->pos.z - chaos_params->center.z;
    Float squared = (dx * dx) + (dz * dz);
    Angle ang;

    if (squared >= ARENA_RADIUS * ARENA_RADIUS) {
        ang = (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
        work->pos.x = chaos_params->center.x + (njCos(ang) * ARENA_RADIUS);
        work->pos.z = chaos_params->center.z + (njSin(ang) * ARENA_RADIUS);
    }
}
