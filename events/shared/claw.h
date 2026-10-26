/* an event's claw: a grabber on a spring, whose two arms open and close */
#ifndef EVENT_CLAW_H
#define EVENT_CLAW_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* what the claw's step does, by work->mode. CLAW_OPEN, CLAW_CLOSE and
 * CLAW_MOVE set CLAW_STILL when they're done */
enum claw_mode {
    CLAW_STILL, /* nothing */
    CLAW_OPEN, /* its arms turn apart, until fully open */
    CLAW_CLOSE, /* its arms turn together, until a step lands on shut */
    /* it moves by a speed for a count of steps. task_create zeroes the speed
     * and the count, and no call here sets them. a move then ends on its first
     * step, in place */
    CLAW_MOVE,
    CLAW_LOCKED /* nothing, for good: event_claw_set_mode can't change it */
};

/* create a claw at x, y and z, turned by the angles, or return null when
 * task_create fails. it starts shut and still. its arms turn 0x20 a step as
 * it opens, and 0x200 as it closes */
struct task *event_claw_create(Float x, Float y, Float z, Angle ang_x,
                               Angle ang_y, Angle ang_z);
/* set the claw's mode, but only while it is CLAW_STILL. task may be null */
void event_claw_set_mode(struct task *task, enum claw_mode mode);
/* set how far its arms turn each step as it opens, and as it closes. task may
 * be null. each close step turns the arms by close_rate, mod 0x10000. the close
 * ends on the first step that lands on shut. from open, a close_rate that
 * divides 0x3000 lands there without passing it. any other turns the arms on
 * past shut, a whole turn or more. a multiple of 0x2000, 0 included, never
 * lands there. from shut, a close turns them one or more whole turns, or none
 * for a close_rate of 0 */
void event_claw_set_rates(struct task *task, int open_rate, int close_rate);

#endif
