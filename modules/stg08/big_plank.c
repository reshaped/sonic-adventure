/* BigIta: a big plank with a solid box */
#include <ninja.h>

#include "collision/collision.h"
#include "object/common.h"
#include "stg08/big_plank.h"
#include "system/task.h"

extern NJS_OBJECT big_plank_object;

/* a box 20 units along x, 6 high and 10 along z, its base at the plank's
 * position. it pushes every group, and no group can push it */
static struct collision_shape big_plank_shape = {
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

void big_plank_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &big_plank_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &big_plank_object;
    model_start_solid(task);
}
