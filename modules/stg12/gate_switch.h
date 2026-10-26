/* O GATE SW: a gate that rises out of the way once its switch is on */
#ifndef STG12_GATE_SWITCH_H
#define STG12_GATE_SWITCH_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record */
void gate_switch_step(struct task *task);

#endif
