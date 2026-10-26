/* the crane the object list names O_CRANE. its turning arm carries two
 * handles for player 0 to hang from */
#ifndef STG06_CRANE_H
#define STG06_CRANE_H

#include "system/task.h"

/* the step the object list names for O_CRANE. its first step makes the two
 * handles as children, and writes through null if task_create_child can't
 * make one */
void crane_step(struct task *task);

#endif
