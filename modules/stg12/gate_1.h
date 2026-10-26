/* O GATE1: two sliding doors that open for player 0 from the side they
 * face */
#ifndef STG12_GATE_1_H
#define STG12_GATE_1_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record */
void gate_1_step(struct task *task);

#endif
