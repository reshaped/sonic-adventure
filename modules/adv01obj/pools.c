/* the particle pools the Egg Carrier adds to the ones every stage has */
#include "adv01obj/pools.h"
#include "adv01obj/puff.h"
#include "effect/pools.h"
#include "effect/splash.h"

struct particle_pool_spec *egg_carrier_pools[] = {
    &egg_carrier_puff_pool,
    &splash_pool,
    NULL,
};
