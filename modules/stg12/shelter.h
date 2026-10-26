/* Hot Shelter's stage task, and its sky task, which sets each act's view.
 * in acts 0 to 2, the sky task also sets the fog and the far clip */
#ifndef STG12_SHELTER_H
#define STG12_SHELTER_H

#include "system/task.h"

/* task steps that stage_steps and sky_steps name for the stage */
void shelter_step(struct task *task);
void shelter_sky_step(struct task *task);

#endif
