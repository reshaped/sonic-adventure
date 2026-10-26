/* O S2 KASEKI fossil in the second shop, turned about y only */
#include <ninja.h>

#include "adv00/shop2_fossil.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shop2_fossil_object;

static void shop2_fossil_step(struct task *task);

void shop2_fossil_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    task->step = shop2_fossil_step;
    task->draw = shop2_fossil_step;
}

/* the task's step and its draw, with no range test */
static void shop2_fossil_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&shop2_fossil_object);
    njPopMatrix(1);
}
