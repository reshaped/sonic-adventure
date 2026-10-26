/* O GREEN and O GREENE hedge beds that are only drawn, and O GREENA, O GREENB
 * and O GREEND solid bushes */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/green.h"
#include "system/task.h"

extern NJS_OBJECT green_object;
extern NJS_OBJECT green_a_object;
extern NJS_OBJECT green_b_object;
extern NJS_OBJECT green_d_object;
extern NJS_OBJECT green_e_object;

/* reach of a bush's collision record from its position */
#define BUSH_REACH 8.0F

#define BUSH_SHAPE                                                             \
    {SHAPE_SPHERE,                                                             \
     0,                                                                        \
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),                                           \
     {0.0F, 4.0F, 0.0F},                                                       \
     4.0F,                                                                     \
     0.0F,                                                                     \
     0.0F,                                                                     \
     {0},                                                                      \
     0,                                                                        \
     NULL}
static struct collision_shape green_a_shape = BUSH_SHAPE;
static struct collision_shape green_b_shape = BUSH_SHAPE;
static struct collision_shape green_d_shape = BUSH_SHAPE;

void green_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &green_object;
    model_start(task);
}

void green_a_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &green_a_object;
    if (!placement_editing(task)) {
        collision_init(task, &green_a_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = BUSH_REACH;
    }
    model_start_solid(task);
}

void green_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &green_b_object;
    if (!placement_editing(task)) {
        collision_init(task, &green_b_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = BUSH_REACH;
    }
    model_start_solid(task);
}

void green_d_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &green_d_object;
    if (!placement_editing(task)) {
        collision_init(task, &green_d_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = BUSH_REACH;
    }
    model_start_solid(task);
}

void green_e_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &green_e_object;
    model_start(task);
}
