/* O KAOSE: the Chaos Emerald that ends the stage for Sonic and Tails */
#ifndef STG02_EMERALD_H
#define STG02_EMERALD_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. the emerald's
 * size is scl.x + 1, and scl.y picks its color. a failed allocation of the
 * capsule's task goes unchecked */
void windy_emerald_step(struct task *task);

#endif
