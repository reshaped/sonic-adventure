/* O SIGNAL solid signal, which cycles through three timed phases */
#ifndef ADV00_SIGNAL_H
#define ADV00_SIGNAL_H

#include "system/task.h"

/* the step the object list names for O SIGNAL. it starts the signal on its
 * first step while the set editor is closed, and runs it on each after */
void signal_step(struct task *task);

#endif
