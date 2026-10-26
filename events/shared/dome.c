/* an event's dome: the blast dome, grown at a point. the work keeps its radius
 * in scl.x. scl.z is both its alpha and the blend it passes to
 * blast_dome_blend. that blend thins the dome's bands as it falls from 1 */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/blast_dome.h"
#include "shared/dome.h"
#include "system/task.h"
/* event_dome_texlist's full body names eff_regular_textures. lint reads its
 * stub body, which doesn't */
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* the steps the dome fades over, before it is done */
#define DOME_FADE_STEPS 30

struct dome_vars {
    Uint8 unk_00[4];
    Sint32 scroll; /* how far its texture has scrolled */
    Float growth; /* what its radius grows by each step */
};

/* the dome's TASK_MOTION record */
struct dome {
    Sint32 scroll; /* added to the vars' scroll each step */
    Float damping; /* what scales growth each step, after it gains 0.1 */
    Uint8 unk_08[4];
    Float limit; /* the radius it grows to */
};

#define VARS(work) ((struct dome_vars *)&(work)->vars)
#define DOME(task) ((struct dome *)(task)->motion)

/* p_exp, EFF_REGULAR's texture 131, which the dome draws with */
static NJS_TEXLIST event_dome_texlist = {
#include "shared/dome/event_dome_texlist.inc"
};

static void event_dome_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    blast_dome_scroll(VARS(work)->scroll);
    blast_dome_blend(work->scl.z);
    blast_dome_set_alpha(work->scl.z);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njSetTexture(&event_dome_texlist);
    blast_dome_draw();
    njPopMatrix(1);
}

static void event_dome_step(struct task *task)
{
    struct task_work *work = task->work;
    struct dome *dome = DOME(task);

    switch (work->mode) {
    case EVENT_DOME_WAIT:
        break;
    case EVENT_DOME_GROW:
        event_dome_draw(task);
        if (--TASK_TIMER(work) == 0) {
            work->mode = EVENT_DOME_DONE;
            break;
        }
        if (TASK_TIMER(work) < DOME_FADE_STEPS) {
            work->scl.z -= 0.033F;
        }
        VARS(work)->growth += 0.1F;
        VARS(work)->growth *= dome->damping;
        work->scl.x = work->scl.x > dome->limit
                          ? dome->limit
                          : work->scl.x + VARS(work)->growth;
        VARS(work)->scroll += dome->scroll;
        break;
    case EVENT_DOME_DONE:
        break;
    }
}

struct task *event_dome_create(Float x, Float y, Float z, int steps,
                               Float growth, Float limit)
{
    struct task *task =
        task_create(TASK_MOTION | TASK_WORK, 3, event_dome_step);

    if (task != NULL) {
        struct task_work *work = task->work;
        struct dome *dome = DOME(task);

        work->pos.x = x;
        work->pos.y = y;
        work->pos.z = z;
        work->timer = steps;
        VARS(work)->growth = growth;
        work->mode = EVENT_DOME_WAIT;
        VARS(work)->scroll = 0;
        work->scl.x = 2.0F;
        work->scl.z = 1.0F;
        dome->damping = 0.8F;
        dome->scroll = 10;
        dome->limit = limit;
        blast_dome_init();
    }
    return task;
}

void event_dome_set_mode(struct task *task, enum event_dome_mode mode)
{
    if (task != NULL) {
        task->work->mode = mode;
    }
}

void event_dome_set_growth(struct task *task, Float damping, Float scroll,
                           Float limit)
{
    if (task != NULL && task->work->mode == EVENT_DOME_WAIT) {
        DOME(task)->damping = damping;
        DOME(task)->scroll = scroll;
        DOME(task)->limit = limit;
    }
}
