/* O SYOUKAKI solid fire extinguisher */
#include <ninja.h>

#include "adv00/extinguisher.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL extinguisher_model;

/* clang-format off */
static struct collision_shape extinguisher_shape = {
    SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 4.5F, 0.0F},
    2.0F, 4.5F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

static void extinguisher_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&extinguisher_model);
    njPopMatrix(1);
}

static void extinguisher_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        extinguisher_draw(task);
    } else if (!placement_try_leave(task)) {
        extinguisher_draw(task);
        collision_register(work);
    }
}

void extinguisher_init(struct task *task)
{
    if (!placement_try_leave(task)) {
        collision_init(task, &extinguisher_shape, COLLISION_OBJECT, 1, 1);
        task->step = extinguisher_step;
        task->draw = extinguisher_draw;
    }
}
