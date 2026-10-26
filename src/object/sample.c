/* samples: stand-ins, drawn as the marker cylinder, for things a player
 * carries, in three sizes, and for one the player shakes */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "object/sample.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* draw_highlight_near's mask for Big alone */
#define BIG_ONLY (1 << CHARACTER_BIG)

static void sample_small_step(struct task *task);
static void sample_small_destroy(struct task *task);
static void sample_medium_step(struct task *task);
static void sample_medium_destroy(struct task *task);
static void sample_large_step(struct task *task);
static void sample_large_destroy(struct task *task);
static void sample_shake_step(struct task *task);
static void sample_shake_destroy(struct task *task);

/* each sample's two shapes, a wide one and a narrow one */
/* clang-format off */
static struct collision_shape sample_small_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.0F, 0.0F}, 15.0F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS), {0.0F, 2.0F, 0.0F}, 0.1F,
     2.5F, 0.0F, {0}, 0, NULL},
};
static struct collision_shape sample_medium_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE, 0, 0, {0.0F, 5.0F, 0.0F}, 18.2F, 6.0F,
     0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 4.5F, 0.0F}, 2.6F, 6.0F, 0.0F, {0}, 0, NULL},
};
static struct collision_shape sample_large_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE, 0, 0, {0.0F, 15.0F, 0.0F}, 22.0F, 15.0F,
     0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 15.0F, 0.0F}, 12.6F, 15.0F, 0.0F, {0}, 0, NULL},
};
static struct collision_shape sample_shake_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE, 0, 0, {0.0F, 20.0F, 0.0F}, 1.2F, 20.0F,
     0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 20.0F, 0.0F}, 0.1F, 20.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

void sample_small_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    MODEL(work)->object = &marker_cylinder_object;
    if (!placement_editing(task)) {
        collision_init(task, sample_small_shapes, COLLISION_OBJECT, 1, 2);
    }
    carry_set_kind(work, motion, CARRY_KIND_SMALL);
    task->destructor = sample_small_destroy;
    task->step = sample_small_step;
}

static void sample_small_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    carry_step_small(task);
    draw_highlight_near(work, ALL_CHARACTERS);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + 2.5F, work->pos.z);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, 0.25F, 0.25F, 0.25F);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void sample_small_destroy(struct task *task)
{
}

void sample_medium_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    MODEL(work)->object = &marker_cylinder_object;
    if (!placement_editing(task)) {
        collision_init(task, sample_medium_shapes, COLLISION_OBJECT, 1, 2);
    }
    carry_set_kind(work, motion, CARRY_KIND_MEDIUM);
    task->destructor = sample_medium_destroy;
    task->step = sample_medium_step;
}

static void sample_medium_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    carry_step_large(task);
    draw_highlight_near(work, ALL_CHARACTERS);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + 5.0F, work->pos.z);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, 0.5F, 0.5F, 0.5F);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void sample_medium_destroy(struct task *task)
{
}

void sample_large_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    MODEL(work)->object = &marker_cylinder_object;
    if (!placement_editing(task)) {
        collision_init(task, sample_large_shapes, COLLISION_OBJECT, 1, 2);
    }
    carry_set_kind(work, motion, CARRY_KIND_LARGE);
    task->destructor = sample_large_destroy;
    task->step = sample_large_step;
}

static void sample_large_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    carry_step_large(task);
    draw_highlight_near(work, BIG_ONLY);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + 15.0F, work->pos.z);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, 1.5F, 1.5F, 1.5F);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void sample_large_destroy(struct task *task)
{
}

void sample_shake_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    MODEL(work)->object = &marker_cylinder_object;
    if (!placement_editing(task)) {
        collision_init(task, sample_shake_shapes, COLLISION_OBJECT, 1, 2);
    }
    carry_set_kind(work, motion, CARRY_KIND_SHAKE);
    task->destructor = sample_shake_destroy;
    task->step = sample_shake_step;
}

/* draw the sample moved by motion's accel, the push a shake gave it this
 * step. then step it as a carried thing that doesn't move by itself */
static void sample_shake_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    if (placement_try_leave(task)) {
        return;
    }
    draw_highlight_near(work, ALL_CHARACTERS);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + motion->accel.x,
                work->pos.y + motion->accel.y + 20.0F,
                work->pos.z + motion->accel.z);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, 0.25F, 2.0F, 0.25F);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    carry_step_still(task);
}

static void sample_shake_destroy(struct task *task)
{
}
