/* flames: sprites that play flame01 to 10 once as they rise */
#ifndef EFFECT_FLAME_H
#define EFFECT_FLAME_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/pools.h"

struct dust_params;

/* flame01 to 10 */
extern NJS_TEXLIST flame_texlist;
extern struct particle_pool_spec flame_pool;

/* a flame from flame_pool at pos, moving at speed, drawn at scale * 0.15,
 * with params of its own. both creates start a flame at a random frame
 * below 5, turned 100 to 260 degrees at random. each makes none when the
 * pool is null or no particle is free */
void flame_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale);
/* a flame as params give it, kept, not copied, for the flame's life. it
 * plays rate frames a step from frame 6 too, where a puff plays twice
 * rate */
void flame_create_from(struct dust_params *params);

#endif
