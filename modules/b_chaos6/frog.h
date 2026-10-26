/* Chaos 6's frog: Big's frog, inside the boss until Big hooks it */
#ifndef B_CHAOS6_FROG_H
#define B_CHAOS6_FROG_H

#include "system/task.h"

/* the step of the frog's task, which chaos6_begin creates for Big with a
 * work and a motion. the step reads the motion as a fish_motion. it does
 * nothing while big_lure or the lure's work is null. the frog rides inside
 * the boss until Big hooks it, and Big then reels it in. once it is within
 * 20 of player 0, it lands and the stage is clear. its next step kills the
 * task */
void chaos6_frog_step(struct task *task);
/* set camera_state's target at the frog, and its pos 20 from the frog
 * toward player 0. the frog's task must have run a step with a lure out.
 * nothing in the image calls it */
void chaos6_frog_camera(void);

#endif
