/* the particle pools the Egg Carrier adds to the ones every stage has */
#ifndef ADV01OBJ_POOLS_H
#define ADV01OBJ_POOLS_H

#include "effect/pools.h"

/* the list, ended by a null, of the pools particle_create_pools makes for the
 * Egg Carrier and its inside. 1ST_READ.BIN's stage_pools names it by address
 * for both */
extern struct particle_pool_spec *egg_carrier_pools[];

#endif
