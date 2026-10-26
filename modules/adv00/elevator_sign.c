/* O ELEKANBAN elevator sign, lit at night */
#include <ninja.h>

#include "adv00/elevator_sign.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT elevator_sign_object;

#define LIT(work) ((work)->submode)

static void elevator_sign_step(struct task *task);
static void elevator_sign_draw(struct task *task);

void elevator_sign_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = 0;
    if (story_get_time() == TIME_NIGHT) {
        LIT(work) = 1;
    } else {
        LIT(work) = 0;
    }
    task->step = elevator_sign_step;
    task->draw = elevator_sign_draw;
}

static void elevator_sign_step(struct task *task)
{
    if (!placement_try_leave(task)) {
        elevator_sign_draw(task);
    }
}

static void elevator_sign_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    draw_control_save();
    draw_attr_save();
    if (LIT(work) == 1) {
        draw_control_set(NJD_CONTROL_3D_CONSTANT_ATTR);
        draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
        draw_attr_clear(NJD_FLAG_IGNORE_LIGHT, 0);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(&elevator_sign_object);
    njPopMatrix(1);
    draw_attr_restore();
    draw_control_restore();
}
