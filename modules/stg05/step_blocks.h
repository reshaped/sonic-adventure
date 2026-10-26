/* O BREAKSTEP's twelve pieces as block tasks, in four groups of six. no
 * object list or code refers to the root's step */
#ifndef STG05_STEP_BLOCKS_H
#define STG05_STEP_BLOCKS_H

#include "system/task.h"

/* the root's task step, which creates the four groups */
void step_blocks_step(struct task *task);

#endif
