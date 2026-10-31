/* Amy's fight with Zero, on the Egg Carrier's deck: its stage and sky
 * steps */
#ifndef B_ROBO_EGGMANROBO_H
#define B_ROBO_EGGMANROBO_H

#include "system/task.h"

/* the stage's step and its sky's, which 1ST_READ.BIN's stage_steps and
 * sky_steps name */
void eggmanrobo_step(struct task *task);
void eggmanrobo_sky_step(struct task *task);

#endif
