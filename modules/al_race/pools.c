/* the race's own particle pools: the list holds only the null that ends it,
 * so the race makes none */
#include "al_race/pools.h"
#include "effect/pools.h"

#pragma section HEAD
struct particle_pool_spec *race_pools[1] = {NULL};
#pragma section
