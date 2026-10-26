/* O SIGN1 solid road sign on a post */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/common.h"
#include "stg04/sign.h"
#include "system/task.h"

extern NJS_OBJECT highway_sign_object;

/* the radius of the sign's collision record. its cylinder's top rim lies
 * 21.02 from its position */
#define SIGN_REACH 21.0F

static struct collision_shape highway_sign_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 11.0F, 0.0F},
    1.0F,
    10.0F,
    0.0F,
    {0},
    0,
    NULL,
};

void highway_sign_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &highway_sign_object;
    if (!placement_editing(task)) {
        collision_init(task, &highway_sign_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = SIGN_REACH;
    }
    model_start_solid(task);
}
