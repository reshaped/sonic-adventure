/* the Chao race's sky, a dome around the camera, drawn only on the course */
#ifndef AL_RACE_SKY_H
#define AL_RACE_SKY_H

#include "system/task.h"

/* the step 1ST_READ.BIN's sky_steps gives STAGE_ALIFE_RACE's sky task, which
 * makes the task the race's sky */
void race_sky_step(struct task *task);

#endif
