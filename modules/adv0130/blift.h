/* O BLIFT, a lift that rides up once player 0 stands on it, and comes back
 * down once player 0 steps off */
#ifndef ADV0130_BLIFT_H
#define ADV0130_BLIFT_H

#include "system/task.h"

/* the step egg_carrier_inside_object_kinds names for O BLIFT. the task
 * needs a work and a force */
void egg_carrier_blift_step(struct task *task);

#endif
