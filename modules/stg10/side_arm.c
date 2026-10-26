/* O SIDE_ARM robot arm that reaches out and back, turned toward the nearest
 * player, and bursts apart when hit */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>
#include <string.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg10/side_arm.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* an array, as side_arm_fixed.c needs it. the generated action is an array
 * only where every declaration gives it an extent */
extern NJS_ACTION side_arm_action[];
/* @hack type: the hand and claws as arrays. SHC then loads claw 2's address
 * ahead of the test of ARM_BURST and claw 0's after it (0x0c90501c,
 * 0x0c905026). as NJS_OBJECTs, side_arm_draw runs 32 bytes long and 305 of its
 * 332 words differ */
extern NJS_OBJECT side_arm_hand_object[];
extern NJS_OBJECT side_arm_claw_object_0[];
extern NJS_OBJECT side_arm_claw_object_1[];
extern NJS_OBJECT side_arm_claw_object_2[];

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* how far the collision record reaches from the arm's position */
#define REACH 140.0F

/* the arm plays its action from frame 0 to this one and back. the action's
 * frames past it, to 59, never play */
#define FRAME_LAST 38.0F
#define FRAME_OUT_SPEED 0.4F /* the action's frames a step, going out */
#define FRAME_BACK_SPEED 0.8F /* the action's frames a step, coming back */

/* the action's nodes that the arm reads, by their order in its tree. the tree
 * holds the claws in the reverse order of their names, each with its tip after
 * it */
#define NODE_HAND 6
#define NODE_CLAW_2 7
#define NODE_TIP_2 8
#define NODE_CLAW_1 9
#define NODE_TIP_1 10
#define NODE_CLAW_0 11
#define NODE_TIP_0 12
#define NODE_LAST 12 /* the node the pose stops at */

/* the sway's phase is burst->sway << SWAY_SHIFT, a whole turn each 32 steps */
#define SWAY_SHIFT 11
/* how far the sway turns the arm either way, while it holds at FRAME_LAST */
#define SWAY 2048.0F

#define PIECES 4 /* the pieces the arm bursts into */
#define BURST_STEPS 220 /* the steps the pieces fly before they vanish */

/* the arm's sound as it reaches FRAME_LAST, and the burst's. both play under
 * id 0, so the sound_stop_id calls, which pass the task, never stop them */
#define SE_ARM 297
#define SE_BURST 25

#define PIECE_LIFT 1.0F /* a piece's upward launch speed at a scale of 1 */
#define PIECE_GRAVITY 0.027222222F /* the fall speed a piece gains a step */
/* the shares of its speed and of its spin a piece keeps a step */
#define PIECE_SPEED_KEPT 0.99F
#define PIECE_SPIN_KEPT 0.9F

struct side_arm_piece {
    NJS_MATRIX matrix; /* its node's matrix as the arm burst */
    NJS_POINT3 pos; /* how far it has flown from that node */
    NJS_VECTOR speed;
    struct angles ang; /* its turn from that node's matrix */
    struct angles spin;
};

/* the arm's facing and sway, and its burst's timer, shake and pieces. it is
 * allocated from the heap, which doesn't clear it */
struct side_arm_burst {
    Sint32 timer; /* steps since the burst */
    Angle roll; /* the arm's shake about z as it bursts */
    Float shake; /* how far the shake turns the arm, which dies away */
    Angle yaw; /* the way the arm faces */
    Sint32 sway; /* steps since the arm last came back to frame 0 */
    struct side_arm_piece pieces[PIECES];
};

struct side_arm_part {
    NJS_OBJECT *object;
    Uint32 node; /* the action node it flies off from */
};

struct arm_vars {
    Float frame;
    Float scale; /* a tenth of scale x, held from 0.04 to 1 */
    struct side_arm_burst *burst;
};

#define VARS(work) ((struct arm_vars *)&(work)->vars)

/* in work->tick: the arm has burst */
#define ARM_BURST 0x1

enum arm_mode { ARM_INIT, ARM_RUN };

/* the arm's phase, in work->submode */
enum arm_phase {
    ARM_OUT,
    ARM_AT_END,
    ARM_BACK,
    ARM_AT_START,
    ARM_BURSTING,
    ARM_GONE
};

static void side_arm_init(struct task *task);
static void side_arm_reset(struct task *task);
static void side_arm_run(struct task *task);
static void side_arm_draw(struct task *task);
static void side_arm_destroy(struct task *task);

