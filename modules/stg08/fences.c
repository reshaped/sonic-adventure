/* O_Saku01, O_Saku01b, O_Saku02 and O_Saku02b: solid fences */
#include <ninja.h>

#include "collision/collision.h"
#include "object/common.h"
#include "stg08/fences.h"
#include "system/task.h"

extern NJS_OBJECT fence_01_object;
extern NJS_OBJECT fence_01b_object;
extern NJS_OBJECT fence_02_object;
extern NJS_OBJECT fence_02b_object;

/* every fence's box: 44 units wide, 20 high and 4 deep */
static struct collision_shape icecap_fence_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, 10.0F, 0.0F},
    22.0F,
    10.0F,
    2.0F,
    {0},
    0,
    NULL,
};

void fence_01_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &icecap_fence_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &fence_01_object;
    model_start_solid(task);
}

void fence_01b_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &icecap_fence_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &fence_01b_object;
    model_start_solid(task);
}

void fence_02_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &icecap_fence_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &fence_02_object;
    model_start_solid(task);
}

void fence_02b_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &icecap_fence_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &fence_02b_object;
    model_start_solid(task);
}
