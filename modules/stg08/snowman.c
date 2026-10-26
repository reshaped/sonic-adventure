/* E SMAN, a snowman that chases player 0 and blows a freezing breath. once
 * its body is beaten, its head hops out and runs from player 0 */
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
#include "effect/dust.h"
#include "effect/explosion.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg08/snowman.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 3
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 7
#define TEXT_16_LENGTH 8
#include "stg08/snowman/text.inc"

/* pom01 to pom08, the snow puffs' eight frames */
extern NJS_TEXLIST icecap_pom_texlist;

enum snowman_mode {
    /* the body is beaten. on the task's next step, the task becomes the
     * head's, and the head comes out */
    SNOWMAN_BEATEN = -1,
    SNOWMAN_WAIT = 0, /* it waits until player 0 comes within RANGE */
    SNOWMAN_WAKE = 1, /* it bobs in place, gathering speed ahead */
    SNOWMAN_CHASE = 2, /* it chases player 0 while it sees a player */
    /* it wanders, swinging its head, until it sees a player */
    SNOWMAN_LOOK = 3,
    SNOWMAN_TURN = 4, /* it turns toward record->target */
    SNOWMAN_HIT = 5, /* it wobbles from a hit, then is beaten */
    SNOWMAN_BLOW = 6 /* it blows a freezing breath at record->target */
};
/* the head's work->mode, once the body is beaten. SNOWMAN_BEATEN then
 * blows the head up */
enum snowman_head_mode {
    /* the head's first step. it sets COLLISION_TARGET in
     * work->collision->flags, which snowman_step's collision_init set
     * already, and the head comes out */
    HEAD_START = 0,
    HEAD_OUT = 1, /* the lid tips open, and the head hops out */
    HEAD_HIT = 2, /* it wobbles from a hit, then is beaten */
    HEAD_WALK = 4 /* it runs from player 0 */
};

/* set the mode, at submode 0, unless the snowman is already beaten */
#define SNOWMAN_SET_MODE(work, to)                                             \
    do {                                                                       \
        if ((work)->mode != SNOWMAN_BEATEN) {                                  \
            (work)->mode = (to);                                               \
            (work)->submode = 0;                                               \
        }                                                                      \
    } while (0)

/* in work->flags */
#define SNOWMAN_SEES 0x100 /* a player is within the head's sight */
/* it has turned for home. coming within RANGE of home clears it, as a chase
 * does */
#define SNOWMAN_TURNED 0x200
#define SNOWMAN_HOME 0x400 /* home is within RANGE */
#define SNOWMAN_BURST 0x1000 /* the body has burst, so the head draws alone */

struct snowman_vars {
    Uint8 unk_00[4];
    /* sprays owed to its sides and along its trail, in 256ths of a spray */
    Sint16 sides_due;
    Sint16 trail_due;
    Float bob; /* how far the bob lifts and drops it */
};

#define VARS(work) ((struct snowman_vars *)&(work)->vars)

/* one spray, in sides_due and trail_due. snowman_collide throws sprays while
 * more than one is owed */
#define SPRAY 0x100

/* indexes into record->angles */
#define HEAD 0 /* the head's heading, then its flip about x as it hops out */
#define BOB 1 /* the bob's phase, which record->phase advances a step */
#define TILT 2 /* the lid's tilt about x as it opens */
#define BODY 3 /* the body's heading, once the head is out */

/* player 0 within RANGE wakes the snowman, and home within RANGE keeps it
 * from turning for home. RANGE is record->motion's drive, a player's field,
 * which the snowman uses its own way */
#define RANGE(record) ((record)->motion.drive)
/* the bob's lift this step, in record->motion's slip, a player's field */
#define LIFT(record) ((record)->motion.slip)
/* how far it swells as it draws a breath: a scale, 1 at rest */
#define SWELL(record) ((record)->motion.gravity)
/* how far the head turns a step. the swing flips its sign at each end */
#define HEAD_SPEED(record) ((record)->heading)
/* record->frame as the step before left it */
#define PREVIOUS_FRAME(record) ((record)->unk_1e4)
/* how fast scl.y springs back toward 1 */
#define SPRING_SPEED(work) ((work)->scl.z)

/* snowman_step sets RANGE to SNOWMAN_RANGE, whatever scl.x holds */
#define SNOWMAN_RANGE 50.0F
/* the set editor's ring is RANGE_SEGMENTS lines, each RANGE_TURN of a turn */
#define RANGE_SEGMENTS 16
#define RANGE_TURN (0x10000 / RANGE_SEGMENTS)

/* snowman_move takes MOVE_DRAG of the speed away a step, and MOVE_GRAVITY
 * from its y. a speed under MOVE_STILL along an axis stops */
#define MOVE_DRAG 0.1F
#define MOVE_GRAVITY 0.1F
#define MOVE_STILL 0.01F

/* the head turns no further than HEAD_REACH either way of the body */
#define HEAD_REACH 0x2000

/* the lid tips back LID_TILT, LID_TURN a step. angle_approach leaves the
 * tilt in 0 to 0xffff, so an open lid's tilt is LID_OPEN */
#define LID_TILT 0x4000
#define LID_TURN 0x400
#define LID_OPEN (0x10000 - LID_TILT)
/* the head flips about x from the top of its hop. it turns HEAD_FLIP a step,
 * through one whole turn */
#define HEAD_FLIP 0x800
/* the head's walk takes STRIDE action frames a stride. a foot comes down at
 * FOOT_DOWN and at STRIDE */
#define STRIDE 20.0F
#define FOOT_DOWN 10.0F
/* the head faces away once its heading is more than HEAD_AWAY off the way to
 * player 0 */
#define HEAD_AWAY 0x7000
/* the squared distance within which player 0 corners the head: about 4.5 */
#define HEAD_CORNERED 20.0F

/* a blast lives BLAST_STEPS steps after its first. on each, it freezes
 * player 0 if player 0 is within BLAST_REACH of it */
#define BLAST_STEPS 10
#define BLAST_REACH 10.0F

#define SE_ALERT 0x33f
#define SE_BLOW 0x33d
#define SE_SNOWMAN_MOVE 0x33c
#define SE_STEP 0x33e
#define SE_BEATEN 0x33b

static NJS_TEXNAME e_snowman_textures[16] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
};

NJS_TEXLIST e_snowman_texlist = {
#include "stg08/snowman/e_snowman_texlist.inc"
};

static NJS_MATERIAL snowman_foot_plus_x_materials[2] = {
#include "stg08/snowman/snowman_foot_plus_x_materials.inc"
};

static Sint16 snowman_foot_plus_x_meshes0[33] = {
#include "stg08/snowman/snowman_foot_plus_x_meshes0.inc"
};

static Sint16 snowman_foot_plus_x_meshes1[5] = {
#include "stg08/snowman/snowman_foot_plus_x_meshes1.inc"
};

static NJS_COLOR snowman_foot_plus_x_uvs0[28] = {
#include "stg08/snowman/snowman_foot_plus_x_uvs0.inc"
};

static NJS_COLOR snowman_foot_plus_x_uvs1[4] = {
#include "stg08/snowman/snowman_foot_plus_x_uvs1.inc"
};

static NJS_MESHSET snowman_foot_plus_x_meshsets[2] = {
#include "stg08/snowman/snowman_foot_plus_x_meshsets.inc"
};

static NJS_POINT3 snowman_foot_plus_x_points[12] = {
#include "stg08/snowman/snowman_foot_plus_x_points.inc"
};

static NJS_VECTOR snowman_foot_plus_x_normals[12] = {
#include "stg08/snowman/snowman_foot_plus_x_normals.inc"
};

static NJS_MODEL snowman_foot_plus_x_model = {
#include "stg08/snowman/snowman_foot_plus_x_model.inc"
};

static NJS_OBJECT snowman_foot_plus_x_object = {
#include "stg08/snowman/snowman_foot_plus_x_object.inc"
};

static NJS_MATERIAL snowman_foot_minus_x_materials[2] = {
#include "stg08/snowman/snowman_foot_minus_x_materials.inc"
};

static Sint16 snowman_foot_minus_x_meshes0[33] = {
#include "stg08/snowman/snowman_foot_minus_x_meshes0.inc"
};

