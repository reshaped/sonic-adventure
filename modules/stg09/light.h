/* O LHTR and O LHTG lights, which go on and off for random times */
#ifndef STG09_LIGHT_H
#define STG09_LIGHT_H

#include "system/task.h"

/* the object list's steps. a light stays on for up to scl.x steps, and then
 * off for up to scl.y. each count is random. a light plays its sound while
 * it's on */
void light_r_step(struct task *task);
void light_g_step(struct task *task);

#endif
