/* O SAKUA and O SAKUB: solid fences */
#ifndef STG02_FENCES_H
#define STG02_FENCES_H

#include "system/task.h"

/* the object list's steps. task needs TASK_WORK's record. outside the set
 * editor, each fence's box pushes back players, enemies, objects and pickups.
 * none of them pushes it */
void windy_fence_a_step(struct task *task);
void windy_fence_b_step(struct task *task);

#endif
