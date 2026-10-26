/* snow: puffs that grow, slow and rise, from a pool of their own */
#ifndef EFFECT_SNOW_H
#define EFFECT_SNOW_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "effect/pools.h"

/* snowp01 to snowp08, which the egg carrier's and Final Egg's puffs draw
 * too */
extern NJS_TEXLIST snow_texlist;
extern struct particle_pool_spec snow_pool;

/* a white puff from snow_pool at pos, moving at speed, drawn at scale, with
 * params of its own. both creates start a puff at a random frame below 4,
 * turned 22.5 to 45 degrees at random. each makes none when the pool is
 * null or no particle is free */
void snow_create(NJS_POINT3 *pos, const NJS_VECTOR *speed, Float scale);
/* a puff as params give it, kept, not copied, for the puff's life.
 * params->scale and growth set its size itself, not in DUST_SIZEs. it plays
 * rate frames a step from frame 6 too, where a dust puff plays twice
 * rate */
void snow_create_from(struct dust_params *params);

#endif
