/* the spill of act 0's dam, which pours over four sides of the wall as the
 * wall sinks */
#ifndef STG12_DAM_SPILL_H
#define STG12_DAM_SPILL_H

#include "system/task.h"

/* the first step of a task with a work. the dam makes the task as its child
 * once the wall's top sinks to its water */
void shelter_dam_spill_start(struct task *task);

#endif
