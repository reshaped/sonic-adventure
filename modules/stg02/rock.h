/* O_VCROK: a rock that shakes once a player comes near, then flies at a
 * mark and breaks */
#ifndef STG02_ROCK_H
#define STG02_ROCK_H

#include "system/task.h"

/* the object list's step for O_VCROK. task needs TASK_WORK's record. scl.x
 * picks the rock's model, 0 or 1. a rock whose scl.x isn't 0 shows no pieces
 * as it breaks. the first step sets scl.y. a failed allocation of a piece's
 * task goes unchecked */
void windy_rock_step(struct task *task);

#endif
