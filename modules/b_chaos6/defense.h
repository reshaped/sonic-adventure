/* Chaos 6's guard flash */
#ifndef B_CHAOS6_DEFENSE_H
#define B_CHAOS6_DEFENSE_H

#include "system/task.h"

/* the guard flash's first step, for a task with a work. the flash keeps to
 * the boss's side toward work->pos, 38 from him across, at work->pos.y, as
 * he moves. it fades in, plays once, fades out and kills its task */
void chaos6_defense_step(struct task *task);

#endif
