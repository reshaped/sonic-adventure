/* O COWGIRL: a cowgirl figure that plays her action round, with a platform
 * that carries players */
#ifndef STG09_COWGIRL_H
#define STG09_COWGIRL_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's and TASK_FORCE's records.
 * the action moves on |scl.x| frames a step. each phase of the UVs shows for
 * scl.z + 2 steps, scl.z truncated */
void cowgirl_step(struct task *task);

#endif
