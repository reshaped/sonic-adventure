/* O GRAY BLD, O BROWN BLD and O RED BLD: the Perfect Chaos fight's buildings,
 * which blow apart */
#ifndef B_CHAOS7_BUILDING_H
#define B_CHAOS7_BUILDING_H

#include "system/task.h"

/* the steps the object list names */
void chaos7_gray_building_step(struct task *task);
void chaos7_brown_building_step(struct task *task);
void chaos7_red_building_step(struct task *task);

#endif
