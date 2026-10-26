/* an event's dome: the blast dome of effect/blast_dome.h, grown at a point */
#ifndef EVENT_DOME_H
#define EVENT_DOME_H
#include <sg_xpt.h>

#include "system/task.h"

/* what the dome's step does, by work->mode */
enum event_dome_mode {
    EVENT_DOME_WAIT, /* nothing */
    EVENT_DOME_GROW, /* it draws and grows, then sets EVENT_DOME_DONE */
    EVENT_DOME_DONE /* nothing */
};

/* create a dome at x, y and z, of radius 2, or return null when task_create
 * fails. it waits until it is set to EVENT_DOME_GROW. it then draws once on
 * each step, as many times as steps gives. it grows after each draw but the
 * last. a growth adds 0.1 to growth, scales growth by the damping and adds it
 * to the radius. a radius past limit drops back to limit at the next growth
 * instead. its alpha falls 0.033 after each draw that leaves fewer than 30
 * draws */
struct task *event_dome_create(Float x, Float y, Float z, int steps,
                               Float growth, Float limit);
/* set the dome's mode. task may be null */
void event_dome_set_mode(struct task *task, enum event_dome_mode mode);
/* while the dome waits, set its damping, its scroll and the radius it grows
 * to. scroll is how far its texture scrolls each step, truncated to an int.
 * create sets 0.8, 10 and its own limit. task may be null */
void event_dome_set_growth(struct task *task, Float damping, Float scroll,
                           Float limit);

#endif
