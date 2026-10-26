/* O WASI: an eagle that flies between random points around where it was
 * placed */
#ifndef STG02_EAGLE_H
#define STG02_EAGLE_H

#include "system/task.h"

/* the object list's step for O WASI. task needs TASK_WORK's, TASK_MOTION's
 * and TASK_EXTRA's records, and player 0's task must exist. the eagle's
 * targets lie 80 plus scl.y from where it was placed, in x and z */
void windy_eagle_step(struct task *task);

#endif
