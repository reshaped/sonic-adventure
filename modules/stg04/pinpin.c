/* O PinPin solid short red post with an arrow */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/pinpin.h"
#include "system/task.h"

extern NJS_OBJECT pinpin_object;

#define PINPIN_REACH 7.3F

#define PINPIN_SHAPE(y, radius, height)                                        \
    {SHAPE_CYLINDER,                                                           \
     0,                                                                        \
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),                                           \
     {0.0F, (y), 0.0F},                                                        \
     (radius),                                                                 \
     (height),                                                                 \
     0.0F,                                                                     \
     {0},                                                                      \
     0,                                                                        \
     NULL}
static struct collision_shape pinpin_shapes[3] = {
    PINPIN_SHAPE(-1.5F, 3.5F, 2.0F),
    PINPIN_SHAPE(1.0F, 2.5F, 1.0F),
    PINPIN_SHAPE(4.0F, 1.2F, 3.0F),
};

void pinpin_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &pinpin_object;
    if (!placement_editing(task)) {
        collision_init(task, pinpin_shapes, COLLISION_OBJECT, 1, 3);
        work->collision->radius = PINPIN_REACH;
    }
    model_start_solid(task);
}
