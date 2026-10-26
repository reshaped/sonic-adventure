/* O EGGLIFT, a lift that takes player 0 up from inside the Egg Carrier to its
 * deck */
#ifndef ADV0130_EGG_LIFT_H
#define ADV0130_EGG_LIFT_H

#include "system/task.h"

/* the step egg_carrier_inside_object_kinds names for O EGGLIFT. the task
 * needs a work and a force */
void egg_carrier_egg_lift_step(struct task *task);

#endif
