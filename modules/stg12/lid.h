/* the lid over the hole under act 0's upper pool. it breaks into falling
 * pieces as the pool starts to pour */
#ifndef STG12_LID_H
#define STG12_LID_H

#include "system/task.h"

/* the lid's first step. the upper pool creates the lid as its child. task
 * needs TASK_WORK's record */
void shelter_lid_start(struct task *task);

#endif
