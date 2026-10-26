/* O UP_ARM robot arm that reaches out and back, and bursts apart when hit */
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
#include "sound/sound.h"
#include "stg10/up_arm.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* an array, as up_arm_fixed.c needs it. the generated action is an array
 * only where every declaration gives it an extent */
extern NJS_ACTION up_arm_action[];
/* @hack type: the hand and claws as arrays. SHC then pools their addresses in
 * the order claw 2, claw 0, the hand and claw 1 (0x0c903cec to 0x0c903cf8).
 * as NJS_OBJECTs, 20 of up_arm_draw's 325 words differ */
extern NJS_OBJECT up_arm_hand_object[];
extern NJS_OBJECT up_arm_claw_object_0[];
extern NJS_OBJECT up_arm_claw_object_1[];
extern NJS_OBJECT up_arm_claw_object_2[];

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* how far the collision record reaches from the arm's position */
#define REACH 200.0F

/* the arm plays its action from frame 0 to this one and back. the action's
 * frames past it, to 29, never play */
#define FRAME_LAST 13.0F
#define FRAME_OUT_SPEED 0.2F /* the action's frames a step, going out */
#define FRAME_BACK_SPEED 0.4F /* the action's frames a step, coming back */

/* the action's nodes that the arm reads, by their order in its tree. the tree
 * holds the claws in the reverse order of their names, each with its tip after
 * it */
#define NODE_HAND_PARENT 3
#define NODE_HAND 4
#define NODE_CLAW_2 5
#define NODE_TIP_2 6
#define NODE_CLAW_1 7
#define NODE_TIP_1 8
#define NODE_CLAW_0 9
#define NODE_LAST 9 /* the node the pose stops at */

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

struct up_arm_piece {
    NJS_MATRIX matrix; /* its node's matrix as the arm burst */
    NJS_POINT3 pos; /* how far it has flown from that node */
    NJS_VECTOR speed;
    struct angles ang; /* its turn from that node's matrix */
    struct angles spin;
};

/* allocated from the heap */
struct up_arm_burst {
    Sint32 timer; /* steps since the burst */
    struct up_arm_piece pieces[PIECES];
};

struct up_arm_part {
    NJS_OBJECT *object;
    Uint32 node; /* the action node it flies off from */
};

