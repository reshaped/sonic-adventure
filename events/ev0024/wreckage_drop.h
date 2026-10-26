/* EV0024.PRS's falling wreckage: a task that drops pieces at an interval */
#ifndef EV0024_WRECKAGE_DROP_H
#define EV0024_WRECKAGE_DROP_H
#include <sg_xpt.h>

#include "system/task.h"

/* make a task that drops a piece of wreckage every interval + 2 steps, until
 * it is killed. each piece starts at height y, within spread of x and of z,
 * at scale. its speed on each axis is random, up to speed either way. null
 * when task_create fails. when heap_alloc fails at the task's first step,
 * the task kills itself */
struct task *wreckage_drop_create(Float x, Float y, Float z, Float speed,
                                  Float scale, Float spread, Sint16 interval);

#endif
