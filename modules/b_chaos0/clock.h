/* O BCLOCK clock that starts at ten o'clock and advances one minute every 361
 * steps */
#ifndef B_CHAOS0_CLOCK_H
#define B_CHAOS0_CLOCK_H

#include "system/task.h"

/* the step the object list names for O BCLOCK. its first step starts the
 * clock, and each one after runs it */
void chaos0_clock_step(struct task *task);

#endif
