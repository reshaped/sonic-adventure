/* O GATE2 and GATE13: gates that a handle raises */
#ifndef STG12_GATE_2_H
#define STG12_GATE_2_H

#include "system/task.h"

/* the object list's steps. task needs TASK_MOTION's and TASK_WORK's
 * records */
void gate_2_step(struct task *task);
void gate_13_step(struct task *task);

#endif
