/* O S2 BOXFUTA lid of the box in the second shop */
#include <ninja.h>

#include "adv00/shop2_box_lid.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shop2_box_lid_object;

static void shop2_box_lid_step(struct task *task);

/* the draw turns the lid about y alone, and reads neither angle cleared
 * here */
void shop2_box_lid_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = 0;
    task->step = shop2_box_lid_step;
    task->draw = shop2_box_lid_step;
}

/* the task's step and its draw */
static void shop2_box_lid_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (!placement_try_leave(task)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&shop2_box_lid_object);
        njPopMatrix(1);
    }
}
