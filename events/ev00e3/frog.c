/* Froggy as an event actor that welds nodes 1 and 12 of his model */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "ev00e3/frog.h"
#include "event/event.h"
#include "system/task.h"
#include "system/util.h"

/* four pairs of point indexes, node 1's then node 12's */
static Sint16 ev00e3_frog_pairs[8] = {
#include "ev00e3/frog/ev00e3_frog_pairs.inc"
};
static struct model_weld ev00e3_frog_welds[2] = {
    {4, 1, 12, ev00e3_frog_pairs},
    {0, 0, 0, NULL},
};

/* the shape event_actor_create gives an actor. it pushes the players and
 * stands firm against everything. it touches nothing until event_set_radius
 * gives it a radius */
/* clang-format off */
static struct collision_shape ev00e3_frog_collision[1] = {
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_PLAYERS, {0.0F, 0.0F, 0.0F},
     0.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
};
/* clang-format on */

/* weld node 12 onto node 1 at the playing motion's frame, if a motion
 * plays, then run event_actor_step */
static void ev00e3_frog_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    struct event_motion *motion;

    /* @hack spelling: the test reads work->event again, not event. SHC then
     * keeps event in R13 and motion in R14 (0x0cb804d4, 0x0cb804dc in
     * EV00E3). through event, the two trade, and 6 of 25 words differ */
    if (work->event->motions != NULL) {
        motion = event->motions;
        model_weld(motion->action.object, motion->action.motion,
                   event->motion_frame, ev00e3_frog_welds);
    }
    event_actor_step(task);
}

struct task *ev00e3_frog_create(void)
{
    struct task *task = task_create(TASK_WORK, 5, ev00e3_frog_step);

    event_alloc(task->work);
    collision_init(task, ev00e3_frog_collision, COLLISION_OBJECT, 1, 1);
    return task;
}