struct arm_vars {
    Float frame;
    Float scale; /* a tenth of scale x, held from 0.04 to 1 */
    struct up_arm_burst *burst;
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

static void up_arm_init(struct task *task);
static void up_arm_reset(struct task *task);
static void up_arm_run(struct task *task);
static void up_arm_draw(struct task *task);
static void up_arm_destroy(struct task *task);

/* the radius of a claw's shape at a scale of 1 */
static Float up_arm_claw_radius = 14.0F;
/* the radius of the hand's shape at a scale of 1 */
static Float up_arm_hand_radius = 54.0F;
/* the arm starts back once it has held at FRAME_LAST more than this many
 * steps */
static Sint32 up_arm_end_hold = 60;
/* the arm starts out again once it has held at frame 0 more than this many
 * steps */
static Sint32 up_arm_start_hold = 40;
/* the step of the burst on which a second blast goes off */
static Sint32 up_arm_blast_step = 30;
/* the chance each step that a piece gives off smoke */
static Float up_arm_smoke_odds = 0.25F;

/* the arm's shapes, all spheres. up_arm_run puts each center at its node, in
 * the world, each step */
static struct collision_shape up_arm_shapes[4] = {
    /* the hand, centered on its parent node. an attack hurts a shape only past
     * its guard, which is 0 here and 2 on the claws */
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     7.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    /* claws 2, 1 and 0, which hurt players */
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     14.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     14.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     14.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the pieces the arm bursts into, each with the node it flies off from. none
 * is its own. the hand flies off from its parent and claw 0 from the hand.
 * claws 1 and 2 fly off from the tips of claws 2 and 1 */
static struct up_arm_part up_arm_parts[PIECES] = {
    {up_arm_hand_object, NODE_HAND_PARENT},
    {up_arm_claw_object_0, NODE_HAND},
    {up_arm_claw_object_1, NODE_TIP_2},
    {up_arm_claw_object_2, NODE_TIP_1},
};

/* params for a piece's smoke. up_arm_run writes its pos, speed and scale
 * before each dust_create_from */
static struct dust_params up_arm_smoke = {
    1.0F,
    0.1F,
    0.2F,
    0.62F,
    0.07F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* a node's origin, where a shape's center goes */
static NJS_POINT3 up_arm_node_point = {0.0F, 0.0F, 0.0F};
/* where a piece starts, at its node */
static NJS_POINT3 up_arm_piece_start = {0.0F, 0.0F, 0.0F};
/* a piece's ang as it starts: no turn from its node */
static struct angles up_arm_piece_ang = {0, 0, 0};
/* where a piece's smoke comes from, at the piece */
static NJS_POINT3 up_arm_smoke_point = {0.0F, 0.0F, 0.0F};

/* a piece's node matrix, copied from its record for njMultiMatrix */
static NJS_MATRIX up_arm_matrix;

static void up_arm_free(struct task *task)
{
    heap_free(VARS(task->work)->burst);
}

void up_arm_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ARM_INIT:
            up_arm_init(task);
            break;
        case ARM_RUN:
            up_arm_draw(task);
            up_arm_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ARM_INIT:
            up_arm_init(task);
            break;
        case ARM_RUN:
            up_arm_free(task);
            up_arm_reset(task);
            up_arm_draw(task);
            up_arm_run(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCALE(*10)");
            break;
        }
    }
}

/* the enemy record lets E-102 lock on to the arm */
static void up_arm_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = up_arm_draw;
    task->destructor = up_arm_destroy;
    enemy_alloc_record(task, work);
    VARS(work)->frame = 0.0F;
    work->submode = ARM_OUT;
    collision_init(task, up_arm_shapes, COLLISION_ITEM, 1, 4);
    work->collision->radius = REACH;
    up_arm_reset(task);
    work->mode = ARM_RUN;
}

/* give the arm a new burst record, clear ARM_BURST and turn the claws' shapes
 * on. a caller that holds a record frees it first */
static void up_arm_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;

    VARS(work)->burst = heap_alloc(sizeof(struct up_arm_burst));
    VARS(work)->scale = work->scl.x * 0.1F;
    if (VARS(work)->scale < 0.04F) {
        VARS(work)->scale = 0.04F;
    } else if (VARS(work)->scale > 1.0F) {
        VARS(work)->scale = 1.0F;
    }
    collision->shapes[0].radius = up_arm_hand_radius * VARS(work)->scale;
    collision->shapes[1].radius = up_arm_claw_radius * VARS(work)->scale;
    collision->shapes[2].radius = up_arm_claw_radius * VARS(work)->scale;
    collision->shapes[3].radius = up_arm_claw_radius * VARS(work)->scale;
    work->tick = 0;
    collision->shapes[1].flags &= ~SHAPE_OFF;
    collision->shapes[2].flags &= ~SHAPE_OFF;
    collision->shapes[3].flags &= ~SHAPE_OFF;
}

/* play the arm's action out and back until it bursts, and move each shape to
 * its node. once a shape is hurt, release E-102's lock-on and burst. the
 * pieces fly for BURST_STEPS steps */
