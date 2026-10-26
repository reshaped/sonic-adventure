/* O S2 ROBOT robot in the second shop */
#include <ninja.h>

#include "adv00/shop2_robot.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shop2_robot_object;

static void shop2_robot_step(struct task *task);

void shop2_robot_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = 0;
    task->step = shop2_robot_step;
    task->draw = shop2_robot_step;
}

/* the task's step and its draw */
static void shop2_robot_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&shop2_robot_object);
    njPopMatrix(1);
}
