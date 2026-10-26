/* the cart course's sky task, which draws a backdrop around the camera */
#ifndef MINICART_SKY_H
#define MINICART_SKY_H

#include "system/task.h"

/* the sky task's step, which 1ST_READ.BIN's sky_steps names */
void cart_sky_step(struct task *task);

#endif
