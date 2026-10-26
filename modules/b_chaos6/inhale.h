/* Chaos 6's inhale: the swirl at his mouth while he breathes in */
#ifndef B_CHAOS6_INHALE_H
#define B_CHAOS6_INHALE_H

#include "system/task.h"

/* the step of the swirl's task, a child of the boss's that needs a work.
 * chaos6_inhale_reset must have run first: the step scrolls the cones' uvs
 * in place from what it kept. the swirl fades in, holds while the boss
 * plays boss.h's BODY_INHALING, and fades out. its task then kills itself */
void chaos6_inhale_step(struct task *task);
/* keep the cones' v coordinates, which the swirl's task scrolls from. the
 * first call keeps them, and later calls leave what it kept */
void chaos6_inhale_reset(void);

#endif
