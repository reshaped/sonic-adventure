/* O S2 AOROBOT blue robot in the second shop, drawn with no collision */
#include <ninja.h>

#include "adv00/shop2_blue_robot.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shop2_blue_robot_object;

static void shop2_blue_robot_step(struct task *task);

/* the draw turns the robot about y alone, and reads neither angle cleared
 * here */
void shop2_blue_robot_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = 0;
    task->step = shop2_blue_robot_step;
    task->draw = shop2_blue_robot_step;
}

/* the task's step and its draw */
static void shop2_blue_robot_step(struct task *task)
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
    draw_tree(&shop2_blue_robot_object);
    njPopMatrix(1);
}
