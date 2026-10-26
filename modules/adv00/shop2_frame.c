/* O S2 FRAME picture frame in the second shop */
#include <ninja.h>

#include "adv00/shop2_frame.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shop2_frame_object;

static void shop2_frame_step(struct task *task);
static void shop2_frame_destroy(struct task *task);

void shop2_frame_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = 0;
    task->step = shop2_frame_step;
    task->draw = shop2_frame_step;
    task->destructor = shop2_frame_destroy;
}

/* the task's step and its draw */
static void shop2_frame_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(&shop2_frame_object);
    njPopMatrix(1);
}

static void shop2_frame_destroy(struct task *task)
{
}