static void up_arm_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct up_arm_burst *burst = VARS(work)->burst;
    struct up_arm_piece *piece;
    NJS_VECTOR point;
    Angle ang;
    Float scale;
    int i;

    if (work->submode == ARM_GONE) {
        return;
    }
    if (work->submode == ARM_OUT) {
        VARS(work)->frame += FRAME_OUT_SPEED;
        if (VARS(work)->frame >= FRAME_LAST) {
            /* @hack redundant: stores of FRAME_LAST to point that never run.
             * SHC then keeps burst in the frame at @(8,R15) and i's byte
             * offset into up_arm_parts at @(12,R15) (0x0c903024, 0x0c9033ce);
             * without them, the two trade, the function runs 40 bytes long and
             * 560 of 1318 words differ */
            if (0) {
                point.x = FRAME_LAST;
                point.y = FRAME_LAST;
            }
            VARS(work)->frame = FRAME_LAST;
            work->submode = ARM_AT_END;
            work->timer = 0;
            sound_play(SE_ARM, 0, 0, 0);
        }
    } else if (work->submode == ARM_AT_END) {
        if (TASK_TIMER(work) > up_arm_end_hold) {
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
            sound_stop_id((int)task);
        }
    } else if (work->submode == ARM_AT_START) {
        if (TASK_TIMER(work) > up_arm_start_hold) {
            work->submode = ARM_OUT;
        } else {
            work->timer++;
        }
    }
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = VARS(work)->scale;
    njScale(NULL, scale, scale, scale);
    pose_action(NULL, up_arm_action, VARS(work)->frame, NODE_LAST, NULL);
    pose_get_matrix(NODE_HAND_PARENT, NULL);
    njCalcPoint(NULL, &up_arm_node_point, &collision->shapes[0].center);
    pose_get_matrix(NODE_CLAW_2, NULL);
    njCalcPoint(NULL, &up_arm_node_point, &collision->shapes[1].center);
    pose_get_matrix(NODE_CLAW_1, NULL);
    njCalcPoint(NULL, &up_arm_node_point, &collision->shapes[2].center);
    pose_get_matrix(NODE_CLAW_0, NULL);
    njCalcPoint(NULL, &up_arm_node_point, &collision->shapes[3].center);
    if ((work->flags & COLLISION_DAMAGED) && !(work->tick & ARM_BURST)) {
        sound_stop_id((int)task);
        lockon_release(task);
        collision->shapes[0].levels |= SHAPE_GUARD;
        work->tick |= ARM_BURST;
        work->submode = ARM_BURSTING;
        work->timer = 0;
        burst->timer = 0;
        collision->shapes[1].flags |= SHAPE_OFF;
        collision->shapes[2].flags |= SHAPE_OFF;
        collision->shapes[3].flags |= SHAPE_OFF;
        bomb_create(&collision->shapes[0].center, VARS(work)->scale * 2.7F);
        sound_play(SE_BURST, 0, 0, 0);
        piece = burst->pieces;
        for (i = 0; i < PIECES; i++) {
            pose_get_matrix(up_arm_parts[i].node, &piece->matrix);
            piece->pos = up_arm_piece_start;
            /* @hack redundant: a store of 1024.0 to point.z that never runs.
             * SHC then pools 1/16384 ahead of 1024.0 (0x0c9032e8, 0x0c9032f4)
             * and keeps the two in FR12 and FR15; without it, the two trade
             * and 34 of 1318 words differ */
            if (0) {
                point.z = 1024.0F;
            }
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
                pose_get_matrix(up_arm_parts[0].node, NULL);
                njCalcVector(NULL, (NJS_VECTOR *)up_arm_parts[i].object->pos,
                             &point);
                piece->speed.x = point.x * 0.07F;
                piece->speed.y =
                    (point.y * 0.07F) + (VARS(work)->scale * PIECE_LIFT);
                piece->speed.z = point.z * 0.07F;
                piece->spin.x = 0;
                piece->spin.y = 0;
                piece->spin.z = 0x400;
            }
            piece->ang = up_arm_piece_ang;
            piece++;
        }
    }
    njPopMatrix(1);
    if (work->submode == ARM_BURSTING) {
        if (burst->timer == up_arm_blast_step) {
            bomb_create(&collision->shapes[0].center, VARS(work)->scale * 2.1F);
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
            /* @hack redundant: reads of speed.x, spin.y and spin.z that change
             * nothing, ahead of their scaling. SHC counts them as uses, which
             * keeps burst in the frame at @(8,R15) (0x0c903024). without
             * them, burst moves to @(12,R15), the function runs 40 bytes long
             * and 583 of 1318 words differ */
            /* @hack spelling: speed.x and spin.y and z scaled by *=, and the
             * others by products spelled out, which with those reads keeps
             * burst at @(8,R15) too. either spelling throughout moves burst to
             * @(12,R15) and runs the function 40 bytes long. spelled out
             * throughout, 582 of 1318 words differ. with *= throughout, 583
             * do */
            (void)piece->speed.x;
            piece->speed.x *= PIECE_SPEED_KEPT;
            piece->speed.y = piece->speed.y * PIECE_SPEED_KEPT;
            piece->speed.z = piece->speed.z * PIECE_SPEED_KEPT;
            piece->spin.x = piece->spin.x * PIECE_SPIN_KEPT;
            (void)piece->spin.y;
            piece->spin.y *= PIECE_SPIN_KEPT;
            (void)piece->spin.z;
            piece->spin.z *= PIECE_SPIN_KEPT;
            if (up_arm_smoke_odds > rand() * RAND_SCALE) {
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &piece->pos);
                memcpy(&up_arm_matrix, piece->matrix, sizeof(NJS_MATRIX));
                njMultiMatrix(NULL, &up_arm_matrix);
                njCalcPoint(NULL, &up_arm_smoke_point, &point);
                njPopMatrix(1);
                up_arm_smoke.scale = VARS(work)->scale * 5.0F;
                up_arm_smoke.speed = piece->speed;
                up_arm_smoke.pos = point;
                dust_create_from(&up_arm_smoke);
            }
            piece++;
        }
        /* @hack redundant: stores to point and an njAddVector of it that never
         * run. SHC then pools 1/16384 at 0x0c9032e8 and keeps it in FR12, and
         * keeps -1.0 in FR13, which each random spin copies (FMOV FR13,FR2 at
         * 0x0c903324); without them, 206 of 1318 words differ */
        if (0) {
            point.x = 5.0F;
            point.y = PIECE_GRAVITY;
            njAddVector(&point, &point);
        }
        work->timer++;
    }
    collision_register(work);
}

