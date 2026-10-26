/* O HASIGO: a solid ladder, with a ledge path up its rungs that players
 * climb */
#ifndef STG10_LADDER_H
#define STG10_LADDER_H

#include "system/task.h"

/* the object list's step. it runs once, sets the ladder up, and gives the
 * task a step of its own. task needs TASK_WORK's record. a positive scl.y
 * leaves (int)scl.y rungs out of the path at the ladder's foot. that count
 * must be below 49 */
void final_ladder_start(struct task *task);

#endif
