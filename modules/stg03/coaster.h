/* the coaster, which the set editor names O COASTER: a train of four cars.
 * player 0 boards its front car, and the train carries Sonic or Amy along
 * its path */
#ifndef STG03_COASTER_H
#define STG03_COASTER_H

#include "effect/pools.h"
#include "system/task.h"

/* the sparkles' pool, which particle_create_pools makes as the stage starts */
extern struct particle_pool_spec twinkle_coaster_sparkle_pool;

/* the step twinkle_object_kinds names for O COASTER. its task needs a work,
 * and player 0 must exist */
void twinkle_coaster_step(struct task *task);

#endif
