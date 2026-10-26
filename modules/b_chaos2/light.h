/* a light where Chaos 2 bursts into his drops. its action turns
 * light_object and scales it from wide and flat to narrow and tall, while
 * its texture cycles, and the light darkens away */
#ifndef B_CHAOS2_LIGHT_H
#define B_CHAOS2_LIGHT_H

#include "system/task.h"

/* a task's step that plays the light at the work's pos. the light plays its
 * action's 15 frames over 91 steps, and destroys its task on the 92nd */
void chaos2_light_step(struct task *task);

#endif