static Sint16 snowman_foot_minus_x_meshes1[5] = {
#include "stg08/snowman/snowman_foot_minus_x_meshes1.inc"
};

static NJS_COLOR snowman_foot_minus_x_uvs0[28] = {
#include "stg08/snowman/snowman_foot_minus_x_uvs0.inc"
};

static NJS_COLOR snowman_foot_minus_x_uvs1[4] = {
#include "stg08/snowman/snowman_foot_minus_x_uvs1.inc"
};

static NJS_MESHSET snowman_foot_minus_x_meshsets[2] = {
#include "stg08/snowman/snowman_foot_minus_x_meshsets.inc"
};

static NJS_POINT3 snowman_foot_minus_x_points[12] = {
#include "stg08/snowman/snowman_foot_minus_x_points.inc"
};

static NJS_VECTOR snowman_foot_minus_x_normals[12] = {
#include "stg08/snowman/snowman_foot_minus_x_normals.inc"
};

static NJS_MODEL snowman_foot_minus_x_model = {
#include "stg08/snowman/snowman_foot_minus_x_model.inc"
};

static NJS_OBJECT snowman_foot_minus_x_object = {
#include "stg08/snowman/snowman_foot_minus_x_object.inc"
};

static NJS_MATERIAL snowman_head_materials[8] = {
#include "stg08/snowman/snowman_head_materials.inc"
};

static Sint16 snowman_head_meshes0[10] = {
#include "stg08/snowman/snowman_head_meshes0.inc"
};

static Sint16 snowman_head_meshes1[10] = {
#include "stg08/snowman/snowman_head_meshes1.inc"
};

static Sint16 snowman_head_meshes2[36] = {
#include "stg08/snowman/snowman_head_meshes2.inc"
};

static Sint16 snowman_head_meshes3[86] = {
#include "stg08/snowman/snowman_head_meshes3.inc"
};

static Sint16 snowman_head_meshes4[36] = {
#include "stg08/snowman/snowman_head_meshes4.inc"
};

static Sint16 snowman_head_meshes5[30] = {
#include "stg08/snowman/snowman_head_meshes5.inc"
};

static Sint16 snowman_head_meshes6[30] = {
#include "stg08/snowman/snowman_head_meshes6.inc"
};

static Sint16 snowman_head_meshes7[17] = {
#include "stg08/snowman/snowman_head_meshes7.inc"
};

static NJS_COLOR snowman_head_uvs0[8] = {
#include "stg08/snowman/snowman_head_uvs0.inc"
};

static NJS_COLOR snowman_head_uvs1[8] = {
#include "stg08/snowman/snowman_head_uvs1.inc"
};

static NJS_COLOR snowman_head_uvs2[33] = {
#include "stg08/snowman/snowman_head_uvs2.inc"
};

static NJS_COLOR snowman_head_uvs3[76] = {
#include "stg08/snowman/snowman_head_uvs3.inc"
};

static NJS_COLOR snowman_head_uvs4[33] = {
#include "stg08/snowman/snowman_head_uvs4.inc"
};

static NJS_COLOR snowman_head_uvs6[28] = {
#include "stg08/snowman/snowman_head_uvs6.inc"
};

static NJS_COLOR snowman_head_uvs7[14] = {
#include "stg08/snowman/snowman_head_uvs7.inc"
};

static NJS_MESHSET snowman_head_meshsets[8] = {
#include "stg08/snowman/snowman_head_meshsets.inc"
};

static NJS_POINT3 snowman_head_points[103] = {
#include "stg08/snowman/snowman_head_points.inc"
};

static NJS_VECTOR snowman_head_normals[103] = {
#include "stg08/snowman/snowman_head_normals.inc"
};

static NJS_MODEL snowman_head_model = {
#include "stg08/snowman/snowman_head_model.inc"
};

static NJS_OBJECT snowman_head_object = {
#include "stg08/snowman/snowman_head_object.inc"
};

static NJS_MKEY_F snowman_head_walk_pos0[18] = {
#include "stg08/snowman/snowman_head_walk_pos0.inc"
};

static NJS_MKEY_A snowman_head_walk_ang0[20] = {
#include "stg08/snowman/snowman_head_walk_ang0.inc"
};

static NJS_MKEY_A snowman_head_walk_ang1[20] = {
#include "stg08/snowman/snowman_head_walk_ang1.inc"
};

static NJS_MKEY_A snowman_head_walk_ang2[20] = {
#include "stg08/snowman/snowman_head_walk_ang2.inc"
};

static NJS_MDATA2 snowman_head_walk_mdata[3] = {
#include "stg08/snowman/snowman_head_walk_mdata.inc"
};

static NJS_MOTION snowman_head_walk_motion = {
#include "stg08/snowman/snowman_head_walk_motion.inc"
};

static NJS_ACTION snowman_head_walk_action = {
#include "stg08/snowman/snowman_head_walk_action.inc"
};

static NJS_MATERIAL snowman_tread_plus_x_materials[4] = {
#include "stg08/snowman/snowman_tread_plus_x_materials.inc"
};

static Sint16 snowman_tread_plus_x_meshes0[14] = {
#include "stg08/snowman/snowman_tread_plus_x_meshes0.inc"
};

static Sint16 snowman_tread_plus_x_meshes1[10] = {
#include "stg08/snowman/snowman_tread_plus_x_meshes1.inc"
};

static Sint16 snowman_tread_plus_x_meshes2[10] = {
#include "stg08/snowman/snowman_tread_plus_x_meshes2.inc"
};

static Sint16 snowman_tread_plus_x_meshes3[5] = {
#include "stg08/snowman/snowman_tread_plus_x_meshes3.inc"
};

static NJS_COLOR snowman_tread_plus_x_uvs0[12] = {
#include "stg08/snowman/snowman_tread_plus_x_uvs0.inc"
};

static NJS_COLOR snowman_tread_plus_x_uvs1[8] = {
#include "stg08/snowman/snowman_tread_plus_x_uvs1.inc"
};

static NJS_COLOR snowman_tread_plus_x_uvs2[8] = {
#include "stg08/snowman/snowman_tread_plus_x_uvs2.inc"
};

static NJS_COLOR snowman_tread_plus_x_uvs3[4] = {
#include "stg08/snowman/snowman_tread_plus_x_uvs3.inc"
};

static NJS_MESHSET snowman_tread_plus_x_meshsets[4] = {
#include "stg08/snowman/snowman_tread_plus_x_meshsets.inc"
};

static NJS_POINT3 snowman_tread_plus_x_points[12] = {
#include "stg08/snowman/snowman_tread_plus_x_points.inc"
};

static NJS_VECTOR snowman_tread_plus_x_normals[12] = {
#include "stg08/snowman/snowman_tread_plus_x_normals.inc"
};

static NJS_MODEL snowman_tread_plus_x_model = {
#include "stg08/snowman/snowman_tread_plus_x_model.inc"
};

static NJS_OBJECT snowman_tread_plus_x_object = {
#include "stg08/snowman/snowman_tread_plus_x_object.inc"
};

static NJS_MATERIAL snowman_tread_minus_x_materials[4] = {
#include "stg08/snowman/snowman_tread_minus_x_materials.inc"
};

static Sint16 snowman_tread_minus_x_meshes0[14] = {
#include "stg08/snowman/snowman_tread_minus_x_meshes0.inc"
};

static Sint16 snowman_tread_minus_x_meshes1[10] = {
#include "stg08/snowman/snowman_tread_minus_x_meshes1.inc"
};

static Sint16 snowman_tread_minus_x_meshes2[10] = {
#include "stg08/snowman/snowman_tread_minus_x_meshes2.inc"
};

static Sint16 snowman_tread_minus_x_meshes3[5] = {
#include "stg08/snowman/snowman_tread_minus_x_meshes3.inc"
};

static NJS_COLOR snowman_tread_minus_x_uvs0[12] = {
#include "stg08/snowman/snowman_tread_minus_x_uvs0.inc"
};

static NJS_COLOR snowman_tread_minus_x_uvs1[8] = {
#include "stg08/snowman/snowman_tread_minus_x_uvs1.inc"
};

