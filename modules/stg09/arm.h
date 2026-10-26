/* O SARM: an arm that swings through its action, resting 250 steps between
 * swings */
#ifndef STG09_ARM_H
#define STG09_ARM_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record */
void arm_step(struct task *task);

#endif
