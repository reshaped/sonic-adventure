/* the island act's ground, whose task puts it in the collision world and
 * draws it */
#ifndef ADV02_ISLAND_LAND_H
#define ADV02_ISLAND_LAND_H

#include "system/task.h"

/* the step of the ground's task, which the Mystic Ruins field task creates as
 * the island act starts */
void island_land_step(struct task *task);

#endif
