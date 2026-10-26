/* Chaos 6's shockwave: a ring that spreads over the deck */
#ifndef B_CHAOS6_SHOCKWAVE_H
#define B_CHAOS6_SHOCKWAVE_H

#include "system/task.h"

/* start a shockwave: a ring 8 above the deck. its center starts 15 behind
 * from's pos, against the heading (cos, sin) of from's ang.y in x and z. it
 * spreads for 70 steps, and its task kills itself on the 71st. a ball at
 * the ring's edge hurts player 0 where the edge reaches him. nothing when
 * task_create fails */
void chaos6_shockwave_create(struct task_work *from);

#endif
