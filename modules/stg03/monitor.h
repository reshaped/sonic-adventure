/* O MONITOR screen with a scrolling picture, under a rocking speaker */
#ifndef STG03_MONITOR_H
#define STG03_MONITOR_H

#include "system/task.h"

/* task step for O MONITOR, named in the object list. its set entry's scale
 * x, rounded down, is the picture's scroll a step. scale y and scale z are the
 * speaker's turn a step and at the start, in degrees */
void monitor_step(struct task *task);

#endif
