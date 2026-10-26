/* the Egg Walker's stage: the steps of its stage task and its sky task */
#ifndef B_EGM2_EGGMOBILE2_H
#define B_EGM2_EGGMOBILE2_H

#include "system/task.h"

/* the stage task's step, whose address 1ST_READ.BIN's stage_steps names.
 * its first two steps stop the time. the second also starts the boss's
 * music and reads STATION_SQUARE_STATION's lights from the disc. then it
 * makes the boss. each step after holds player 0 within the stretch of street
 * the fight takes up */
void eggmobile2_step(struct task *task);
/* the sky task's step, whose address 1ST_READ.BIN's sky_steps names. its
 * first step sets the color behind the land. each step turns the fog off
 * around a push and pop of the matrix, which draw nothing */
void eggmobile2_sky_step(struct task *task);

#endif
