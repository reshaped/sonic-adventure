/* the past's particle pool specs, ended by NULL */
#include "adv03/pools.h"
#include "effect/pools.h"

extern struct particle_pool_spec dat_0c9c9b1c;
extern struct particle_pool_spec dat_0c9c9bec;
extern struct particle_pool_spec past_smoke_pool;

#pragma section HEAD
struct particle_pool_spec *past_pools[4] = {&dat_0c9c9b1c, &dat_0c9c9bec,
                                            &past_smoke_pool, NULL};
#pragma section