static void up_arm_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct up_arm_burst *burst = VARS(work)->burst;
    struct up_arm_piece *piece;
    Angle ang;
    Float scale;
    int i;

    if (frame_step == 0) {
        texture_set_stage();
        if (work->tick & ARM_BURST) {
            up_arm_hand_object->evalflags |= NJD_EVAL_HIDE;
            up_arm_claw_object_0->evalflags |= NJD_EVAL_HIDE;
            up_arm_claw_object_1->evalflags |= NJD_EVAL_HIDE;
            up_arm_claw_object_2->evalflags |= NJD_EVAL_HIDE;
        } else {
            up_arm_hand_object->evalflags &= ~NJD_EVAL_HIDE;
            up_arm_claw_object_0->evalflags &= ~NJD_EVAL_HIDE;
            up_arm_claw_object_1->evalflags &= ~NJD_EVAL_HIDE;
            up_arm_claw_object_2->evalflags &= ~NJD_EVAL_HIDE;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        scale = VARS(work)->scale;
        njScale(NULL, scale, scale, scale);
        draw_action(up_arm_action, VARS(work)->frame);
        njPopMatrix(1);
        if (work->submode == ARM_BURSTING) {
            piece = burst->pieces;
            for (i = 0; i < PIECES; i++) {
                njPushMatrix(NULL);
                njTranslateV(NULL, &piece->pos);
                memcpy(&up_arm_matrix, piece->matrix, sizeof(NJS_MATRIX));
                njMultiMatrix(NULL, &up_arm_matrix);
                TURN_VIA(njRotateX, piece->ang.x, ang);
                TURN_VIA(njRotateY, piece->ang.y, ang);
                TURN_VIA(njRotateZ, piece->ang.z, ang);
                draw_model(up_arm_parts[i].object->model);
                njPopMatrix(1);
                piece++;
            }
        }
    }
}

static void up_arm_destroy(struct task *task)
{
    sound_stop_id((int)task);
    up_arm_free(task);
}
