/* the Egg Carrier's puffs: snow sprites that grow, slow, rise and fade */
#ifndef ADV01OBJ_PUFF_H
#define ADV01OBJ_PUFF_H

#include "effect/dust.h"
#include "effect/pools.h"

/* one of egg_carrier_pools, so made as each Egg Carrier stage starts */
extern struct particle_pool_spec egg_carrier_puff_pool;

/* start a puff from params, or none when egg_carrier_puff_pool wasn't made
 * or has no free particle. the puff takes its position, speed, scale and
 * color from params. it keeps params, not a copy, and reads it each step,
 * so params must outlive it */
void egg_carrier_puff_create(struct dust_params *params);

#endif
