/* the Perfect Chaos fight's water column, which rises and sinks again */
#ifndef B_CHAOS7_COLUMN_H
#define B_CHAOS7_COLUMN_H

#include "system/task.h"

/* a water column's step. Chaos's breath and the falling pieces of cars,
 * walls and buildings start one where they meet the sea. its first step sets
 * the scl.x they give it to 0 */
void chaos7_column_step(struct task *task);

#endif
