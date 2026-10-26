/* smoke puff particles shared by Final Egg and Hot Shelter objects */
#ifndef STG10_PUFF_H
#define STG10_PUFF_H

#include "effect/dust.h"
#include "effect/pools.h"

/* final_pools, and 1ST_READ.BIN's shelter_pools, name it */
extern struct particle_pool_spec final_puff_pool;

/* start a smoke puff from params, or none when final_puff_pool wasn't made
 * or has no free particle. the puff takes its position, speed, scale and
 * color from params. it keeps params, not a copy, and reads them each step,
 * so they must outlive it */
void final_puff_create(struct dust_params *params);

#endif
