/* O POLE tall, thin solid pole */
#include <ninja.h>

#include "adv00/pole.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT pole_object;

/* clang-format off */
static struct collision_shape pole_shape[] = {
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 65.0F, 0.0F},
     1.0F, 65.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static void pole_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&pole_object);
    njPopMatrix(1);
}

static void pole_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        pole_draw(task);
        return;
    }
    pole_draw(task);
    collision_register(work);
}

void pole_init(struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    collision_init(task, pole_shape, COLLISION_OBJECT, 1, 1);
    task->step = pole_step;
    task->draw = pole_draw;
}
