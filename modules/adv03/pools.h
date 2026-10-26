/* the past's particle pool specs */
#ifndef ADV03_POOLS_H
#define ADV03_POOLS_H

#include "effect/pools.h"

/* a list in the form of 1ST_READ.BIN's stage_pools, ended by NULL. no code
 * reads it: stage_pools' row for the past is NULL */
extern struct particle_pool_spec *past_pools[];

#endif
