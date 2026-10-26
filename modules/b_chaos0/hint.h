/* Chaos 0's hints: what Tikal tells the player during the fight */
#ifndef B_CHAOS0_HINT_H
#define B_CHAOS0_HINT_H

#include "system/task.h"

/* the step of the hints' task, which the boss makes as its child as the
 * fight starts. each hint shows once, and none within 600 steps of the
 * last */
void chaos0_hint_step(struct task *task);

#endif