static NJS_COLOR snowman_tread_minus_x_uvs2[8] = {
#include "stg08/snowman/snowman_tread_minus_x_uvs2.inc"
};

static NJS_COLOR snowman_tread_minus_x_uvs3[4] = {
#include "stg08/snowman/snowman_tread_minus_x_uvs3.inc"
};

static NJS_MESHSET snowman_tread_minus_x_meshsets[4] = {
#include "stg08/snowman/snowman_tread_minus_x_meshsets.inc"
};

static NJS_POINT3 snowman_tread_minus_x_points[12] = {
#include "stg08/snowman/snowman_tread_minus_x_points.inc"
};

static NJS_VECTOR snowman_tread_minus_x_normals[12] = {
#include "stg08/snowman/snowman_tread_minus_x_normals.inc"
};

static NJS_MODEL snowman_tread_minus_x_model = {
#include "stg08/snowman/snowman_tread_minus_x_model.inc"
};

static NJS_OBJECT snowman_tread_minus_x_object = {
#include "stg08/snowman/snowman_tread_minus_x_object.inc"
};

static NJS_MATERIAL snowman_lid_materials[1] = {
#include "stg08/snowman/snowman_lid_materials.inc"
};

static Sint16 snowman_lid_meshes[42] = {
#include "stg08/snowman/snowman_lid_meshes.inc"
};

static NJS_COLOR snowman_lid_uvs[38] = {
#include "stg08/snowman/snowman_lid_uvs.inc"
};

static NJS_MESHSET snowman_lid_meshsets[1] = {
#include "stg08/snowman/snowman_lid_meshsets.inc"
};

static NJS_POINT3 snowman_lid_points[19] = {
#include "stg08/snowman/snowman_lid_points.inc"
};

static NJS_VECTOR snowman_lid_normals[19] = {
#include "stg08/snowman/snowman_lid_normals.inc"
};

static NJS_MODEL snowman_lid_model = {
#include "stg08/snowman/snowman_lid_model.inc"
};

static NJS_OBJECT snowman_lid_object = {
#include "stg08/snowman/snowman_lid_object.inc"
};

static NJS_MATERIAL snowman_body_materials[14] = {
#include "stg08/snowman/snowman_body_materials.inc"
};

static Sint16 snowman_body_meshes0[8] = {
#include "stg08/snowman/snowman_body_meshes0.inc"
};

static Sint16 snowman_body_meshes1[14] = {
#include "stg08/snowman/snowman_body_meshes1.inc"
};

static Sint16 snowman_body_meshes2[18] = {
#include "stg08/snowman/snowman_body_meshes2.inc"
};

static Sint16 snowman_body_meshes3[19] = {
#include "stg08/snowman/snowman_body_meshes3.inc"
};

static Sint16 snowman_body_meshes4[12] = {
#include "stg08/snowman/snowman_body_meshes4.inc"
};

static Sint16 snowman_body_meshes5[9] = {
#include "stg08/snowman/snowman_body_meshes5.inc"
};

static Sint16 snowman_body_meshes6[40] = {
#include "stg08/snowman/snowman_body_meshes6.inc"
};

static Sint16 snowman_body_meshes7[5] = {
#include "stg08/snowman/snowman_body_meshes7.inc"
};

static Sint16 snowman_body_meshes8[10] = {
#include "stg08/snowman/snowman_body_meshes8.inc"
};

static Sint16 snowman_body_meshes9[10] = {
#include "stg08/snowman/snowman_body_meshes9.inc"
};

static Sint16 snowman_body_meshes10[7] = {
#include "stg08/snowman/snowman_body_meshes10.inc"
};

static Sint16 snowman_body_meshes11[5] = {
#include "stg08/snowman/snowman_body_meshes11.inc"
};

static Sint16 snowman_body_meshes12[24] = {
#include "stg08/snowman/snowman_body_meshes12.inc"
};

static Sint16 snowman_body_meshes13[7] = {
#include "stg08/snowman/snowman_body_meshes13.inc"
};

static NJS_COLOR snowman_body_uvs0[6] = {
#include "stg08/snowman/snowman_body_uvs0.inc"
};

static NJS_COLOR snowman_body_uvs2[16] = {
#include "stg08/snowman/snowman_body_uvs2.inc"
};

static NJS_COLOR snowman_body_uvs3[18] = {
#include "stg08/snowman/snowman_body_uvs3.inc"
};

static NJS_COLOR snowman_body_uvs4[10] = {
#include "stg08/snowman/snowman_body_uvs4.inc"
};

static NJS_COLOR snowman_body_uvs5[8] = {
#include "stg08/snowman/snowman_body_uvs5.inc"
};

static NJS_COLOR snowman_body_uvs7[4] = {
#include "stg08/snowman/snowman_body_uvs7.inc"
};

static NJS_COLOR snowman_body_uvs8[8] = {
#include "stg08/snowman/snowman_body_uvs8.inc"
};

static NJS_COLOR snowman_body_uvs9[8] = {
#include "stg08/snowman/snowman_body_uvs9.inc"
};

static NJS_COLOR snowman_body_uvs10[6] = {
#include "stg08/snowman/snowman_body_uvs10.inc"
};

static NJS_COLOR snowman_body_uvs11[4] = {
#include "stg08/snowman/snowman_body_uvs11.inc"
};

static NJS_COLOR snowman_body_uvs12[20] = {
#include "stg08/snowman/snowman_body_uvs12.inc"
};

static NJS_COLOR snowman_body_uvs13[6] = {
#include "stg08/snowman/snowman_body_uvs13.inc"
};

static NJS_MESHSET snowman_body_meshsets[14] = {
#include "stg08/snowman/snowman_body_meshsets.inc"
};

static NJS_POINT3 snowman_body_points[58] = {
#include "stg08/snowman/snowman_body_points.inc"
};

static NJS_VECTOR snowman_body_normals[58] = {
#include "stg08/snowman/snowman_body_normals.inc"
};

static NJS_MODEL snowman_body_model = {
#include "stg08/snowman/snowman_body_model.inc"
};

static NJS_OBJECT snowman_body_object = {
#include "stg08/snowman/snowman_body_object.inc"
};

/* snowman_step turns the second sphere off, and nothing turns it on */
/* clang-format off */
static struct collision_shape snowman_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS | ATTACK(1),
     PUSH_FIRM(PUSH_PLAYERS) | PUSH_ALL,
     {0.0F, 5.06F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS | ATTACK(1),
     PUSH_FIRM(PUSH_PLAYERS) | PUSH_ALL,
     {0.0F, 3.2F, 0.0F}, 3.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static NJS_MODEL *snowman_body_pieces[] = {
    &snowman_body_model,
    &snowman_lid_model,
    &snowman_tread_plus_x_model,
    &snowman_tread_minus_x_model,
    NULL,
};
static NJS_MODEL *snowman_head_pieces[] = {
    &snowman_head_model,
    &snowman_foot_minus_x_model,
    &snowman_foot_plus_x_model,
    NULL,
};

/* the set editor's lines. snowman_draw_range draws the red one a segment at
 * a time, as a ring whose radius is work->scl.x. nothing draws the blue
 * one */
static NJS_POINT3 snowman_blue_points[2];
static NJS_POINT3 snowman_range_points[2];
static NJS_BGRA snowman_blue_colors[2] = {
#include "stg08/snowman/snowman_blue_colors.inc"
};
static NJS_POINT3COL snowman_blue_line = {
#include "stg08/snowman/snowman_blue_line.inc"
};
static NJS_BGRA snowman_range_colors[2] = {
#include "stg08/snowman/snowman_range_colors.inc"
};
static NJS_POINT3COL snowman_range_line = {
#include "stg08/snowman/snowman_range_line.inc"
};

static NJS_TEXANIM snowman_snow_frames[8] = {
#include "stg08/snowman/snowman_snow_frames.inc"
};
static struct particle_anim snowman_snow_anim = {
    &icecap_pom_texlist, snowman_snow_frames, 8, NJD_COLOR_BLENDING_SRCALPHA,
    NJD_COLOR_BLENDING_ONE};
struct particle_pool_spec snowman_snow_pool = {&snowman_snow_anim, 0, NULL,
                                               NULL};

static struct dust_params snowman_side_snow = {
    1.0F,
    0.02F,
    0.1F,
    0.04F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.9F, 0.9F, 0.9F},
};
static struct dust_params snowman_trail_snow = {
    1.0F,
    0.01F,
    0.4F,
    0.04F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.9F, 0.9F, 0.9F},
};
static struct dust_params snowman_breath = {
    1.0F,
    0.07F,
    0.5F,
    0.04F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.9F, 0.9F, 0.9F},
};

