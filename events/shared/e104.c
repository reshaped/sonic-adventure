/* an event's E-104, which draws the model each event defines */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "event/event.h"
#include "shared/e104.h"
#include "system/task.h"
#include "system/util.h"

/* his first step clears unk_00 and unk_08. nothing reads them */
struct e104_vars {
    Float unk_00;
    Uint8 unk_04[4];
    Uint32 unk_08;
};

#define VARS(work) ((struct e104_vars *)&(work)->vars)

/* each event that links e104.c defines these. the draw draws
 * event_e104_object while no motion is queued */
extern NJS_TEXLIST event_e104_texlist;
extern NJS_OBJECT event_e104_object;

static void event_e104_destroy(struct task *task)
{
}

static void event_e104_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    Angle ang;

    if (event->motions != NULL) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        event_motions_draw(work);
        njPopMatrix(1);
    } else {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njSetTexture(&event_e104_texlist);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        draw_tree(&event_e104_object);
        draw_set_palette(0);
        njPopMatrix(1);
    }
}

static void event_e104_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    NJS_POINT3 under;
    struct angles slope;
    Float x;
    Float y;
    Float z;

    if (event->paths != NULL) {
        event_paths_step(work);
    }
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    under.y = ground_below(x, y, z, &slope);
    if (event->flags & EVENT_GROUND) {
        work->pos.y = under.y;
    }
    if (event->flags & EVENT_SLOPE) {
        work->ang.x = slope.x;
        work->ang.z = slope.z;
    }
    if (event->shadow > 0.0F) {
        under.x = x;
        under.y += SHADOW_LIFT;
        under.z = z;
        /* @hack spelling: work->event read again for the shadow's size (MOV.L
         * @(60,R14),R3 at 0x0cb813ee in EV00B7), though R13 holds it as event;
         * without it, the step runs 4 bytes short and 22 of 81 words differ */
        shadow_draw_at(&slope, &under, work->event->shadow);
    }
    event_e104_draw(task);
}

static void event_e104_init(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->unk_00 = 0.0F;
    VARS(work)->unk_08 = 0;
    task->step = event_e104_step;
    task->draw = event_e104_draw;
    task->destructor = event_e104_destroy;
}

struct task *event_e104_create(void)
{
    struct task *task;

    task = task_create(TASK_WORK, 3, event_e104_init);
    if (task != NULL) {
        event_alloc(task->work);
    }
    return task;
}
