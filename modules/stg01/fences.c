/* O FENA, O FENB, O FENC and O FEND solid fences of four lengths */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg01/fences.h"
#include "system/task.h"

extern NJS_OBJECT beach_fence_a_object;
extern NJS_OBJECT beach_fence_b_object;
extern NJS_OBJECT beach_fence_c_object;
extern NJS_OBJECT beach_fence_d_object;

static struct collision_shape beach_fence_a_shape = {SHAPE_RECTANGLE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {17.0F, 5.0F, 0.0F},
                                                     19.0F,
                                                     5.5F,
                                                     1.5F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};
static struct collision_shape beach_fence_b_shape = {SHAPE_RECTANGLE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {0.0F, 5.0F, 0.0F},
                                                     5.0F,
                                                     5.5F,
                                                     1.5F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};
static struct collision_shape beach_fence_c_shape = {SHAPE_RECTANGLE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {12.0F, 5.0F, 0.0F},
                                                     15.0F,
                                                     5.5F,
                                                     1.5F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};
static struct collision_shape beach_fence_d_shape = {SHAPE_RECTANGLE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {0.0F, 5.0F, 0.0F},
                                                     3.0F,
                                                     5.5F,
                                                     1.5F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};

void beach_fence_a_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_fence_a_object;
    if (!placement_editing(task)) {
        collision_init(task, &beach_fence_a_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void beach_fence_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_fence_b_object;
    if (!placement_editing(task)) {
        collision_init(task, &beach_fence_b_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void beach_fence_c_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_fence_c_object;
    if (!placement_editing(task)) {
        collision_init(task, &beach_fence_c_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void beach_fence_d_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &beach_fence_d_object;
    if (!placement_editing(task)) {
        collision_init(task, &beach_fence_d_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}
