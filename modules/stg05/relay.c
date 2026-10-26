/* O RELAY P, whose action loops where it stands */
#include <ninja.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg05/relay.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION relay_action;

/* the action's frame, its frame count, and the frames it advances each step.
 * they take the place of the set entry's scale, which the relay never reads */
#define FRAME(work) ((work)->scl.x)
#define FRAMES(work) ((work)->scl.y)
#define SPEED(work) ((work)->scl.z)

#define DRAW_RADIUS 100.0F

static void relay_run(struct task *task);
static void relay_draw(struct task *task);
static void relay_destroy(struct task *task);

void relay_step(struct task *task)
{
    struct task_work *work = task->work;

    FRAME(work) = 0.0F;
    FRAMES(work) = relay_action.motion->nbFrame;
    SPEED(work) = 1.0F;
    task->step = relay_run;
    task->draw = relay_draw;
    task->destructor = relay_destroy;
}

/* loop the action, SPEED(work) frames a step. the set editor holds it
 * still */
static void relay_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        FRAME(work) += SPEED(work);
        if (FRAME(work) >= FRAMES(work)) {
            FRAME(work) -= FRAMES(work);
        }
    }
    relay_draw(task);
}

static void relay_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(&relay_action, FRAME(work));
        njPopMatrix(1);
    }
}

static void relay_destroy(struct task *task)
{
}
