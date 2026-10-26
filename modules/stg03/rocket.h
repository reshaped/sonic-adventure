/* O ROCKET, a ride whose tiers turn under eight rocking arms */
#ifndef STG03_ROCKET_H
#define STG03_ROCKET_H

#include "system/task.h"

/* task step for O ROCKET, named in the object list. outside Twinkle Park's
 * act 0 and STAGE_CART, the ride neither moves nor draws */
void twinkle_rocket_step(struct task *task);

#endif
