/* Chaos 6's snow: puffs from a pool of the module's own */
#ifndef B_CHAOS6_SNOW_H
#define B_CHAOS6_SNOW_H

#include "effect/dust.h"
#include "effect/pools.h"

/* the snow's pool. 1ST_READ.BIN's chaos6_pools lists it, so
 * particle_create_pools makes it for the stage */
extern struct particle_pool_spec chaos6_snow_pool;

/* a puff from chaos6_snow_pool as params give it, made as
 * dust_create_from makes one from dust_pool. the puff keeps params and reads
 * them each step, so they must outlive it */
void chaos6_snow_create_from(struct dust_params *params);

#endif
