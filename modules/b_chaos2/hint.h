/* Chaos 2's hints: what Tikal tells the player during the fight */
#ifndef B_CHAOS2_HINT_H
#define B_CHAOS2_HINT_H

#include "system/task.h"

/* the step of the hints' task. the boss makes the task once the camera's
 * swing round him at the fight's start is done, and destroys it with his
 * own task. each hint shows once, and none within 600 steps of the last */
void chaos2_hint_step(struct task *task);

#endif
