/* O SETIFF: a source of debris that swirls up about the world's y axis */
#ifndef STG02_SWIRL_H
#define STG02_SWIRL_H

#include "system/task.h"

/* the object list's step for O SETIFF. task needs TASK_WORK's record. the
 * source starts a burst each time its count of steps passes scl.y + 40. it
 * then counts again from 0. failed allocations of a burst's task and its
 * pieces' go unchecked */
void windy_swirl_step(struct task *task);

#endif
