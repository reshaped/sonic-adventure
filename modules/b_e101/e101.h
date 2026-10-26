/* B_E101's stage task and its sky's task */
#ifndef B_E101_E101_H
#define B_E101_E101_H

#include "system/task.h"

/* the tasks 1ST_READ.BIN starts for the stage, from its stage_steps and
 * sky_steps */
void e101_step(struct task *task);
void e101_sky_step(struct task *task);

#endif
