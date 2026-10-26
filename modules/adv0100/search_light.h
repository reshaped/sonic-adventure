/* O S LIGHT, a searchlight. its neck turns and its head swings, and a hit on
 * the head puts its beam out or back on */
#ifndef ADV0100_SEARCH_LIGHT_H
#define ADV0100_SEARCH_LIGHT_H

#include "system/task.h"

/* the step egg_carrier_object_kinds names for O S LIGHT. the task needs a
 * work and a motion */
void egg_carrier_search_light_step(struct task *task);

#endif
