/* screen fade to black and back as the cargo ride starts and ends */
#ifndef STG12_CARGO_FADE_H
#define STG12_CARGO_FADE_H

#include "system/task.h"

/* the first step of a task with a work. the task fades the screen to black,
 * holds it there for 31 steps, fades it back and ends */
void cargo_fade_start(struct task *task);

#endif
