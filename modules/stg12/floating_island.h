/* O UKIJIMA: a floating platform that bobs and rocks under player 0 */
#ifndef STG12_FLOATING_ISLAND_H
#define STG12_FLOATING_ISLAND_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a task that carries an island raises rest, and the island follows */
struct floating_island_vars {
    NJS_OBJECT *body; /* in the collision world */
    Float speed; /* vertical */
    Float rest; /* the height the island returns to */
};

#define FLOATING_ISLAND(work) ((struct floating_island_vars *)&(work)->vars)

/* the object list's step. task needs TASK_MOTION's, TASK_WORK's and
 * TASK_FORCE's records */
void floating_island_step(struct task *task);
/* the step of an island another task carries, as the lower water does. the
 * island holds still until rest changes, and is never destroyed for range.
 * task needs TASK_MOTION's, TASK_WORK's and TASK_FORCE's records */
void floating_island_carried_step(struct task *task);

#endif
