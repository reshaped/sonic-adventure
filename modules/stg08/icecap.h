/* Ice Cap's stage task, and its sky task, which sets each act's light and
 * fog */
#ifndef STG08_ICECAP_H
#define STG08_ICECAP_H

#include "system/task.h"

/* the value the snow board's stand stores in the stage task's work->flags as
 * a player takes the board. the stage task, waiting for it, then starts the
 * board run's opening */
#define ICECAP_BOARD_TAKEN 8

/* task steps that stage_steps and sky_steps name for the stage */
void icecap_step(struct task *task);
void icecap_sky_step(struct task *task);

#endif
