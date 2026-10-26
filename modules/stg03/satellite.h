/* O SATELLITE, a dish that turns and lifts the players who touch it */
#ifndef STG03_SATELLITE_H
#define STG03_SATELLITE_H

#include "system/task.h"

/* task step for O SATELLITE, named in the object list. outside Twinkle
 * Park's act 0 and STAGE_CART, the satellite neither draws nor collides */
void satellite_step(struct task *task);

#endif
