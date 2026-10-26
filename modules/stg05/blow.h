/* O BLOW and O BLOW2, blowers that draw nothing. each advances its phases
 * and counters, and O BLOW plays a sound once every 318 steps */
#ifndef STG05_BLOW_H
#define STG05_BLOW_H

#include "system/task.h"

/* the object list's steps */
void blow_step(struct task *task);
void blow_2_step(struct task *task);

#endif