static void snowman_destroy(struct task *task);
static void snowman_spray_sides(struct task_work *work,
                                struct lockon_target *record);
static void snowman_spray_trail(struct task_work *work,
                                struct lockon_target *record);
static void snowman_spray_breath(struct task_work *work,
                                 struct lockon_target *record);
static void snowman_spray_step_plus_x(struct task_work *work,
                                      struct lockon_target *record);
static void snowman_spray_step_minus_x(struct task_work *work,
                                       struct lockon_target *record);
static void snowman_blast_create(struct task_work *work,
                                 struct lockon_target *record);
static void snowman_blast_step(struct task *task);
static void snowman_run(struct task *task);
static void snowman_draw(struct task *task);
static void snowman_show(struct task_work *work, struct lockon_target *record);
static void snowman_body_step(struct task *task);
static void snowman_head_run(struct task *task);
static void snowman_head_draw(struct task *task);
static void snowman_head_show(struct task_work *work,
                              struct lockon_target *record);
static void snowman_body_show(struct task_work *work,
                              struct lockon_target *record);
static void snowman_edit(struct task *task);
static void snowman_draw_range(NJS_POINT3 *pos, Float radius);

void snowman_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    if (!placement_editing(task)) {
        record = enemy_alloc_record(task, work);
        collision_init(task, snowman_shapes, COLLISION_ENEMY, 1, 2);
        work->collision->shapes[0].flags &= ~SHAPE_OFF;
        work->collision->shapes[1].flags |= SHAPE_OFF;
        VARS(work)->sides_due = 0;
        VARS(work)->trail_due = 0;
        work->pos.y =
            ground_below(work->pos.x, work->pos.y, work->pos.z, &work->ang);
        VARS(work)->bob = 0.0F;
        HEAD_SPEED(record) = 0x200;
        record->turn_speed = 0x80;
        record->phase = 0;
        record->angles[HEAD] = work->ang.y;
        record->angles[BOB] = 0;
        record->angles[TILT] = 0;
        LIFT(record) = 0.0F;
        /* @hack redundant: RANGE set to scl.x just before SNOWMAN_RANGE
         * replaces it, a store SHC keeps (FMOV.S FR3,@(R0,R14) at
         * 0x0c910d40); without it, the function runs 8 bytes short and 68 of
         * 169 words differ */
        RANGE(record) = work->scl.x;
        RANGE(record) = SNOWMAN_RANGE;
        SWELL(record) = 1.0F;
        record->sight_angle = 0x2000;
        record->sight_distance = 80.0F * 80.0F;
        record->near_distance = 20.0F * 20.0F;
        record->flags |= ENEMY_KEEP_ON_LEDGE;
        record->flags |= ENEMY_SHADOW;
        record->probe_offset.x = 0.0F;
        record->probe_offset.y = 5.0F;
        record->probe_offset.z = 0.0F;
        record->bottom = -5.0F;
        record->radius = 4.0F;
        record->step_down = 20.0F;
        record->shadow_size = 1.5F;
        record->shadow_ratio = 1.0F;
        enemy_save_pos(work, record);
        enemy_set_home(work, record);
    }
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    SPRING_SPEED(work) = 0.0F;
    if (placement_editing(task)) {
        task->step = snowman_edit;
    } else {
        task->step = snowman_run;
    }
    task->destructor = snowman_destroy;
    task->draw = snowman_draw;
}

static void snowman_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    if (record != NULL) {
        enemy_end(task, work, record);
    }
}

static void snowman_mark_sight(struct task_work *work,
                               struct lockon_target *record)
{
    Angle heading;

    work->flags &= ~SNOWMAN_SEES;
    heading = work->ang.y;
    work->ang.y = record->angles[HEAD];
    if (enemy_sees_player(work, record) == 1) {
        work->flags |= SNOWMAN_SEES;
    }
    work->ang.y = heading;
}

static void snowman_take_hit(struct task_work *work,
                             struct lockon_target *record)
{
    if (enemy_take_hit(work, record)) {
        SNOWMAN_SET_MODE(work, SNOWMAN_HIT);
        work->tick = 0;
        SPRING_SPEED(work) = 0.35F;
        record->motion.speed.x = 0.0F;
        record->motion.speed.z = 0.0F;
        SWELL(record) = 1.0F;
    }
}

/* add the step's push, record->motion.accel, to the speed with drag and
 * gravity. then move by the speed, and clear the push */
static void snowman_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.accel.x += -record->motion.speed.x * MOVE_DRAG;
    record->motion.accel.y +=
        (-record->motion.speed.y * MOVE_DRAG) - MOVE_GRAVITY;
    record->motion.accel.z += -record->motion.speed.z * MOVE_DRAG;
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y += record->motion.accel.y;
    record->motion.speed.z += record->motion.accel.z;
    if (_builtin_fabsf(record->motion.speed.x) < MOVE_STILL) {
        record->motion.speed.x = 0.0F;
    }
    if (_builtin_fabsf(record->motion.speed.y) < MOVE_STILL) {
        record->motion.speed.y = 0.0F;
    }
    if (_builtin_fabsf(record->motion.speed.z) < MOVE_STILL) {
        record->motion.speed.z = 0.0F;
    }
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

/* turn the head toward aim_y by HEAD_SPEED, and hold it within HEAD_REACH */
static void snowman_turn_head(struct task_work *work,
                              struct lockon_target *record)
{
    int offset;

    record->angles[HEAD] =
        angle_approach(record->angles[HEAD], record->aim_y, HEAD_SPEED(record));
    /* @hack spelling: the offset narrowed in a statement of its own, which
     * reads the head's angle whole, by MOV.L (0x0c91104a); narrowed as it is
     * taken, SHC reads the angle's low half by MOV.W, and 4 of 49 words
     * differ */
    offset = record->angles[HEAD] - work->ang.y;
    offset = (Uint16)offset;
    if (offset > 0x8000) {
        /* @hack spelling: a turn taken off by adding -0x10000, which adds the
         * pooled -0x10000 (ADD R1,R4 at 0x0c91105a); as -= 0x10000, SHC
         * subtracts a pooled 0x10000 and 1 of 49 words differ */
        offset += -0x10000;
    }
    if (offset > HEAD_REACH) {
        offset = HEAD_REACH;
    } else if (offset < -HEAD_REACH) {
        offset = -HEAD_REACH;
    }
    record->angles[HEAD] = work->ang.y + offset;
}

static void snowman_mark_home(struct task_work *work,
                              struct lockon_target *record)
{
    if (enemy_home_distance(work, record) < RANGE(record)) {
        work->flags |= SNOWMAN_HOME;
        work->flags &= ~SNOWMAN_TURNED;
    } else {
        work->flags &= ~SNOWMAN_HOME;
    }
}

static void snowman_swing_head(struct task_work *work,
                               struct lockon_target *record)
{
    record->angles[HEAD] += HEAD_SPEED(record);
    if (HEAD_SPEED(record) > 0) {
        if (record->angles[HEAD] >= work->ang.y + HEAD_REACH) {
            record->angles[HEAD] = work->ang.y + HEAD_REACH;
            HEAD_SPEED(record) *= -1;
        }
    } else {
        if (record->angles[HEAD] <= work->ang.y - HEAD_REACH) {
            record->angles[HEAD] = work->ang.y - HEAD_REACH;
            HEAD_SPEED(record) *= -1;
        }
    }
}

static void snowman_bob(struct task_work *work, struct lockon_target *record)
{
    record->angles[BOB] += record->phase;
    LIFT(record) = njSin(record->angles[BOB]) * VARS(work)->bob;
}

/* scl.y springs back toward 1 at SPRING_SPEED, and scl.x takes the rest
 * of 2 */
