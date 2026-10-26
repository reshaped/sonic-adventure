/* O Fence02 solid railing */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/fence_2.h"
#include "system/task.h"

extern NJS_OBJECT fence_2_object;

/* reach of the railing's collision record from its position */
#define FENCE_2_REACH 15.52F

static struct collision_shape fence_2_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 4.5F, 0.0F},
    10.0F,
    4.5F,
    1.0F,
    {0},
    0,
    NULL,
};

void fence_2_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &fence_2_object;
    if (!placement_editing(task)) {
        collision_init(task, &fence_2_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = FENCE_2_REACH;
    }
    model_start_solid(task);
}
