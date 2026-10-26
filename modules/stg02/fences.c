/* O SAKUA and O SAKUB: solid fences */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg02/fences.h"
#include "system/task.h"

extern NJS_OBJECT windy_fence_a_object;
extern NJS_OBJECT windy_fence_b_object;

static struct collision_shape windy_fence_a_shape = {SHAPE_RECTANGLE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {0.0F, 0.0F, 0.0F},
                                                     2.0F,
                                                     12.5F,
                                                     2.0F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};
static struct collision_shape windy_fence_b_shape = {SHAPE_RECTANGLE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {17.0F, 0.0F, 0.0F},
                                                     20.0F,
                                                     12.0F,
                                                     2.0F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};

void windy_fence_a_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_fence_a_object;
    if (!placement_editing(task)) {
        collision_init(task, &windy_fence_a_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void windy_fence_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_fence_b_object;
    if (!placement_editing(task)) {
        collision_init(task, &windy_fence_b_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}
