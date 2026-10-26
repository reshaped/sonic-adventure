/* Chaos 6's snow: puffs of snowp01 to snowp08, from a pool of the module's
 * own, that step as dust puffs do */
#include <stdlib.h>

#include "b_chaos6/snow.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "system/util.h"

extern struct particle_anim chaos6_snow_anim;

#pragma section HEAD
struct particle_pool_spec chaos6_snow_pool = {
#include "b_chaos6/snow/chaos6_snow_pool.inc"
};
#pragma section

void chaos6_snow_create_from(struct dust_params *params)
{
    struct particle_pool *pool = chaos6_snow_pool.pool;
    struct particle *particle = particle_alloc(pool, dust_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale * DUST_SIZE;
        particle->time = rand() * RAND_SCALE * 6.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}
