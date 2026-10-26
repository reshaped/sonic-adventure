/* Final Egg stage task, the sky task that sets each act's fog, and a light
 * task */
#ifndef STG10_FINAL_H
#define STG10_FINAL_H

#include "system/task.h"

/* task steps that stage_steps and sky_steps name for the stage */
void final_step(struct task *task);
void final_sky_step(struct task *task);
/* a light task's step, which neither this image nor 1ST_READ.BIN points at.
 * its first step makes and turns on a directional light of its own. each
 * later step gives the stage's dir_light its own light's intensities, color
 * and direction */
void final_light_step(struct task *task);

#endif
