/* EV00B7.PRS's picture of Froggy: a task that draws him six times his size
 * and pressed flat, high behind Eggman, turning about his own y axis */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "ev00b7/frog_screen.h"
#include "system/task.h"
#include "system/util.h"

/* the angle the picture turns each step: a full turn every 128 steps */
#define FROG_SCREEN_TURN 0x200

extern NJS_TEXLIST ev00b7_frog_texlist;
extern NJS_ACTION ev00b7_frog_action;

/* four pairs: a point of Froggy's node 1, then the point of his node 12
 * that model_weld moves onto it */
static Sint16 ev00b7_frog_screen_pairs[8] = {
#include "ev00b7/frog_screen/ev00b7_frog_screen_pairs.inc"
};
static struct model_weld ev00b7_frog_screen_welds[2] = {
    {4, 1, 12, ev00b7_frog_screen_pairs},
    {0, 0, 0, NULL},
};

static struct task *ev00b7_frog_screen = NULL;

static void ev00b7_frog_screen_destroy(struct task *task)
{
}

static void ev00b7_frog_screen_draw(struct task *task)
{
    NJS_ACTION *action;
    Angle ang;

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 140.0F, -437.0F);
    njRotateX(NULL, 0x1800);
    njScale(NULL, 6.0F, 6.0F, 0.01F);
    TURN_VIA(njRotateY, task->work->ang.y, ang);
    njSetTexture(&ev00b7_frog_texlist);
    action = &ev00b7_frog_action;
    model_weld(action->object, action->motion, 0.0F, ev00b7_frog_screen_welds);
    draw_action(action, 0.0F);
    njPopMatrix(1);
}

static void ev00b7_frog_screen_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang = work->ang.y;

    ang += FROG_SCREEN_TURN;
    work->ang.y = (Uint16)ang;
    ev00b7_frog_screen_draw(task);
}

void ev00b7_frog_screen_create(void)
{
    struct task_work *work;

    ev00b7_frog_screen = task_create(TASK_WORK, 5, NULL);
    if (ev00b7_frog_screen != NULL) {
        ev00b7_frog_screen->step = ev00b7_frog_screen_step;
        ev00b7_frog_screen->draw = ev00b7_frog_screen_draw;
        ev00b7_frog_screen->destructor = ev00b7_frog_screen_destroy;
        work = ev00b7_frog_screen->work;
        work->ang.y = 0;
    }
}

void ev00b7_frog_screen_kill(void)
{
    if (ev00b7_frog_screen != NULL) {
        task_kill(ev00b7_frog_screen);
        ev00b7_frog_screen = NULL;
    }
}
