/* Chaos 4's surfacing, the only mode in which a hit on him counts */
#ifndef B_CHAOS4_SURFACE_H
#define B_CHAOS4_SURFACE_H

#include "system/task.h"

/* his step in BOSS_SURFACE, which his task's step calls. extra is his
 * struct chaos */
void chaos4_surface(struct task_work *work, struct task_motion *motion,
                    void *extra);

#endif
