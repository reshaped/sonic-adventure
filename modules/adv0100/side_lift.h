/* O SIDELIFT, a lift that carries player 0 between its placement and a far
 * end its scale sets */
#ifndef ADV0100_SIDE_LIFT_H
#define ADV0100_SIDE_LIFT_H

#include "system/task.h"

/* the step egg_carrier_object_kinds names for O SIDELIFT. the task needs a
 * work, a motion and a force */
void egg_carrier_side_lift_step(struct task *task);

#endif
