/* Chaos 6's hints: what Tikal tells the player during the fight */
#ifndef B_CHAOS6_HINT_H
#define B_CHAOS6_HINT_H
#include <sg_xpt.h>

#include "system/task.h"

/* TRUE for the 600 steps after a hint shows, while chaos6_hint_step holds
 * the next one off, and FALSE otherwise */
extern Sint8 chaos6_hint_waiting;

/* the step of the hints' task, a child the boss creates as the fight
 * first begins. Tikal gives each hint at most once */
void chaos6_hint_step(struct task *task);

#endif
