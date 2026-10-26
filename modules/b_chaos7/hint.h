/* Tikal's hint in the Perfect Chaos fight */
#ifndef B_CHAOS7_HINT_H
#define B_CHAOS7_HINT_H

#include "system/task.h"

/* the hint's step, for a task with a work, which the fight starts in its
 * opening. it shows the hint once the stage has counted a retry */
void chaos7_hint_step(struct task *task);

#endif
