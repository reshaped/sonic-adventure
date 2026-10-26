/* O SIBA01 and O SIBA02 solid patches of lawn, one square and one round */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/lawn.h"
#include "system/task.h"

extern NJS_OBJECT lawn_1_object;
extern NJS_OBJECT lawn_2_object;

/* the radius of the square patch's collision record. its shape's corners lie
 * 28.4 from its position */
#define LAWN_1_REACH 29.0F

static struct collision_shape lawn_1_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    20.0F,
    2.0F,
    20.0F,
    {0},
    0,
    NULL,
};

static struct collision_shape lawn_2_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    40.0F,
    2.0F,
    0.0F,
    {0},
    0,
    NULL,
};

void lawn_1_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &lawn_1_object;
    if (!placement_editing(task)) {
        collision_init(task, &lawn_1_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = LAWN_1_REACH;
    }
    model_start_solid(task);
}

void lawn_2_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &lawn_2_object;
    if (!placement_editing(task)) {
        collision_init(task, &lawn_2_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}
