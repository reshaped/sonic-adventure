/* Sand Hill's sky task, which draws a backdrop around the camera and sets
 * the stage's fog and light */
#ifndef SBOARD_SKY_H
#define SBOARD_SKY_H

#include "system/task.h"

/* the sky task's step, which 1ST_READ.BIN's sky_steps names */
void sunaboard_sky_step(struct task *task);

#endif
