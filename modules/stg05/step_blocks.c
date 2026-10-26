/* O BREAKSTEP's twelve pieces as block tasks, in four groups of six. no
 * object list or code refers to the root's step */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg05/step_blocks.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT break_step_piece_0_object;
extern NJS_OBJECT break_step_piece_1_object;
extern NJS_OBJECT break_step_piece_2_object;
extern NJS_OBJECT break_step_piece_3_object;
extern NJS_OBJECT break_step_piece_4_object;
extern NJS_OBJECT break_step_piece_5_object;
extern NJS_OBJECT break_step_piece_6_object;
extern NJS_OBJECT break_step_piece_7_object;
extern NJS_OBJECT break_step_piece_8_object;
extern NJS_OBJECT break_step_piece_9_object;
extern NJS_OBJECT break_step_piece_10_object;
extern NJS_OBJECT break_step_piece_11_object;

struct step_blocks_block_vars {
    NJS_OBJECT *object; /* the block's piece, from its task spec's var */
    NJS_OBJECT *unk_04; /* a copy of object, which no step reads */
    NJS_TEXLIST *texlist; /* the stage's, which the draw names itself */
};

#define BLOCK(work) ((struct step_blocks_block_vars *)&(work)->vars)

enum step_blocks_mode { STEPS_INIT, STEPS_RUN };

/* a block starts in BLOCK_REST, which no step here leaves, so the modes after
 * it never run. from BLOCK_SINK, a block moves by its force's speed.y, and
 * clears it, until work->submode passes work->tick. BLOCK_TOUCH then turns its
 * cylinder on for one step, and BLOCK_FALL drops it */
enum step_blocks_block_mode {
    BLOCK_INIT,
    BLOCK_REST,
    BLOCK_SINK,
    BLOCK_TOUCH,
    BLOCK_TOUCHED,
    BLOCK_FALL,
    BLOCK_GONE
};

#define BLOCK_GRAVITY 0.01F
/* the upward speed past which a falling block is killed */
#define BLOCK_LEAVE_SPEED 5.0F
#define DRAW_RADIUS 100.0F

static void step_blocks_destroy(struct task *task);
static void step_blocks_group_step(struct task *task);
static void step_blocks_block_step(struct task *task);
static void step_blocks_block_draw(struct task *task);

/* the first shape, a wide flat cylinder, starts off. BLOCK_TOUCH turns it on
 * for one step */
static struct collision_shape step_blocks_shapes[3] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     25.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     3.5F,
     2.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.0F, 0.0F},
     5.5F,
     6.5F,
     12.5F,
     {0},
     0,
     NULL},
};

/* the root's four groups, two of each layout. a group's work->flags picks
 * its layout */
