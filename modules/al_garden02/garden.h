/* the Mystic Ruins garden: the stage's task */
#ifndef AL_GARDEN02_GARDEN_H
#define AL_GARDEN02_GARDEN_H

#include "system/task.h"

/* the stage task's step, which 1ST_READ.BIN's stage_steps names by its
 * address, 0x0cb80000, for STAGE_ALIFE_MR */
void mystic_ruins_garden_step(struct task *task);

#endif
