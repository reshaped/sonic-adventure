/* Twinkle Park's stage task and sky task. the stage task moves play between
 * the acts. the sky task lights each act and draws its sky */
#ifndef STG03_TWINKLE_H
#define STG03_TWINKLE_H

#include "system/task.h"

/* the steps 1ST_READ.BIN's stage_steps and sky_steps name for the stage. the
 * stage task reads through null when it can't make an act's music task */
void twinkle_step(struct task *task);
void twinkle_sky_step(struct task *task);

#endif
