/* O POWERBOX low solid power box */
#include <ninja.h>

#include "adv00/power_box.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT power_box_object;

static void power_box_step(struct task *task);
static void power_box_draw(struct task *task);

/* clang-format off */
static struct collision_shape power_box_shape[] = {
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 1.0F, 0.0F},
     4.0F, 1.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* clear the x and z angles, so the box turns about y only */
void power_box_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    if (!placement_editing(task)) {
        collision_init(task, power_box_shape, COLLISION_OBJECT, 1, 1);
    }
    task->step = power_box_step;
    task->draw = power_box_draw;
}

static void power_box_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
        }
        power_box_draw(task);
    }
}

static void power_box_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&power_box_object);
    njPopMatrix(1);
}
