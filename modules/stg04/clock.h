/* O Tokei clock on a column with a ticking pendulum */
#ifndef STG04_CLOCK_H
#define STG04_CLOCK_H

#include "system/task.h"

/* task step for O Tokei, named in the object list */
void highway_clock_step(struct task *task);

#endif
