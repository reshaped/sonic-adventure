/* the trail a thrown drop of Chaos 2's leaves. the drop makes a sprite
 * each step it flies, and each sprite shrinks away */
#ifndef B_CHAOS2_TRAIL_H
#define B_CHAOS2_TRAIL_H

#include "system/task.h"

/* a task's step that plays a sprite of the trail at the work's pos. the
 * sprite shrinks away over 15 steps, and destroys its task on the 16th */
void chaos2_trail_step(struct task *task);

#endif
