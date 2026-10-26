/* O Dou1 and O Dou2, which draw nothing where the set file puts them */
#include <ninja.h>

#include "stg07/dou.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* which dou a task is, in its work->mode. no code reads it */
enum dou_kind { DOU_1, DOU_2 };

static void dou_destroy(struct task *task)
{
}

/* push a matrix at the dou's place, turned its way, and pop it with
 * nothing drawn */
static void dou_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njPopMatrix(1);
}

void dou_1_step(struct task *task)
{
    task->work->mode = DOU_1;
    task->step = dou_draw;
    task->draw = dou_draw;
    task->destructor = dou_destroy;
}

void dou_2_step(struct task *task)
{
    task->work->mode = DOU_2;
    task->step = dou_draw;
    task->draw = dou_draw;
    task->destructor = dou_destroy;
}
