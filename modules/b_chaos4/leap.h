/* Chaos 4's leap out of the swamp, and the columns of water it sends up */
#ifndef B_CHAOS4_LEAP_H
#define B_CHAOS4_LEAP_H

#include "chaos/chaos.h"
#include "system/task.h"

/* his step in BOSS_LEAP, which his task's step calls */
void chaos4_leap(struct task_work *work, struct task_motion *motion,
                 struct chaos *chaos);

#endif
