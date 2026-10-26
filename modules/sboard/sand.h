/* sand: the puffs a board throws up on the sand hill */
#ifndef SBOARD_SAND_H
#define SBOARD_SAND_H

#include "effect/dust.h"
#include "effect/pools.h"

/* the puffs' pool, which draws snabop01 to 08 from eff_sandboard_texlist */
extern struct particle_pool_spec sand_pool;

/* a puff from sand_pool, which keeps params as dust_create_from does. it
 * starts at a random frame below 4, turned between 22.5 and 45 degrees. it
 * steps by params as dust_step does, but for two things. it takes scale and
 * growth as they are, not in DUST_SIZEs. its rate stays the same from frame
 * 6 on. it makes none when sand_pool.pool is null or no particle is free */
void sand_create_from(struct dust_params *params);

#endif
