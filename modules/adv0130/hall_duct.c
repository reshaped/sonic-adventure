/* O HALLDUCT, a duct drawn at a fixed place in the hall, whatever its set
 * entry's position. the set entry's y angle turns it */
#include <ninja.h>

#include "adv0130/hall_duct.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION hall_duct_action;

enum hall_duct_mode { DUCT_INIT, DUCT_RUN };

static void hall_duct_draw(struct task *task);

void egg_carrier_hall_duct_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DUCT_INIT:
        task->draw = hall_duct_draw;
        work->mode = DUCT_RUN;
        break;
    case DUCT_RUN:
        break;
    }
    hall_duct_draw(task);
}

static void hall_duct_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, -187.0F, 120.0F, -160.0F);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(&hall_duct_action, 0.0F);
    njPopMatrix(1);
}
