/* the spikes: Toge, a solid model. their shape hurts a player who touches
 * it */
#include <ninja.h>

#include "collision/collision.h"
#include "object/common.h"
#include "stg07/spikes.h"
#include "system/task.h"

extern NJS_OBJECT spikes_object;

/* clang-format off */
static struct collision_shape spikes_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(3) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 4.0F, 0.0F}, 10.0F, 0.0F, 0.0F,
    {0}, 0, NULL,
};
/* clang-format on */

void spikes_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &spikes_shape, COLLISION_OBJECT, 1, 1);
    MODEL(work)->object = &spikes_object;
    model_start_solid(task);
}