/* the arm starts back once it has held at FRAME_LAST more than this many
 * steps */
static Sint32 side_arm_end_hold = 60;
/* the arm starts out again once it has held at frame 0 more than this many
 * steps */
static Sint32 side_arm_start_hold = 40;
/* the radius of a claw tip's shape at a scale of 1 */
static Float side_arm_claw_radius = 7.0F;
/* the radius of the hand's shape at a scale of 1 */
static Float side_arm_hand_radius = 18.0F;
/* how far the burst first shakes the arm about z */
static Float side_arm_shake = 512.0F;
/* the turn of the shake's phase each step, a quarter turn */
static Sint32 side_arm_shake_speed = 0x4000;
/* the share of its shake the arm keeps each step */
static Float side_arm_shake_kept = 0.95F;
/* the step of the burst on which a second blast goes off */
static Sint32 side_arm_blast_step = 30;
/* the most the arm turns toward the player each step */
static Angle side_arm_turn_step = 0x80;
/* the most the arm turns either way from the way its set entry faces */
static Angle side_arm_turn_limit = 0x2000;
/* the chance each step that a piece gives off smoke */
static Float side_arm_smoke_odds = 0.2F;

/* the arm's shapes, all spheres. side_arm_run puts each center at its node, in
 * the world, each step */
static struct collision_shape side_arm_shapes[5] = {
    /* the hand. an attack hurts a shape only past its guard, which is 0 here
     * and 2 on the claws */
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE | SHAPE_IGNORE_ENEMIES |
         SHAPE_IGNORE_OBJECTS,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     18.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    /* the tips of claws 2, 1 and 0, which hurt players */
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE | SHAPE_IGNORE_ENEMIES |
         SHAPE_IGNORE_OBJECTS,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     7.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE | SHAPE_IGNORE_ENEMIES |
         SHAPE_IGNORE_OBJECTS,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     7.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE | SHAPE_IGNORE_ENEMIES |
         SHAPE_IGNORE_OBJECTS,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     7.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    /* a second sphere at the hand, which hurts objects. it ignores players,
     * attacks, items and enemies */
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE | SHAPE_IGNORE_PLAYERS |
         SHAPE_IGNORE_ENEMIES,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_OBJECTS,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     45.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the pieces the arm bursts into, each with the node it flies off from. claws
 * 0 and 2 fly off from each other's nodes */
static struct side_arm_part side_arm_parts[PIECES] = {
    {side_arm_hand_object, NODE_HAND},
    {side_arm_claw_object_0, NODE_CLAW_2},
    {side_arm_claw_object_1, NODE_CLAW_1},
    {side_arm_claw_object_2, NODE_CLAW_0},
};

/* params for a piece's smoke. side_arm_run writes its pos, speed and scale
 * before each dust_create_from */
static struct dust_params side_arm_smoke = {
    1.0F,
    0.1F,
    0.4F,
    0.62F,
    0.07F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* a node's origin, where a shape's center goes */
static NJS_POINT3 side_arm_node_point = {0.0F, 0.0F, 0.0F};
/* where a piece starts, at its node */
static NJS_POINT3 side_arm_piece_start = {0.0F, 0.0F, 0.0F};
/* a piece's ang as it starts: no turn from its node */
static struct angles side_arm_piece_ang = {0, 0, 0};
/* where a piece's smoke comes from, at the piece */
static NJS_POINT3 side_arm_smoke_point = {0.0F, 0.0F, 0.0F};

/* a piece's node matrix, copied from its record for njMultiMatrix */
static NJS_MATRIX side_arm_matrix;

static void side_arm_free(struct task *task)
{
    heap_free(VARS(task->work)->burst);
}

void side_arm_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ARM_INIT:
            side_arm_init(task);
            break;
        case ARM_RUN:
            side_arm_draw(task);
            side_arm_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ARM_INIT:
            side_arm_init(task);
            break;
        case ARM_RUN:
            side_arm_free(task);
            side_arm_reset(task);
            side_arm_draw(task);
            side_arm_run(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCALE(*10)");
            break;
        }
    }
}

/* the enemy record lets E-102 lock on to the arm */
static void side_arm_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = side_arm_draw;
    task->destructor = side_arm_destroy;
    enemy_alloc_record(task, work);
    VARS(work)->frame = 0.0F;
    work->submode = ARM_OUT;
    collision_init(task, side_arm_shapes, COLLISION_ITEM, 1, 5);
    work->collision->radius = REACH;
    side_arm_reset(task);
    work->mode = ARM_RUN;
}