static void snowman_spring(struct task_work *work)
{
    Float stretch = work->scl.y - 1.0F;

    SPRING_SPEED(work) += -(SPRING_SPEED(work) * 0.12F) - (stretch * 0.32F);
    work->scl.y += SPRING_SPEED(work);
    work->scl.x = 2.0F - work->scl.y;
}

/* throw the sprays owed, and meet the world. at a wall or a ledge, the
 * snowman turns for home */
static void snowman_collide(struct task_work *work,
                            struct lockon_target *record)
{
    while (VARS(work)->sides_due - SPRAY > 0) {
        snowman_spray_sides(work, record);
        VARS(work)->sides_due -= SPRAY;
    }
    while (VARS(work)->trail_due - SPRAY > 0) {
        snowman_spray_trail(work, record);
        VARS(work)->trail_due -= SPRAY;
    }
    enemy_collide(work, record);
    if ((record->flags & ENEMY_ON_WALL) || (record->flags & ENEMY_NO_LEDGE)) {
        SNOWMAN_SET_MODE(work, SNOWMAN_TURN);
        enemy_turn_home(work, record);
    }
    enemy_save_pos(work, record);
}

static void snowman_wait(struct task_work *work, struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    Float distance = SQUARE(player->pos.x - work->pos.x) +
                     SQUARE(player->pos.z - work->pos.z);

    if (distance < RANGE(record) * RANGE(record)) {
        SNOWMAN_SET_MODE(work, SNOWMAN_WAKE);
        sound_play(SE_ALERT, 0, 0, 0);
    }
    snowman_collide(work, record);
}

static void snowman_wake(struct task_work *work, struct lockon_target *record)
{
    switch (work->submode) {
    case 0:
        work->timer = 0;
        VARS(work)->bob = 0.3F;
        record->phase = 0x2000;
        work->submode++;
    case 1:
        VARS(work)->sides_due += SPRAY;
        VARS(work)->trail_due += SPRAY;
        if (TASK_TIMER(work)++ == 5) {
            SNOWMAN_SET_MODE(work, SNOWMAN_CHASE);
        }
        record->motion.speed.x += njCos(work->ang.y) * 0.08F;
        record->motion.speed.z += -(njSin(work->ang.y) * 0.08F);
        break;
    }
    snowman_bob(work, record);
    snowman_collide(work, record);
}

static void snowman_chase(struct task_work *work, struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    Float distance = SQUARE(player->pos.x - work->pos.x) +
                     SQUARE(player->pos.z - work->pos.z);

    /* @hack spelling: the submode tested by a switch of one case, which tests
     * it with CMP/EQ #0,R0 (0x0c911450); as an if, SHC tests it with TST R0,R0
     * and 1 of 149 words differ */
    switch (work->submode) {
    case 0:
        work->submode = 1;
        work->flags &= ~SNOWMAN_TURNED;
        VARS(work)->bob = 0.3F;
        HEAD_SPEED(record) = 0x200;
        record->turn_speed = 0x100;
        record->phase = 0x2000;
        break;
    }
    VARS(work)->sides_due += SPRAY / 32;
    VARS(work)->trail_due += SPRAY;
    snowman_mark_home(work, record);
    enemy_turn_to_player(work, record, 0);
    snowman_turn_head(work, record);
    snowman_mark_sight(work, record);
    if (distance < 40.0F * 40.0F &&
        angle_distance(record->angles[HEAD], record->aim_y) < 0x400 &&
        angle_distance(work->ang.y, record->aim_y) < 0x800) {
        record->target = player->pos;
        enemy_turn_to_target(work, record);
        SNOWMAN_SET_MODE(work, SNOWMAN_BLOW);
    }
    if (!(work->flags & SNOWMAN_SEES)) {
        SNOWMAN_SET_MODE(work, SNOWMAN_LOOK);
    }
    snowman_bob(work, record);
    record->motion.accel.x = njCos(work->ang.y) * 0.04F;
    record->motion.accel.z = -(njSin(work->ang.y) * 0.04F);
    snowman_move(work, record);
    snowman_collide(work, record);
}

/* out of RANGE of home, the snowman turns once, toward a point a unit from
 * home */
static void snowman_look_about(struct task_work *work,
                               struct lockon_target *record)
{
    /* @hack spelling: the submode tested by a switch of one case, which tests
     * it with CMP/EQ #0,R0 (0x0c9115b0); as an if, SHC tests it with TST R0,R0
     * and 1 of 135 words differ */
    switch (work->submode) {
    case 0:
        work->submode = 1;
        HEAD_SPEED(record) = 0x100;
        record->turn_speed = 0x80;
        break;
    }
    VARS(work)->sides_due += SPRAY / 64;
    VARS(work)->trail_due += SPRAY;
    snowman_mark_home(work, record);
    snowman_swing_head(work, record);
    snowman_mark_sight(work, record);
    if (work->flags & SNOWMAN_SEES) {
        SNOWMAN_SET_MODE(work, SNOWMAN_CHASE);
    } else if (!(work->flags & SNOWMAN_HOME)) {
        if (!(work->flags & SNOWMAN_TURNED)) {
            SNOWMAN_SET_MODE(work, SNOWMAN_TURN);
            work->flags |= SNOWMAN_TURNED;
            enemy_aim_at_home(work, record);
            record->aim_y += (Angle)(rand() * 0.5F) - 0x2000;
            record->target.x = njCos(record->aim_y) + record->home.x;
            record->target.z = njSin(record->aim_y) + record->home.z;
            enemy_aim_at_target(work, record);
        }
    }
    snowman_bob(work, record);
    record->motion.accel.x = njCos(work->ang.y) * (0.04F * 0.7F);
    record->motion.accel.z = -(njSin(work->ang.y) * (0.04F * 0.7F));
    snowman_move(work, record);
    snowman_collide(work, record);
}

static void snowman_turn(struct task_work *work, struct lockon_target *record)
{
    /* @hack spelling: the submode tested by a switch of one case, which tests
     * it with CMP/EQ #0,R0 (0x0c91172a); as an if, SHC tests it with TST R0,R0
     * and 1 of 73 words differ */
    switch (work->submode) {
    case 0:
        work->submode = 1;
        HEAD_SPEED(record) = 0x280;
        record->turn_speed = 0x180;
        break;
    }
    VARS(work)->sides_due += SPRAY / 16;
    VARS(work)->trail_due += SPRAY;
    snowman_mark_home(work, record);
    enemy_turn_to_target(work, record);
    snowman_turn_head(work, record);
    if (angle_distance(record->aim_y, work->ang.y) == 0) {
        SNOWMAN_SET_MODE(work, SNOWMAN_LOOK);
    }
    snowman_mark_sight(work, record);
    if (work->flags & SNOWMAN_SEES) {
        SNOWMAN_SET_MODE(work, SNOWMAN_CHASE);
    }
    snowman_bob(work, record);
    snowman_move(work, record);
    snowman_collide(work, record);
}

static void snowman_hit(struct task_work *work, struct lockon_target *record)
{
    if (work->tick++ > 15) {
        SNOWMAN_SET_MODE(work, SNOWMAN_BEATEN);
    }
}

/* face record->target, and draw a breath. hold it, then blow it out in puffs
 * and blasts, and rest */
static void snowman_blow(struct task_work *work, struct lockon_target *record)
{
    switch (work->submode) {
    case 0:
        work->submode = 1;
        HEAD_SPEED(record) = 0x200;
        record->turn_speed = 0x100;
        SWELL(record) = 1.0F;
        work->tick = 0;
    case 1:
        VARS(work)->sides_due += SPRAY / 16;
        VARS(work)->trail_due += SPRAY;
        if (enemy_turn_to_target(work, record) == 0) {
            work->submode = 2;
        }
        snowman_turn_head(work, record);
        break;
    case 2:
        VARS(work)->sides_due += SPRAY / 64;
        VARS(work)->trail_due += SPRAY / 2;
        SWELL(record) += 0.05F;
        if (work->tick++ == 10) {
            work->submode = 3;
            work->tick = 0;
        }
        break;
    case 3:
        if (work->tick++ == 15) {
            work->submode = 4;
            work->tick = 0;
        }
        break;
    case 4:
        SWELL(record) -= 1.0F / 120.0F;
        if (work->tick++ == 60) {
            work->tick = 0;
            SWELL(record) = 1.0F;
            work->submode = 5;
        }
        snowman_spray_breath(work, record);
        snowman_spray_breath(work, record);
        if (!(work->tick & 7)) {
            snowman_blast_create(work, record);
        }
        sound_play_for(SE_BLOW, (int)work, 1, 0, 2);
        break;
    case 5:
        if (work->tick++ == 30) {
            SNOWMAN_SET_MODE(work, SNOWMAN_LOOK);
        }
        break;
    }
    snowman_move(work, record);
    snowman_collide(work, record);
}

