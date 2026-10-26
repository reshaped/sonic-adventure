/* O GATESIDE: two solid posts, one at each side of a gate */
#ifndef STG12_GATE_SIDE_H
#define STG12_GATE_SIDE_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record */
void gate_side_step(struct task *task);

#endif
