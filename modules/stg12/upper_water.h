/* the upper pool's surface in act 0, which players swim in, and a second
 * piece of its water. only the collision world holds the second piece. the
 * surface follows the pool down as it pours, then fades */
#ifndef STG12_UPPER_WATER_H
#define STG12_UPPER_WATER_H

#include "system/task.h"

/* the surface's first step. the upper pool creates the surface as its
 * child, with the work and the force the surface needs */
void shelter_upper_water_start(struct task *task);

#endif