/* make the beaten snowman's task the head's. from here on, record->target
 * keeps the body's position, and angles[BODY] its heading */
static void snowman_head_init(struct task_work *work,
                              struct lockon_target *record)
{
    record->angles[BODY] = work->ang.y;
    record->target = work->pos;
    /* @hack spelling: 5.8 spelled 5.8000002F. SHC rounds a float literal
     * toward 0, so this gives the Float nearest 5.8, which the image holds at
     * 0x0c911a50; as 5.8F, it comes out an ulp lower, and 0 of 70 words and 1
     * of 36 other words differ */
    work->pos.y += 5.8000002F;
    work->ang.y = record->angles[HEAD];
    record->motion.speed.x = 0.0F;
    record->motion.speed.y = 0.0F;
    record->motion.speed.z = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 3.2F;
    record->probe_offset.z = 0.0F;
    record->bottom = -3.2F;
    record->radius = 3.7F;
    record->shadow_size = 1.0F;
    record->shadow_ratio = 1.0F;
    record->action = &snowman_head_walk_action;
    record->frame = 0.0F;
    enemy_save_pos(work, record);
    enemy_collide(work, record);
}

/* the snowman's step. once it is beaten, the task becomes the head's, and a
 * child task the body's */
static void snowman_run(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    if (placement_try_leave(task)) {
        return;
    }
    snowman_take_hit(work, record);
    switch (work->mode) {
    case SNOWMAN_WAIT:
        snowman_wait(work, record);
        break;
    case SNOWMAN_WAKE:
        snowman_wake(work, record);
        break;
    /* @hack spelling: SNOWMAN_LOOK's case above SNOWMAN_CHASE's, which puts the
     * call of snowman_look_about first (BSR at 0x0c911b22); in the enum's
     * order, the two calls trade places and 4 of 151 words differ */
    case SNOWMAN_LOOK:
        snowman_look_about(work, record);
        sound_play_at_for(SE_SNOWMAN_MOVE, (int)work, 1, 50, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        break;
    case SNOWMAN_CHASE:
        snowman_chase(work, record);
        sound_play_at_for(SE_SNOWMAN_MOVE, (int)work, 1, 50, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        break;
    case SNOWMAN_TURN:
        snowman_turn(work, record);
        sound_play_at_for(SE_SNOWMAN_MOVE, (int)work, 1, 50, work->pos.x,
                          work->pos.y, work->pos.z, 2);
        break;
    case SNOWMAN_HIT:
        snowman_hit(work, record);
        break;
    case SNOWMAN_BLOW:
        snowman_blow(work, record);
        break;
    case SNOWMAN_BEATEN:
        task->step = snowman_head_run;
        task->draw = snowman_head_draw;
        snowman_head_init(work, record);
        work->mode = HEAD_START;
        work->submode = 0;
        task_create_child(TASK_WORK, snowman_body_step, task);
        snowman_head_show(work, record);
        snowman_body_show(work, record);
        collision_register(work);
        sound_play(SE_BEATEN, 0, 0, 0);
        return;
    }
    snowman_spring(work);
    snowman_show(work, record);
    collision_register(work);
}

static void snowman_draw(struct task *task)
{
    snowman_show(task->work, LOCKON_TARGET(task));
}

static void snowman_show(struct task_work *work, struct lockon_target *record)
{
    Float swell = SWELL(record);
    NJS_OBJECT *object;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + 0x4000);
    njScale(NULL, 1.0F, 1.0F, 1.0F);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    njSetTexture(&e_snowman_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 2.4F + ((LIFT(record) + 6.6F) * swell), 0.0F);
    TURN(njRotateY, record->angles[HEAD] - work->ang.y);
    draw_tree(&snowman_head_object);
    njPopMatrix(1);
    object = &snowman_body_object;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1] * swell, object->pos[2]);
    njTranslate(NULL, 0.0F, LIFT(record), 0.0F);
    njPushMatrix(NULL);
    njScale(NULL, swell, swell, swell);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->child;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0] * swell, object->pos[1] * swell,
                object->pos[2] * swell);
    njScale(NULL, swell, 1.0F, swell);
    draw_model(object->model);
    njPopMatrix(1);
    njTranslate(NULL, 0.0F, -LIFT(record), 0.0F);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1] * swell, object->pos[2]);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1] * swell, object->pos[2]);
    njRotateY(NULL, 0x8000);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void snowman_head_collide(struct task_work *work,
                                 struct lockon_target *record)
{
    enemy_collide(work, record);
    if (record->flags & ENEMY_ON_WALL) {
        record->motion.speed.x *= -1.0F;
        record->motion.speed.z *= -1.0F;
        enemy_turn_to_player(work, record, 0);
    } else if (record->flags & ENEMY_NO_LEDGE) {
        enemy_turn_to_player(work, record, 0);
    }
    enemy_save_pos(work, record);
}

/* a hurt head stops and wobbles. when a player hurt it, player 0 bounces up,
 * whichever player it was */
static void snowman_head_take_hit(struct task_work *work,
                                  struct lockon_target *record)
{
    struct collision_work *hit = work->collision->hit;

    if (work->flags & COLLISION_DAMAGED) {
        SNOWMAN_SET_MODE(work, HEAD_HIT);
        work->tick = 0;
        SPRING_SPEED(work) = 0.35F;
        record->motion.speed.x = 0.0F;
        record->motion.speed.z = 0.0F;
        if (hit->kind == COLLISION_PLAYER) {
            player_set_speed(0, 0.0F, 2.0F, 0.0F);
        }
    }
}

/* walking under a push, the head plays its stride at a rate its speed sets.
 * each foot that comes down then throws a spray. with no push, it plays on at
 * 1 a step to the next foot down. it stops there, throwing no spray */
static void snowman_head_animate(struct task_work *work,
                                 struct lockon_target *record)
{
    Float speed = njSqrt(SQUARE(record->motion.speed.x) +
                         SQUARE(record->motion.speed.z)) *
                  1.5F;

