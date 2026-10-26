/* the particle pools the stage makes */
#include "effect/pools.h"
#include "sboard/pools.h"
#include "sboard/sand.h"

/* create_pools stops at the null */
#pragma section HEAD
struct particle_pool_spec *sunaboard_pools[2] = {&sand_pool, NULL};
#pragma section
