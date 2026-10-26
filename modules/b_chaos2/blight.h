/* a light at Chaos 2's feet as he spreads his puddle over the floor, or as
 * he bursts. it is blight_object, which changes shape through its motions
 * while its texture cycles, and darkens away */
#ifndef B_CHAOS2_BLIGHT_H
#define B_CHAOS2_BLIGHT_H

#include "system/task.h"

/* a task's step that plays the light at the work's pos and scale, which
 * the light's maker sets. the light plays its motions' 15 frames over 45
 * steps, and destroys its task on the 46th */
void chaos2_blight_step(struct task *task);

#endif