    if (work->mode == HEAD_WALK) {
        if (record->motion.accel.x == 0.0F && record->motion.accel.z == 0.0F) {
            speed = 1.0F;
            if (record->frame != 0.0F && record->frame != FOOT_DOWN) {
                record->frame += speed;
                if (record->frame >= STRIDE) {
                    record->frame = 0.0F;
                } else if (record->frame >= FOOT_DOWN &&
                           record->frame < FOOT_DOWN + 1.0F) {
                    record->frame = FOOT_DOWN;
                }
            }
        } else {
            if (speed < 0.4F) {
                speed = 0.4F;
            }
            record->frame += speed;
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y + 0x4000);
            if (PREVIOUS_FRAME(record) < FOOT_DOWN &&
                record->frame >= FOOT_DOWN) {
                snowman_spray_step_minus_x(work, record);
                sound_play_at(SE_STEP, 0, 0, 30, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            if (PREVIOUS_FRAME(record) < STRIDE && record->frame >= STRIDE) {
                snowman_spray_step_plus_x(work, record);
                record->frame -= STRIDE;
                sound_play_at(SE_STEP, 0, 0, 30, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            njPopMatrix(1);
        }
        PREVIOUS_FRAME(record) = record->frame;
    }
}

/* the body's step, as a child of the head's task. the body bursts 31 steps
 * after the lid is open */
static void snowman_body_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *head = task->parent->work;
    struct lockon_target *record = LOCKON_TARGET(task->parent);

    switch (work->mode) {
    case 0:
        if (record->angles[TILT] == LID_OPEN) {
            work->mode++;
        }
        break;
    case 1:
        if (TASK_TIMER(work)++ == 30) {
            explosion_create_at(record->target.x, record->target.y + 5.0F,
                                record->target.z, 2.0F);
            enemy_break(work, snowman_body_pieces, &e_snowman_texlist);
            head->flags |= SNOWMAN_BURST;
        }
        break;
    }
}

static void snowman_head_out(struct task_work *work,
                             struct lockon_target *record)
{
    switch (work->submode) {
    case 0:
        record->angles[HEAD] = 0;
        work->timer = 0;
        work->submode++;
    case 1:
        if (TASK_TIMER(work)++ == 30) {
            work->timer = 0;
            work->submode++;
        }
        break;
    case 2:
        record->angles[TILT] =
            angle_approach(record->angles[TILT], -LID_TILT, LID_TURN);
        if (record->angles[TILT] == LID_OPEN) {
            work->submode++;
        }
        break;
    case 3:
        if (TASK_TIMER(work)++ == 10) {
            record->motion.accel.y = 2.4F;
            work->submode++;
        }
        break;
    case 4:
        if (_builtin_fabsf(record->motion.speed.y) < 0.1F) {
            work->submode++;
            record->angles[HEAD] += HEAD_FLIP;
        }
        snowman_move(work, record);
        break;
    case 5:
        if (record->flags & ENEMY_ON_GROUND) {
            SNOWMAN_SET_MODE(work, HEAD_WALK);
        }
        snowman_move(work, record);
        break;
    }
    if (record->angles[HEAD]) {
        record->angles[HEAD] += HEAD_FLIP;
        if (record->angles[HEAD] == 0x10000) {
            record->angles[HEAD] = 0;
        }
    }
    snowman_head_collide(work, record);
}

/* the head runs, then stands until player 0 comes near. it then turns its
 * back on player 0 and runs again */
static void snowman_head_walk(struct task_work *work,
                              struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    Float distance;
    int off;

    VARS(work)->sides_due += SPRAY / 8;
    distance = SQUARE(player->pos.x - work->pos.x) +
               SQUARE(player->pos.z - work->pos.z);
    switch (work->submode) {
    case 0:
        work->submode = 1;
        record->turn_speed = 0x400;
        work->timer = 0;
        break;
    case 1:
        if (TASK_TIMER(work)++ == 120) {
            work->submode = 2;
            work->timer = 0;
            sound_play_at(SE_ALERT, 0, 0, 50, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        record->motion.accel.x = njCos(work->ang.y) * 0.08F;
        record->motion.accel.z = -(njSin(work->ang.y) * 0.08F);
        break;
    case 2:
        if (TASK_TIMER(work)++ >= 20 && distance < 70.0F * 70.0F) {
            work->submode = 1;
            work->timer = 0;
            off = enemy_aim_at_player(work, record, 0);
            /* facing away already, with player 0 within HEAD_CORNERED, it
             * veers aside. otherwise it aims straight away from player 0 */
            if (off > HEAD_AWAY && distance < HEAD_CORNERED) {
                if (rand() * RAND_SCALE < 0.5F) {
                    record->aim_y = work->ang.y + 0x3000;
                } else {
                    record->aim_y = work->ang.y - 0x3000;
                }
            } else {
                record->aim_y += 0x8000;
            }
        }
        break;
    }
    work->ang.y =
        angle_approach(work->ang.y, record->aim_y, record->turn_speed);
    snowman_head_animate(work, record);
    snowman_move(work, record);
    snowman_head_collide(work, record);
}

static void snowman_head_hit(struct task_work *work,
                             struct lockon_target *record)
{
    if (work->tick++ > 15) {
        SNOWMAN_SET_MODE(work, SNOWMAN_BEATEN);
    }
}

static void snowman_head_explode(struct task_work *work,
                                 struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_break(work, snowman_head_pieces, &e_snowman_texlist);
    animal_create_random(1, work->pos.x, work->pos.y + 15.0F, work->pos.z);
}

static void snowman_head_run(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    if (placement_try_leave(task)) {
        return;
    }
    snowman_head_take_hit(work, record);
    switch (work->mode) {
    case HEAD_START:
        work->collision->flags |= COLLISION_TARGET;
        SNOWMAN_SET_MODE(work, HEAD_OUT);
    case HEAD_OUT:
        snowman_head_out(work, record);
        break;
    case HEAD_WALK:
        snowman_head_walk(work, record);
        break;
    case HEAD_HIT:
        snowman_head_hit(work, record);
        break;
    case SNOWMAN_BEATEN:
        snowman_head_explode(work, record);
        placement_kill(task);
        return;
    }
    task_step_children(task);
    snowman_spring(work);
    snowman_head_show(work, record);
    if (!(work->flags & SNOWMAN_BURST)) {
        snowman_body_show(work, record);
    }
    collision_register(work);
}

static void snowman_head_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    snowman_head_show(work, record);
    if (!(work->flags & SNOWMAN_BURST)) {
        snowman_body_show(work, record);
    }
}

static void snowman_head_show(struct task_work *work,
                              struct lockon_target *record)
{
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njSetTexture(&e_snowman_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, 1.0F, 1.0F, 1.0F);
    njTranslate(NULL, 0.0F, 3.2F, 0.0F);
    TURN(njRotateY, work->ang.y + 0x4000);
    TURN(njRotateX, record->angles[HEAD]);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    draw_action(record->action, record->frame);
    njPopMatrix(1);
}

static void snowman_body_show(struct task_work *work,
                              struct lockon_target *record)
{
    NJS_OBJECT *object;

    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &record->target);
    TURN(njRotateY, record->angles[BODY] + 0x4000);
    njScale(NULL, 1.0F, 1.0F, 1.0F);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    object = &snowman_body_object;
    njSetTexture(&e_snowman_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    draw_model(object->model);
    object = object->child;
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    TURN(njRotateX, record->angles[TILT]);
    draw_model(object->model);
    njPopMatrix(1);
    njTranslate(NULL, 0.0F, -LIFT(record), 0.0F);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    njRotateY(NULL, 0x8000);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* the set editor's step. its ring's radius is scl.x, though the snowman's
 * range is SNOWMAN_RANGE whatever scl.x holds */
static void snowman_edit(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&e_snowman_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + 0x4000);
    njScale(NULL, 1.0F, 1.0F, 1.0F);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 9.0F, 0.0F);
    draw_tree(&snowman_head_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    draw_tree(&snowman_body_object);
    njPopMatrix(1);
    njPopMatrix(1);
    snowman_draw_range(&work->pos, work->scl.x);
}

static void snowman_draw_range(NJS_POINT3 *pos, Float radius)
{
    Angle ang;
    int i;

    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, pos->y + 5.0F, pos->z);
    ang = 0;
    snowman_range_points[0].x = njSin(ang) * radius;
    snowman_range_points[0].z = njCos(ang) * radius;
    for (i = 0; i < RANGE_SEGMENTS; i++) {
        /* @hack spelling: ang advanced by RANGE_TURN within njSin's argument,
         * which keeps njSin's address in R11 and njCos's in R10 (0x0c912a58,
         * 0x0c912a60); advanced in a statement of its own, the two trade and
         * 8 of 96 words differ */
        snowman_range_points[1].x = njSin(ang += RANGE_TURN) * radius;
        snowman_range_points[1].z = njCos(ang) * radius;
        njDrawLine3D(&snowman_range_line, 1, 0);
        snowman_range_points[0].x = snowman_range_points[1].x;
        snowman_range_points[0].z = snowman_range_points[1].z;
    }
    njPopMatrix(1);
}

/* a puff's step. it plays its frames at params->rate, and ends after the
 * last. it spins on the way it started, and params say how it grows, slows
 * and rises */
static void snowman_snow_step(struct particle *particle,
                              struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;
    Float drag;
    char *speed;
    char *pos;
    Float x;
    Float y;
    Float z;
    Float px;
    Float py;
    Float pz;

    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += params->rate;
    particle->ang += particle->ang > 0 ? 256 : -256;
    particle->scale += params->growth;
    /* @hack spelling: speed and pos, each read whole through a byte cursor
     * before any is written back, which steps down for the stores; through the
     * members, the loads and stores are ordered differently, the function
     * runs 32 bytes long and 42 of 74 words differ */
    speed = (char *)&particle->speed;
    drag = 1.0F - params->drag;
    x = *(Float *)speed;
    speed += sizeof(Float);
    y = *(Float *)speed;
    speed += sizeof(Float);
    x *= drag;
    y *= drag;
    z = *(Float *)speed;
    speed += sizeof(Float);
    z *= drag;
    y += params->lift;
    *(Float *)(speed -= sizeof(Float)) = z;
    *(Float *)(speed -= sizeof(Float)) = y;
    *(Float *)(speed -= sizeof(Float)) = x;
    pos = (char *)&particle->pos;
    px = *(Float *)pos;
    pos += sizeof(Float);
    py = *(Float *)pos;
    pos += sizeof(Float);
    pz = *(Float *)pos;
    pos += sizeof(Float);
    px += x;
    py += y;
    pz += z;
    *(Float *)(pos -= sizeof(Float)) = pz;
    *(Float *)(pos -= sizeof(Float)) = py;
    *(Float *)(pos -= sizeof(Float)) = px;
}

/* a puff from snowman_snow_pool, at scale and with params. pos and speed are
 * in the current matrix's space. when particle_alloc fails, there is none */
static void snowman_snow_create(struct dust_params *params, NJS_POINT3 *pos,
                                NJS_VECTOR *speed, Float scale)
{
    struct particle *particle;

    /* @hack spelling: particle assigned in the test, which tests
     * particle_alloc's R0 with the copy to R14 in the BT/S's slot
     * (0x0c912c3a, 0x0c912c3e); assigned apart, SHC tests R14 and 3 of 73
     * words differ */
    if ((particle = particle_alloc(snowman_snow_pool.pool,
                                   snowman_snow_step)) != NULL) {
        particle->time = rand() * (2.0F * RAND_SCALE);
        particle->scale = scale;
        particle->ang = (Angle)RANDOM_ANGLE() - 0x8000;
        DUST(particle)->params = params;
        particle->color = params->color;
        njCalcVector(NULL, speed, &particle->speed);
        njCalcPoint(NULL, pos, &particle->pos);
    }
}

static void snowman_spray_sides(struct task_work *work,
                                struct lockon_target *record)
{
    NJS_VECTOR speed;
    NJS_POINT3 pos;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + 0x4000);
    speed.x = 0.2F + (rand() * (0.1F * RAND_SCALE));
    speed.y = 0.4F + (rand() * (0.2F * RAND_SCALE));
    speed.z = -0.5F;
    pos.x = 2.0F;
    pos.y = 2.0F;
    pos.z = 0.0F;
    snowman_snow_create(&snowman_side_snow, &pos, &speed,
                        0.1F + (rand() * (0.05F * RAND_SCALE)));
    speed.x = (-(Float)rand() * (0.1F * RAND_SCALE)) - 0.2F;
    speed.y = 0.4F + (rand() * (0.2F * RAND_SCALE));
    speed.z = -0.5F;
    pos.x = -2.0F;
    pos.y = 2.0F;
    pos.z = 0.0F;
    snowman_snow_create(&snowman_side_snow, &pos, &speed,
                        0.1F + (rand() * (0.05F * RAND_SCALE)));
    njPopMatrix(1);
}

static void snowman_spray_trail(struct task_work *work,
                                struct lockon_target *record)
{
    NJS_VECTOR speed;
    NJS_POINT3 pos;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + 0x4000);
    speed.x = rand() * (0.1F * RAND_SCALE);
    speed.y = 0.1F + (rand() * (0.1F * RAND_SCALE));
    speed.z = 0.0F;
    pos.x = 3.0F;
    pos.y = 2.0F;
    pos.z = (-0.5F + (rand() * RAND_SCALE)) * 10.0F;
    snowman_snow_create(&snowman_trail_snow, &pos, &speed,
                        0.1F + (rand() * (0.05F * RAND_SCALE)));
    speed.x = -(Float)rand() * (0.1F * RAND_SCALE);
    speed.y = 0.1F + (rand() * (0.1F * RAND_SCALE));
    speed.z = 0.0F;
    pos.x = -3.0F;
    pos.y = 2.0F;
    pos.z = (-0.5F + (rand() * RAND_SCALE)) * 10.0F;
    snowman_snow_create(&snowman_trail_snow, &pos, &speed,
                        0.1F + (rand() * (0.05F * RAND_SCALE)));
    njPopMatrix(1);
}

