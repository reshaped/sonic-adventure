/* the Chao race's stage task, which creates the game, steps it in the hall
 * and on the course, and changes act when the game asks */
#ifndef AL_RACE_RACE_H
#define AL_RACE_RACE_H

#include "system/task.h"

/* the stage task's step. 1ST_READ.BIN's stage_steps names it for
 * STAGE_ALIFE_RACE, and stage_create_task creates the stage task with it */
void alife_race_step(struct task *task);

#endif
