/* O BCLOCK clock that starts at ten o'clock and advances one minute every 3601
 * steps */
#ifndef ADV00_CLOCK_H
#define ADV00_CLOCK_H

#include "system/task.h"

/* the step the object list names for O BCLOCK. its first step starts the
 * clock, and each one after runs it. in the set editor it draws the clock and
 * leaves its time as it is */
void clock_step(struct task *task);

#endif
