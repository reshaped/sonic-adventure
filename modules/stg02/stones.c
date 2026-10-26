/* O ISHIA and O ISHIB: solid stones */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg02/stones.h"
#include "system/task.h"

extern NJS_OBJECT windy_stone_a_object;
extern NJS_OBJECT windy_stone_b_object;

static struct collision_shape windy_stone_a_shape = {SHAPE_CAPSULE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {0.0F, 10.0F, 0.0F},
                                                     8.0F,
                                                     16.0F,
                                                     0.0F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};
static struct collision_shape windy_stone_b_shape = {SHAPE_CAPSULE,
                                                     0,
                                                     PUSH_ALL |
                                                         PUSH_FIRM(PUSH_ALL),
                                                     {0.0F, 10.0F, 0.0F},
                                                     8.0F,
                                                     15.0F,
                                                     0.0F,
                                                     {0, 0, 0},
                                                     0,
                                                     NULL};

void windy_stone_a_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_stone_a_object;
    if (!placement_editing(task)) {
        collision_init(task, &windy_stone_a_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}

void windy_stone_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_stone_b_object;
    if (!placement_editing(task)) {
        collision_init(task, &windy_stone_b_shape, COLLISION_OBJECT, 1, 1);
    }
    model_start_solid(task);
}