/* give the arm a new burst record, facing as its set entry does. clear
 * ARM_BURST and turn the claws' shapes on. a caller that holds a record frees
 * it first */
static void side_arm_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct side_arm_burst *burst;

    burst = VARS(work)->burst = heap_alloc(sizeof(struct side_arm_burst));
    VARS(work)->scale = work->scl.x * 0.1F;
    if (VARS(work)->scale < 0.04F) {
        VARS(work)->scale = 0.04F;
    } else if (VARS(work)->scale > 1.0F) {
        VARS(work)->scale = 1.0F;
    }
    collision->shapes[0].radius = side_arm_hand_radius * VARS(work)->scale;
    collision->shapes[1].radius = side_arm_claw_radius * VARS(work)->scale;
    collision->shapes[2].radius = side_arm_claw_radius * VARS(work)->scale;
    collision->shapes[3].radius = side_arm_claw_radius * VARS(work)->scale;
    work->tick = 0;
    burst->roll = 0;
    burst->yaw = work->ang.y;
    collision->shapes[1].flags &= ~SHAPE_OFF;
    collision->shapes[2].flags &= ~SHAPE_OFF;
    collision->shapes[3].flags &= ~SHAPE_OFF;
}

/* until the arm bursts, turn it toward the nearest player, and play its
 * action out and back. it sways while it holds at FRAME_LAST. move each shape
 * to its node. once a shape is hurt, release E-102's lock-on and burst. the
 * pieces fly for BURST_STEPS steps as the arm shakes */
