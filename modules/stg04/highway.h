/* Speed Highway's stage, act and sky tasks. the stage task makes the act
 * task. the act task starts each act's held sound, and makes the task that
 * asks for its music. at the ends of acts 0 and 1, it moves player 0 on to
 * the next act. the sky task sets each act's fog and light and draws its
 * sky */
#ifndef STG04_HIGHWAY_H
#define STG04_HIGHWAY_H

#include "system/task.h"

/* task steps that stage_steps and sky_steps name for the stage */
void highway_step(struct task *task);
void highway_sky_step(struct task *task);

#endif
