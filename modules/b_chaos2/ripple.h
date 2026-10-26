/* the ripple where Chaos 2's ball lands. it is ripple_object, which its
 * action spreads while its texture cycles, and it darkens away */
#ifndef B_CHAOS2_RIPPLE_H
#define B_CHAOS2_RIPPLE_H

#include "system/task.h"

/* a task's step that plays the ripple at the work's pos, which its first
 * step raises by 1. the ripple plays its action's 7 frames over 60 steps,
 * and destroys its task on the 61st */
void chaos2_ripple_step(struct task *task);

#endif
