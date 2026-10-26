/* Ki01, a solid tree */
#include <ninja.h>

#include "collision/collision.h"
#include "object/common.h"
#include "stg08/tree.h"
#include "system/task.h"

extern NJS_OBJECT icecap_tree_object;

static struct collision_shape icecap_tree_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, 40.0F, 0.0F},
    10.0F,
    30.0F,
    0.0F,
    {0},
    0,
    NULL,
};

void icecap_tree_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &icecap_tree_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &icecap_tree_object;
    model_start_solid(task);
}
