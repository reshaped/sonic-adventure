/* B_E101_R's stage task and its sky task */
#ifndef B_E101_R_E101_R_H
#define B_E101_R_E101_R_H

#include "system/task.h"

/* the steps 1ST_READ.BIN's stage_steps and sky_steps give the stage's task
 * and its sky's */
void e101_r_step(struct task *task);
void e101_r_sky_step(struct task *task);

#endif
