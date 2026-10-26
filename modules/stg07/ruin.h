/* the ruin's stage task and sky task. the stage task starts each act's
 * music. at the ends of acts 0 and 1, it moves player 0 on to the next
 * act */
#ifndef STG07_RUIN_H
#define STG07_RUIN_H

#include "system/task.h"

/* the tasks 1ST_READ.BIN starts for the stage, from its stage_steps and
 * sky_steps */
void ruin_step(struct task *task);
void ruin_sky_step(struct task *task);

#endif
