/* O Curb solid curb */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/curb.h"
#include "system/task.h"

extern NJS_OBJECT curb_object;

/* reach of the curb's collision record from its position */
#define CURB_REACH 10.0F

static struct collision_shape curb_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 2.5F, 0.0F},
    6.0F,
    2.5F,
    2.2F,
    {0},
    0,
    NULL,
};

void curb_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &curb_object;
    if (!placement_editing(task)) {
        collision_init(task, &curb_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = CURB_REACH;
    }
    model_start_solid(task);
}
