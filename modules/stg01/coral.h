/* O CORA and O CORB, two corals. each draws scaled by one more than its set
 * entry's scale on each axis */
#ifndef STG01_CORAL_H
#define STG01_CORAL_H

#include "system/task.h"

/* the steps the object list names for O CORA and O CORB */
void coral_a_step(struct task *task);
void coral_b_step(struct task *task);

#endif
