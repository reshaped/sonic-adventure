/* event motions: a model playing a table of actions, blending from each
 * into the next. welds close the model's seams. event_actor_draw draws the
 * motions an event record queues, welded the same way */
#ifndef EVENT_MOTION_H
#define EVENT_MOTION_H
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/motion.h"
#include "player/player.h"
#include "system/task.h"

/* an event motion's state, and the mode an action of its table plays in.
 * the step treats any other value as EVENT_MOTION_FREE */
enum event_motion_mode {
    /* as event_motion_alloc leaves it. the step makes the blend's action,
     * and the next step runs EVENT_MOTION_CHANGE's case */
    EVENT_MOTION_START,
    EVENT_MOTION_CHANGE, /* the step starts a blend into action, or a cut */
    EVENT_MOTION_BLENDING, /* frame goes from 0 to 1 by the entry's blend */
    /* to its end, then again from frame 0. the first time, it starts at
     * start */
    EVENT_MOTION_LOOP,
    /* to its end, then the entry's next. the step subtracts speed from
     * frame, so only a negative speed reaches the end */
    EVENT_MOTION_ONCE,
    EVENT_MOTION_HOLD, /* to its last frame, which it holds */
    /* as event_motion_free sets it. the step frees the blend's action */
    EVENT_MOTION_FREE = MOTION_FREE
};

/* in event_motion_control.flags, which each step clears first. the step
 * sets it when an EVENT_MOTION_ONCE action goes on to its next. it also sets
 * it on each step an EVENT_MOTION_HOLD action holds its last frame */
#define EVENT_MOTION_ENDED 0x1

/* an event motion: how a model plays its table of actions, and the welds
 * that close its seams */
struct event_motion_control {
    Sint16 state; /* an enum event_motion_mode */
    /* PLAYER_WELD_MAKE, PLAYER_WELDING or PLAYER_WELD_FREE: what
     * event_motion_weld does */
    Sint16 weld_mode;
    /* the entry asked for. the caller sets it, and so does the step, to the
     * entry's next, when an EVENT_MOTION_ONCE action ends */
    Uint16 action;
    /* the entry playing. it becomes action on the step a change starts */
    Uint16 current;
    Uint16 previous; /* current as the last step began */
    Uint16 next; /* actions[current].next, which no code reads */
    Sint16 steps; /* steps since a change last started. no code reads it */
    Uint16 flags; /* EVENT_MOTION_ENDED, which each step clears first */
    Float frame; /* the action's frame, or in a blend the blend's, 0 to 1 */
    /* the frame the next looping action starts at. event_motion_enter sets
     * it to 0 as it uses it, and nothing else writes it */
    Float start;
    struct motion_entry *actions;
    struct weld *welds; /* ended by a null base */
    NJS_ACTION *blend; /* the action a blend plays */
    Uint8 unk_24[4]; /* no code reads it. it pads the record to 40 bytes */
};

/* a task's event motion, which event_motion_alloc keeps in its work */
#define EVENT_MOTION(work) ((work)->vars.p[2])

/* step the motion once. when action changes, the motion blends from the
 * old action's pose to the new one's first keys. each step goes the new
 * entry's blend of the way. a blend of 1, or a new action on another model,
 * cuts in instead. the entry's mode then plays the action, by the entry's
 * speed each step */
void event_motion_step(struct event_motion_control *motion);
/* by weld_mode: PLAYER_WELD_MAKE copies each weld's vertices to the heap,
 * finds its nodes and sets PLAYER_WELDING. PLAYER_WELDING poses action, or
 * the blend while blending. it then moves the shared vertices of the welds
 * on that model. any other mode puts the copies back and frees them. welds
 * may be null only at PLAYER_WELD_MAKE, which then does nothing. work is
 * unread */
void event_motion_weld(struct task_work *work,
                       struct event_motion_control *motion);
/* give work an event motion at EVENT_MOTION_START and PLAYER_WELD_MAKE. when
 * heap_alloc fails, EVENT_MOTION(work) stays as it was. it sets no other
 * field: the caller sets actions, action and welds */
void event_motion_alloc(struct task_work *work);
/* free work's event motion and its blend. the motion must exist and must
 * have stepped once. until its first step, blend holds whatever the heap
 * block held. its welds' vertices go back as they were copied, unless welds
 * is null. EVENT_MOTION(work) becomes null. when action changed after the
 * last step, the blend leaks. the step meant to free it starts the change
 * instead */
void event_motion_free(struct task_work *work);
/* draw the first motion queued in the work's event record, as
 * event_motions_draw does. the welds of the work's event motion close its
 * seams, in place of a player's. with a motion queued, an event motion the
 * work has must be at PLAYER_WELDING, or this never returns */
void event_actor_draw(struct task_work *work);

#endif
