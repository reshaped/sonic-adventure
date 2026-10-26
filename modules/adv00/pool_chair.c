/* O POOLCHAIR solid pool chair */
#include <ninja.h>

#include "adv00/pool_chair.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL pool_chair_model;

/* clang-format off */
static struct collision_shape pool_chair_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 1.5F, 0.0F},
    4.0F, 1.5F, 9.0F, {0}, 0, NULL,
};
/* clang-format on */

static void pool_chair_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&pool_chair_model);
    njPopMatrix(1);
}

static void pool_chair_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        pool_chair_draw(task);
    } else if (!placement_try_leave(task)) {
        pool_chair_draw(task);
        collision_register(work);
    }
}

void pool_chair_init(struct task *task)
{
    if (!placement_try_leave(task)) {
        collision_init(task, &pool_chair_shape, COLLISION_OBJECT, 1, 1);
        task->step = pool_chair_step;
        task->draw = pool_chair_draw;
    }
}
