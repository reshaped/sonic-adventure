/* Chaos 4's stage: its own task and its sky's */
#ifndef B_CHAOS4_CHAOS4_H
#define B_CHAOS4_CHAOS4_H

#include "system/task.h"

/* the tasks 1ST_READ.BIN starts for the stage, which its stage_steps and
 * sky_steps name */
void chaos4_step(struct task *task);
void chaos4_sky_step(struct task *task);

#endif