static void snowman_spray_breath(struct task_work *work,
                                 struct lockon_target *record)
{
    NJS_VECTOR speed;
    NJS_POINT3 pos;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + 0x4000);
    speed.x = (-0.5F + (rand() * RAND_SCALE)) * 0.5F;
    speed.y = 0.3F + ((-0.5F + (rand() * RAND_SCALE)) * 0.5F);
    speed.z = 3.0F + (rand() * (2.5F * RAND_SCALE));
    pos.x = 0.0F;
    pos.y = SWELL(record) * 4.6F;
    pos.z = SWELL(record) * 4.0F;
    snowman_snow_create(&snowman_breath, &pos, &speed,
                        0.05F + (rand() * (0.05F * RAND_SCALE)));
    njPopMatrix(1);
}

/* the foot sprays work in the matrix snowman_head_animate sets, at the head
 * and turned its way */
static void snowman_spray_step_plus_x(struct task_work *work,
                                      struct lockon_target *record)
{
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    Float pace = njScalor(&record->motion.speed);

    speed.x = pace * 0.2F;
    speed.y = 0.1F + (rand() * (0.2F * RAND_SCALE));
    speed.z = pace * 0.8F;
    pos.x = 0.0F;
    pos.y = 1.0F;
    pos.z = 0.0F;
    snowman_snow_create(&snowman_side_snow, &pos, &speed,
                        0.02F + (rand() * (0.02F * RAND_SCALE)));
}

static void snowman_spray_step_minus_x(struct task_work *work,
                                       struct lockon_target *record)
{
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    Float pace = njScalor(&record->motion.speed);

    speed.x = -(pace * 0.2F);
    speed.y = 0.1F + (rand() * (0.2F * RAND_SCALE));
    speed.z = pace * 0.8F;
    pos.x = 0.0F;
    pos.y = 1.0F;
    pos.z = 0.0F;
    snowman_snow_create(&snowman_side_snow, &pos, &speed,
                        0.02F + (rand() * (0.02F * RAND_SCALE)));
}

static void snowman_blast_create(struct task_work *work,
                                 struct lockon_target *record)
{
    struct task *blast;
    NJS_VECTOR speed;
    NJS_POINT3 pos;

    /* @hack spelling: blast assigned in the test, which tests task_create's R0
     * with the copy to R14 in the BT/S's slot (0x0c9132b4, 0x0c9132b8);
     * assigned apart, SHC tests R14 and 3 of 83 words differ */
    if ((blast = task_create(TASK_MOTION | TASK_WORK, 6, snowman_blast_step)) !=
        NULL) {
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y + 0x4000);
        speed.x = 0.0F;
        speed.y = 0.0F;
        speed.z = 3.0F;
        pos.x = 0.0F;
        pos.y = SWELL(record) * 4.6F;
        pos.z = SWELL(record) * 4.0F;
        njCalcPoint(NULL, &pos, &blast->work->pos);
        njCalcVector(NULL, &speed, &blast->motion->speed);
        njPopMatrix(1);
    }
}

/* a blast of the breath, which flies on at its speed */
static void snowman_blast_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    switch (work->mode) {
    case 0:
        work->timer = BLAST_STEPS;
        work->mode = 1;
        break;
    case 1:
        if (work->timer == 0) {
            task_destroy(task);
            return;
        }
        work->timer--;
        if (player_in_range(&work->pos, BLAST_REACH) == 1) {
            player_set_mode(0, PLAYER_MODE_FREEZE);
        }
        break;
    }
    work->pos.x += motion->speed.x;
    work->pos.y += motion->speed.y;
    work->pos.z += motion->speed.z;
}
