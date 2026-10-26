/* O FIRESMOKE, puffs of smoke over a fire */
#ifndef ADV03_FIRE_SMOKE_H
#define ADV03_FIRE_SMOKE_H

#include "system/task.h"

/* the step the object list names for O FIRESMOKE. every 24 steps it asks
 * past_smoke_pool for a puff. no puff comes, since nothing makes the pool.
 * once placement_try_leave finds the task out of range, the step does
 * nothing, and the task destroys itself on its next step */
void fire_smoke_step(struct task *task);

#endif
