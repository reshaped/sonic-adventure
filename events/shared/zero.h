/* an event's Zero, Amy and the bird, drawn together by one task */
#ifndef EVENT_ZERO_H
#define EVENT_ZERO_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the textures the task draws with, which the scripts load from
 * AMY_EGGROBO */
extern NJS_TEXLIST event_zero_texlist;

/* a task at level 3 that draws Zero, Amy and the bird at its pos. it turns
 * them by ang.z, then ang.x, then ang.y. their actions loop together, at 1
 * frame a step until event_zero_set_speed. returns null when task_create
 * fails, and event_zero_get then gives what it gave before.
 *
 * the task's first step gives it an event record with EVENT_GROUND and
 * EVENT_SLOPE set. a caller can change the record only after that step.
 * the task reads only the record's flags and shadow. the motions, paths and
 * mouth text given to the record never run. each later step,
 * EVENT_GROUND and EVENT_SLOPE apply by the ground below. while the
 * record's shadow is above 0, a shadow of that size draws on that ground */
struct task *event_zero_create(void);
/* set the animation speed of event_zero_get's task, in frames a step,
 * unless that is null. a speed set before the task's first step is lost,
 * since that step sets it to 1. the task must not have been destroyed */
void event_zero_set_speed(Float speed);
/* the last task event_zero_create made, or null until it makes one. the
 * task's destruction leaves the pointer dangling */
struct task *event_zero_get(void);

#endif