static void side_arm_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct side_arm_burst *burst = VARS(work)->burst;
    struct side_arm_piece *piece;
    NJS_VECTOR toward;
    NJS_VECTOR point;
    Angle ang;
    /* a Sint16: the image sign-extends the heading before it adds the sway
     * (EXTS.W at 0x0c9042a4) */
    Sint16 yaw;
    Sint16 off;
    Float scale;
    int i;

    if (work->submode == ARM_GONE) {
        return;
    }
    if (work->submode == ARM_OUT || work->submode == ARM_AT_END ||
        work->submode == ARM_BACK || work->submode == ARM_AT_START) {
        i = player_nearest(&work->pos);
        toward = player_work[i]->pos;
        njSubVector(&toward, &work->pos);
        yaw = atan2f(toward.z, -toward.x) * ANGLE_PER_RADIAN;
        off = (Sint16)(yaw - work->ang.y);
        if (off > side_arm_turn_limit) {
            yaw = work->ang.y + side_arm_turn_limit;
        } else if (off < -side_arm_turn_limit) {
            yaw = work->ang.y - side_arm_turn_limit;
        }
        if (work->submode == ARM_AT_END) {
            /* @bug uninitialized: the arm's first hold sways from a phase the
             * heap left (MOV.L @(16,R8),R4 at 0x0c904298). side_arm_reset
             * stores no sway, so its count runs on from whatever heap_alloc's
             * block held until the arm first comes back to frame 0 */
            yaw += njSin(burst->sway << SWAY_SHIFT) * SWAY;
        }
        off = (Sint16)(yaw - burst->yaw);
        if (off > side_arm_turn_step) {
            burst->yaw += side_arm_turn_step;
        } else if (off < -side_arm_turn_step) {
            burst->yaw -= side_arm_turn_step;
        } else {
            burst->yaw = yaw;
        }
        burst->sway++;
    }
    if (work->submode == ARM_OUT) {
        VARS(work)->frame += FRAME_OUT_SPEED;
        if (VARS(work)->frame >= FRAME_LAST) {
            VARS(work)->frame = FRAME_LAST;
            work->submode = ARM_AT_END;
            work->timer = 0;
            sound_play(SE_ARM, 0, 0, 0);
        }
    } else if (work->submode == ARM_AT_END) {
        if (TASK_TIMER(work) > side_arm_end_hold) {
            work->submode = ARM_BACK;
        } else {
            work->timer++;
        }
    } else if (work->submode == ARM_BACK) {
        VARS(work)->frame -= FRAME_BACK_SPEED;
        if (VARS(work)->frame < 0.0F) {
            VARS(work)->frame = 0.0F;
            work->submode = ARM_AT_START;
            work->timer = 0;
            burst->sway = 0;
            sound_stop_id((int)task);
        }
    } else if (work->submode == ARM_AT_START) {
        if (TASK_TIMER(work) > side_arm_start_hold) {
            work->submode = ARM_OUT;
        } else {
            work->timer++;
        }
    }
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, burst->yaw, ang);
    scale = VARS(work)->scale;
    njScale(NULL, scale, scale, scale);
    pose_action(NULL, side_arm_action, VARS(work)->frame, NODE_LAST, NULL);
    pose_get_matrix(NODE_TIP_2, NULL);
    njCalcPoint(NULL, &side_arm_node_point, &collision->shapes[1].center);
    pose_get_matrix(NODE_TIP_1, NULL);
    njCalcPoint(NULL, &side_arm_node_point, &collision->shapes[2].center);
    pose_get_matrix(NODE_TIP_0, NULL);
    njCalcPoint(NULL, &side_arm_node_point, &collision->shapes[3].center);
    pose_get_matrix(NODE_HAND, NULL);
    njCalcPoint(NULL, &side_arm_node_point, &collision->shapes[0].center);
    njCalcPoint(NULL, &side_arm_node_point, &collision->shapes[4].center);
    if ((work->flags & COLLISION_DAMAGED) && !(work->tick & ARM_BURST)) {
        sound_stop_id((int)task);
        lockon_release(task);
        collision->shapes[0].levels |= SHAPE_GUARD;
        work->tick |= ARM_BURST;
        work->submode = ARM_BURSTING;
        work->timer = 0;
        burst->shake = side_arm_shake;
        burst->timer = 0;
        collision->shapes[1].flags |= SHAPE_OFF;
        collision->shapes[2].flags |= SHAPE_OFF;
        collision->shapes[3].flags |= SHAPE_OFF;
        bomb_create(&collision->shapes[0].center, VARS(work)->scale * 0.9F);
        sound_play(SE_BURST, 0, 0, 0);
        piece = burst->pieces;
        for (i = 0; i < PIECES; i++) {
            pose_get_matrix(side_arm_parts[i].node, &piece->matrix);
            piece->pos = side_arm_piece_start;
            /* the hand flies up, spinning at random. each claw flies up too,
             * and out along its offset from the hand, spinning about z */
            if (i == 0) {
                piece->speed.x = 0.0F;
                piece->speed.y = VARS(work)->scale * PIECE_LIFT;
                piece->speed.z = 0.0F;
                piece->spin.x = ((rand() * (1.0F / 16384.0F)) - 1.0F) * 1024.0F;
                piece->spin.y = ((rand() * (1.0F / 16384.0F)) - 1.0F) * 1024.0F;
                piece->spin.z = ((rand() * (1.0F / 16384.0F)) - 1.0F) * 1024.0F;
            } else {
                pose_get_matrix(side_arm_parts[0].node, NULL);
                njCalcVector(NULL, (NJS_VECTOR *)side_arm_parts[i].object->pos,
                             &point);
                piece->speed.x = point.x * 0.07F;
                piece->speed.y =
                    (point.y * 0.07F) + (VARS(work)->scale * PIECE_LIFT);
                piece->speed.z = point.z * 0.07F;
                piece->spin.x = 0;
                piece->spin.y = 0;
                piece->spin.z = 0x400;
            }
            piece->ang = side_arm_piece_ang;
            piece++;
        }
        /* @hack redundant: stores to point.y and point.z that never run. SHC
         * then keeps 1024.0 in the pool at 0x0c9045ec, and FR13 for the copy
         * at 0x0c904636; without them, the function runs 10 bytes short and 960
         * of 1465 words differ */
        if (0) {
            point.y = ((point.x * (1.0F / 16384.0F)) - 1.0F) * 1024.0F;
            point.z = (point.x * (1.0F / 16384.0F)) - 1.0F;
        }
    }
    njPopMatrix(1);
    if (work->submode == ARM_BURSTING) {
        burst->roll =
            burst->shake * njSin(TASK_TIMER(work) * side_arm_shake_speed);
        burst->shake *= side_arm_shake_kept;
        if (burst->timer == side_arm_blast_step) {
            bomb_create(&collision->shapes[0].center, VARS(work)->scale * 0.7F);
            sound_play(SE_BLAST, 0, 0, 0);
        }
        burst->timer++;
        if (burst->timer >= BURST_STEPS) {
            work->submode = ARM_GONE;
        }
        piece = burst->pieces;
        for (i = 0; i < PIECES; i++) {
            njAddVector(&piece->pos, &piece->speed);
            piece->speed.y -= PIECE_GRAVITY;
            piece->ang.x += piece->spin.x;
            piece->ang.y += piece->spin.y;
            piece->ang.z += piece->spin.z;
            /* @hack spelling: speed.x and spin.y and z scaled by *=, and the
             * others by products spelled out. SHC then keeps side_arm_matrix's
             * address in the frame twice, at @(8,R15) and @(12,R15), and the
             * speed's y in FR2 (0x0c904af4, 0x0c904b00, 0x0c904b54). spelled
             * out throughout, 54 of 1465 words differ. with *= throughout, 540
             * do */
            piece->speed.x *= PIECE_SPEED_KEPT;
            piece->speed.y = piece->speed.y * PIECE_SPEED_KEPT;
            piece->speed.z = piece->speed.z * PIECE_SPEED_KEPT;
            piece->spin.x = piece->spin.x * PIECE_SPIN_KEPT;
            piece->spin.y *= PIECE_SPIN_KEPT;
            piece->spin.z *= PIECE_SPIN_KEPT;
            if (side_arm_smoke_odds > rand() * RAND_SCALE) {
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &piece->pos);
                memcpy(&side_arm_matrix, piece->matrix, sizeof(NJS_MATRIX));
                njMultiMatrix(NULL, &side_arm_matrix);
                njCalcPoint(NULL, &side_arm_smoke_point, &point);
                njPopMatrix(1);
                side_arm_smoke.scale = VARS(work)->scale * 2.7F;
                side_arm_smoke.speed = piece->speed;
                side_arm_smoke.pos = point;
                dust_create_from(&side_arm_smoke);
            }
            piece++;
        }
        /* @hack redundant: stores to point and a memcpy of it that never run,
         * which keep memcpy's address in R9 and njAddVector's in R8
         * (0x0c904af8, 0x0c904afc); without them, the two trade and 160 of 1465
         * words differ */
        if (0) {
            point.x = RAND_SCALE;
            point.y = PIECE_GRAVITY;
            memcpy(&point, &point, sizeof(point));
        }
        work->timer++;
    }
    collision_register(work);
}

