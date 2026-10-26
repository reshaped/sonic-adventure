/* O S2 RADIO radio in the second shop */
#include <ninja.h>

#include "adv00/shop2_radio.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shop2_radio_object;

static void shop2_radio_step(struct task *task);

void shop2_radio_init(struct task *task)
{
    task->step = shop2_radio_step;
    task->draw = shop2_radio_step;
}

/* the task's step and its draw, with no range test */
static void shop2_radio_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&shop2_radio_object);
    njPopMatrix(1);
}
