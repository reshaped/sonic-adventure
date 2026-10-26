/* Maruta: a solid log */
#include <ninja.h>

#include "collision/collision.h"
#include "object/common.h"
#include "stg08/log.h"
#include "system/task.h"

extern NJS_OBJECT log_object;

/* a capsule lying along z */
static struct collision_shape log_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, 5.0F, 0.0F},
    6.0F,
    48.0F,
    0.0F,
    {0x4000, 0, 0},
    0,
    NULL,
};

void log_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &log_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &log_object;
    model_start_solid(task);
}
