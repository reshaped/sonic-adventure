/* O_Hako, a solid snow-covered box */
#include <ninja.h>

#include "collision/collision.h"
#include "object/common.h"
#include "stg08/snow_box.h"
#include "system/task.h"

extern NJS_OBJECT snow_box_object;

/* the box, 20 units along x, 10 along z and 6 tall, standing on the task's
 * position */
static struct collision_shape snow_box_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, 3.0F, 0.0F},
    10.0F,
    3.0F,
    5.0F,
    {0},
    0,
    NULL,
};

void snow_box_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &snow_box_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &snow_box_object;
    model_start_solid(task);
}
