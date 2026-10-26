/* O KOMONO props: a bottle, an exit sign, two chairs and two lights */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/props.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT prop_bottle_object;
extern NJS_OBJECT container_object;
extern NJS_OBJECT prop_exit_object;
extern NJS_OBJECT prop_chair_a_object;
extern NJS_OBJECT prop_chair_b_object;
extern NJS_OBJECT prop_fluorescent_object;
extern NJS_OBJECT prop_room_light_object;

static void prop_step(struct task *task);
static void prop_draw(struct task *task);
static void prop_turned_step(struct task *task);
static void prop_turned_draw(struct task *task);

/* clang-format off */
static struct collision_shape prop_bottle_shape[] = {
    {SHAPE_CAPSULE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 1.175F, 0.0F},
     0.265F, 0.91F, 0.0F, {0}, 0, NULL},
};

static struct collision_shape prop_container_shape[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 14.7F, 0.0F},
     15.0F, 14.7F, 15.0F, {0}, 0, NULL},
};

static struct collision_shape prop_exit_shape[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, -1.95F, 1.0F},
     4.0F, 1.95F, 1.0F, {0}, 0, NULL},
};

/* each chair's seat and back */
static struct collision_shape prop_chair_a_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 2.0F, 2.5F},
     2.41F, 2.0F, 2.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 4.35F, 0.6F},
     2.41F, 4.35F, 0.6F, {0}, 0, NULL},
};

static struct collision_shape prop_chair_b_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 2.4F, 2.35F},
     2.0F, 2.4F, 2.35F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 4.75F, 0.6F},
     2.0F, 4.75F, 0.6F, {0}, 0, NULL},
};
/* clang-format on */

/* start a prop that draws object's root model turned about y alone. outside
 * the set editor, shapes[0] to shapes[count - 1] are its collision */
static void prop_start(struct task *task, struct collision_shape *shapes,
                       int count, NJS_OBJECT *object)
{
    struct task_work *work = task->work;

    MODEL(work)->object = object;
    if (!placement_editing(task)) {
        collision_init(task, shapes, COLLISION_OBJECT, 1, count);
    }
    task->draw = prop_draw;
    task->step = prop_step;
}

/* nothing calls it, so SHC would drop it as a static, and the object's code
 * would run 64 bytes short. the image holds it at 0x0c9006dc */
void prop_start_turned(struct task *task, struct collision_shape *shapes,
                       int count, NJS_OBJECT *object)
{
    struct task_work *work = task->work;

    MODEL(work)->object = object;
    if (!placement_editing(task)) {
        collision_init(task, shapes, COLLISION_OBJECT, 1, count);
    }
    task->draw = prop_turned_draw;
    task->step = prop_turned_step;
}

static void prop_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
    }
    prop_draw(task);
}

/* draw the object's root model alone, at the root's offset from the prop,
 * turned about y */
static void prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float *pos;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (njPushMatrix(NULL)) {
        texture_set_stage();
        pos = MODEL(work)->object->pos;
        njTranslate(NULL, work->pos.x + pos[0], work->pos.y + pos[1],
                    work->pos.z + pos[2]);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(MODEL(work)->object->model);
        njPopMatrix(1);
    }
}

static void prop_turned_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
    }
    prop_turned_draw(task);
}

static void prop_turned_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float *pos;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (njPushMatrix(NULL)) {
        texture_set_stage();
        pos = MODEL(work)->object->pos;
        njTranslate(NULL, work->pos.x + pos[0], work->pos.y + pos[1],
                    work->pos.z + pos[2]);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(MODEL(work)->object->model);
        njPopMatrix(1);
    }
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (MOV R4,R14 at 0x0c9008e8). without it, task is kept at sp+0, 13 of
 * 17 words differ, and the object's code runs 32 bytes long */
void prop_bottle_init(register struct task *task)
{
    if (!placement_try_leave(task)) {
        prop_start(task, prop_bottle_shape, 1, &prop_bottle_object);
    }
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (MOV R4,R14 at 0x0c90090a). without it, task is kept at sp+0, 13 of
 * 17 words differ, and the object's code runs 32 bytes long */
void prop_container_init(register struct task *task)
{
    if (!placement_try_leave(task)) {
        prop_start(task, prop_container_shape, 1, &container_object);
    }
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (MOV R4,R14 at 0x0c90092c). without it, task is kept at sp+0 and 11
 * of 17 words differ */
void prop_exit_init(register struct task *task)
{
    if (!placement_try_leave(task)) {
        prop_start(task, prop_exit_shape, 1, &prop_exit_object);
    }
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (MOV R4,R14 at 0x0c90098e). without it, task is kept at sp+0 and 11
 * of 17 words differ */
void prop_chair_a_init(register struct task *task)
{
    if (!placement_try_leave(task)) {
        prop_start(task, prop_chair_a_shapes, 2, &prop_chair_a_object);
    }
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (MOV R4,R14 at 0x0c9009b0). without it, task is kept at sp+0 and 11
 * of 17 words differ */
void prop_chair_b_init(register struct task *task)
{
    if (!placement_try_leave(task)) {
        prop_start(task, prop_chair_b_shapes, 2, &prop_chair_b_object);
    }
}

/* the lights have no shapes, and their step is their draw */
void prop_fluorescent_init(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    MODEL(work)->object = &prop_fluorescent_object;
    task->draw = prop_draw;
    task->step = prop_draw;
}

void prop_room_light_init(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    MODEL(work)->object = &prop_room_light_object;
    task->draw = prop_draw;
    task->step = prop_draw;
}
