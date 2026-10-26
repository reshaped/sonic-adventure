/* O TORNADO2 Tornado 2 plane, drawn only in the jungle act */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/mystic_ruins.h"
#include "adv02/texlists.h"
#include "adv02/tornado2.h"
#include "draw/tree.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT tornado2_object;

enum tornado2_mode { TORNADO2_INIT, TORNADO2_RUN };

static void tornado2_draw(struct task *task);

void tornado2_step(struct task *task)
{
    struct task_work *work = task->work;

    if ((Uint8)stage_get_stage_act() != MYSTIC_RUINS_JUNGLE) {
        return;
    }
    switch (work->mode) {
    case TORNADO2_INIT:
        task->draw = tornado2_draw;
        work->mode = TORNADO2_RUN;
        break;
    case TORNADO2_RUN:
        break;
    }
    tornado2_draw(task);
}

static void tornado2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&tornado2_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&tornado2_object);
    njPopMatrix(1);
}
