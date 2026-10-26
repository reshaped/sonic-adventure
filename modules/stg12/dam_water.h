/* the surface of the water act 0's dam holds back */
#ifndef STG12_DAM_WATER_H
#define STG12_DAM_WATER_H

#include "system/task.h"

/* the first step of a task with a work. the dam makes the task as its child
 * as the dam starts */
void shelter_dam_water_start(struct task *task);

#endif
