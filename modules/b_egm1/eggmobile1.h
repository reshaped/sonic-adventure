/* B_EGM1's stage task and its sky's task */
#ifndef B_EGM1_EGGMOBILE1_H
#define B_EGM1_EGGMOBILE1_H

#include "system/task.h"

/* the stage's task step, which 1ST_READ.BIN's stage_steps names. its first
 * step starts the fight with the Egg Hornet */
void eggmobile1_step(struct task *task);
/* the sky's task step, which 1ST_READ.BIN's sky_steps names. it draws the
 * sky and the sea, centered on the camera in x and z */
void eggmobile1_sky_step(struct task *task);

#endif
