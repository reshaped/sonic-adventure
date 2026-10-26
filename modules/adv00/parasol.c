/* O PARASOL solid parasol */
#include <ninja.h>

#include "adv00/parasol.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL parasol_model;

/* clang-format off */
static struct collision_shape parasol_shape = {
    SHAPE_CAPSULE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 6.0F, 0.0F},
    13.0F, 6.0F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

static void parasol_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&parasol_model);
    njPopMatrix(1);
}

static void parasol_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        parasol_draw(task);
    } else if (!placement_try_leave(task)) {
        parasol_draw(task);
        collision_register(work);
    }
}

void station_square_parasol_init(struct task *task)
{
    if (!placement_try_leave(task)) {
        collision_init(task, &parasol_shape, COLLISION_OBJECT, 1, 1);
        task->step = parasol_step;
        task->draw = parasol_draw;
    }
}
