/* O TUTUA, O TUTUB and O TUTUC tubes, which turn, carrying player 0 round */
#ifndef STG09_TUBE_H
#define STG09_TUBE_H

#include "system/task.h"

/* the object list's steps. an odd scl.x, truncated, turns a tube to the
 * right, and scl.y, truncated, sets its speed */
void tube_a_step(struct task *task);
void tube_b_step(struct task *task);
void tube_c_step(struct task *task);

#endif