static void side_arm_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct side_arm_burst *burst = VARS(work)->burst;
    struct side_arm_piece *piece;
    Angle ang;
    Float scale;
    int i;

    if (frame_step == 0) {
        texture_set_stage();
        if (work->tick & ARM_BURST) {
            side_arm_hand_object->evalflags |= NJD_EVAL_HIDE;
            side_arm_claw_object_0->evalflags |= NJD_EVAL_HIDE;
            side_arm_claw_object_1->evalflags |= NJD_EVAL_HIDE;
            side_arm_claw_object_2->evalflags |= NJD_EVAL_HIDE;
        } else {
            side_arm_hand_object->evalflags &= ~NJD_EVAL_HIDE;
            side_arm_claw_object_0->evalflags &= ~NJD_EVAL_HIDE;
            side_arm_claw_object_1->evalflags &= ~NJD_EVAL_HIDE;
            side_arm_claw_object_2->evalflags &= ~NJD_EVAL_HIDE;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, burst->yaw, ang);
        TURN_VIA(njRotateZ, burst->roll, ang);
        scale = VARS(work)->scale;
        njScale(NULL, scale, scale, scale);
        draw_action(side_arm_action, VARS(work)->frame);
        njPopMatrix(1);
        if (work->submode == ARM_BURSTING) {
            piece = burst->pieces;
            for (i = 0; i < PIECES; i++) {
                njPushMatrix(NULL);
                njTranslateV(NULL, &piece->pos);
                memcpy(&side_arm_matrix, piece->matrix, sizeof(NJS_MATRIX));
                njMultiMatrix(NULL, &side_arm_matrix);
                TURN_VIA(njRotateX, piece->ang.x, ang);
                TURN_VIA(njRotateY, piece->ang.y, ang);
                TURN_VIA(njRotateZ, piece->ang.z, ang);
                draw_model(side_arm_parts[i].object->model);
                njPopMatrix(1);
                piece++;
            }
        }
    }
}

static void side_arm_destroy(struct task *task)
{
    sound_stop_id((int)task);
    side_arm_free(task);
}
