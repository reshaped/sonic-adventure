/* JdaiL and JdaiR roll player 0 up a ramp, and tip it onto a rail round the
 * table. each has a target on the table */
#ifndef STG09_JDAI_H
#define STG09_JDAI_H

#include "system/task.h"

/* the object list's steps. a Jdai ignores the set entry's scale, and keeps
 * its ride's state in scl. its target is a child task, which the Jdai steps
 * only after the rail. the target counts touches only then. that lasts until
 * player 0 is back on the table and no bonus holds the camera */
void jdai_left_step(struct task *task);
void jdai_right_step(struct task *task);

#endif
