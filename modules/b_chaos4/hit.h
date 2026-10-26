/* Chaos 4's hit: he bursts into drops, and forms again as they fly back */
#ifndef B_CHAOS4_HIT_H
#define B_CHAOS4_HIT_H

#include "chaos/chaos.h"
#include "system/task.h"

/* his step in BOSS_HIT, which his task's step calls. extra is his struct
 * chaos */
void chaos4_hit(struct task_work *work, struct task_motion *motion,
                void *extra);

#endif