static struct task_spec step_blocks_groups[] = {
    {step_blocks_group_step,
     TASK_WORK,
     0,
     {0},
     {14.9999972F, -6.0F, -49.0F},
     0},
    {step_blocks_group_step, TASK_WORK, 1, {0}, {-6.0F, -7.0F, -50.0F}, 0},
    {step_blocks_group_step, TASK_WORK, 0, {0}, {14.9999972F, -6.0F, 9.0F}, 0},
    {step_blocks_group_step, TASK_WORK, 1, {0}, {-6.0F, -7.0F, 10.0F}, 0},
    {NULL, 0, 0, {0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* each layout's six blocks, numbered 0 to 11 in their work->flags. the
 * block step ignores the number */
static struct task_spec step_blocks_group_0[] = {
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&break_step_piece_11_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     1,
     {0},
     {0.0F, -1.0F, 19.0F},
     (Sint32)&break_step_piece_7_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     2,
     {0},
     {0.0F, -2.0F, 39.0F},
     (Sint32)&break_step_piece_3_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     3,
     {0},
     {-11.0F, 0.0F, -1.0F},
     (Sint32)&break_step_piece_10_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     4,
     {0},
     {-11.0F, -1.0F, 19.0F},
     (Sint32)&break_step_piece_6_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     5,
     {0},
     {-9.0F, -1.0F, 39.0F},
     (Sint32)&break_step_piece_2_object},
    {NULL, 0, 0, {0}, {0.0F, 0.0F, 0.0F}, 0},
};
static struct task_spec step_blocks_group_1[] = {
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     6,
     {0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&break_step_piece_9_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     7,
     {0},
     {2.0F, 0.0F, 19.0F},
     (Sint32)&break_step_piece_5_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     8,
     {0},
     {2.0F, 0.0F, 38.0F},
     (Sint32)&break_step_piece_1_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     9,
     {0},
     {-9.0F, 0.0F, 0.0F},
     (Sint32)&break_step_piece_8_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     10,
     {0},
     {-9.0F, 0.0F, 20.0F},
     (Sint32)&break_step_piece_4_object},
    {step_blocks_block_step,
     TASK_WORK | TASK_FORCE,
     11,
     {0},
     {-8.0F, 1.0F, 40.0F},
     (Sint32)&break_step_piece_0_object},
    {NULL, 0, 0, {0}, {0.0F, 0.0F, 0.0F}, 0},
};

static void step_blocks_group_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STEPS_INIT:
        work->mode = STEPS_RUN;
        switch (work->flags) {
        case 0:
            task_create_children(step_blocks_group_0, task);
            break;
        case 1:
            task_create_children(step_blocks_group_1, task);
            break;
        }
        break;
    case STEPS_RUN:
        task_step_children(task);
        break;
    }
}

static void step_blocks_block_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;

    switch (work->mode) {
    case BLOCK_INIT:
        work->mode = BLOCK_REST;
        BLOCK(work)->unk_04 = BLOCK(work)->object;
        BLOCK(work)->texlist = &obj_mountain_texlist;
        collision_init(task, step_blocks_shapes, COLLISION_OBJECT_MUTUAL, 1, 3);
        work->collision->shapes->flags |= SHAPE_OFF;
        force->speed.x = force->speed.y = force->speed.z = 0.0F;
        break;
    case BLOCK_REST:
        break;
    case BLOCK_SINK:
        work->pos.y += force->speed.y;
        force->speed.y = 0.0F;
        if (work->submode++ > work->tick) {
            work->mode = BLOCK_TOUCH;
        }
        break;
    case BLOCK_TOUCH:
        work->collision->shapes->flags &= ~SHAPE_OFF;
        work->mode = BLOCK_TOUCHED;
        break;
    case BLOCK_TOUCHED:
        work->collision->shapes->flags |= SHAPE_OFF;
        work->mode = BLOCK_FALL;
        break;
    case BLOCK_FALL:
        force->speed.y -= BLOCK_GRAVITY;
        work->pos.y += force->speed.y;
        /* @bug logic: speed.y only drops from 0 here, so it never passes
         * BLOCK_LEAVE_SPEED (FCMP/GT at 0x0c909186), and a falling block
         * would never reach BLOCK_GONE. no step here reaches BLOCK_FALL, so
         * the test never runs */
        if (force->speed.y > BLOCK_LEAVE_SPEED) {
            work->mode = BLOCK_GONE;
        }
        break;
    case BLOCK_GONE:
        task_kill(task);
        break;
    /* @hack redundant: an empty default, which calls collision_register through
     * R3 (0x0c909196); without it, R2 takes its address and 2 of 129 words
     * differ */
    default:
        break;
    }
    collision_register(work);
    step_blocks_block_draw(task);
}

static void step_blocks_block_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = BLOCK(work)->object;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(object->model);
        njPopMatrix(1);
    }
}

void step_blocks_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case STEPS_INIT:
        work->mode = STEPS_RUN;
        task->destructor = step_blocks_destroy;
        task_create_children(step_blocks_groups, task);
        break;
    case STEPS_RUN:
        task_step_children(task);
        break;
    }
}

static void step_blocks_destroy(struct task *task)
{
}
