/* O GG solid arch over the road, and O FF solid fence */
#ifndef STG04_GATE_H
#define STG04_GATE_H

#include "system/task.h"

/* task steps for O GG and O FF, named in the object list */
void highway_gate_step(struct task *task);
void fence_ff_step(struct task *task);

#endif
