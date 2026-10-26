/* act 0's deep water beside the upper pool: a surface the collision world
 * holds as water. once the water below has risen, the surface rides above
 * player 0, no higher than the pool. once that water is full, the surface
 * settles at that water's level and draws */
#ifndef STG12_DEEP_WATER_H
#define STG12_DEEP_WATER_H

#include "system/task.h"

/* the first step of a task with a work and a force, which
 * shelter_water_start makes */
void shelter_deep_water_start(struct task *task);

#endif
