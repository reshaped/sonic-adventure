/* O SMOKE, a fire that puffs smoke and throws flames */
#ifndef ADV03_SMOKE_H
#define ADV03_SMOKE_H

#include "system/task.h"

/* the step the object list names for O SMOKE. it registers the fire's
 * collision on each step. every 16 steps it adds a puff of smoke to a pool of
 * its own, which the task's destructor kills. while the camera is within 500
 * of it, it adds flames to flame_pool. once placement_try_leave finds the
 * task out of range, the step does nothing, and the task destroys itself on its
 * next step */
void past_smoke_step(struct task *task);

#endif
