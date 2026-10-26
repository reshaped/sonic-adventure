/* O ROAD 0 to O ROAD 4: the Perfect Chaos fight's roads above the sea */
#ifndef B_CHAOS7_ROAD_H
#define B_CHAOS7_ROAD_H

#include "system/task.h"

/* each road's step, for a placed task with a work. chaos7_object_kinds names
 * all five */
void chaos7_road_0_step(struct task *task);
void chaos7_road_1_step(struct task *task);
void chaos7_road_2_step(struct task *task);
void chaos7_road_3_step(struct task *task);
void chaos7_road_4_step(struct task *task);

#endif
