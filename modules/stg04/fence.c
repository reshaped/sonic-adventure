/* O FENCE solid railing */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/fence.h"
#include "system/task.h"

extern NJS_OBJECT fence_object;

static struct collision_shape fence_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 4.25F, 0.0F},
    13.0F,
    4.25F,
    2.75F,
    {0},
    0,
    NULL,
};

void fence_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, &fence_shape, COLLISION_OBJECT, 1, 1);
    }
    MODEL(work)->object = &fence_object;
    model_start_solid(task);
}
