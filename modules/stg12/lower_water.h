/* act 0's water surface below the dam, with a floating island and three more
 * pieces of water. the surface rises with act 0's water, and raises the
 * island with it. the third and fourth pieces start once the water is
 * SHELTER_WATER_FULL */
#ifndef STG12_LOWER_WATER_H
#define STG12_LOWER_WATER_H

#include "system/task.h"

/* the surface's first step. shelter_water_start creates the surface. task
 * needs TASK_WORK's and TASK_FORCE's records */
void shelter_lower_water_start(struct task *task);

#endif
