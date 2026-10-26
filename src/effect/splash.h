/* splashes: drops that fly up out of the water, and the ripple that spreads
 * where something meets it */
#ifndef EFFECT_SPLASH_H
#define EFFECT_SPLASH_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "effect/pools.h"

/* splash_a01 to 08 */
extern NJS_TEXLIST splash_texlist;
extern struct particle_pool_spec splash_pool;

/* a white drop from splash_pool at pos, moving at speed, drawn at
 * scale * 0.17, with params of its own. both creates start a drop at a
 * random frame below 5, turned 100 to 260 degrees at random. a drop plays
 * 1.4 times rate frames a step, and twice that from frame 6. each create
 * makes none when the pool is null or no particle is free */
void splash_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale);
/* a drop as params give it, kept, not copied, for the drop's life. it
 * starts at params->scale itself, not in DUST_SIZEs, and grows by
 * growth * 0.255 a step */
void splash_create_from(struct dust_params *params);
/* a ripple task at level 6 at pos, at a scale of size in x and z. it grows
 * by growth->x each step, and growth->y is unread. once its size passes
 * growth->z, it fades over the next 26 steps as it grows on, and ends on
 * the step after. when no task can be made, it writes through a null task */
void ripple_create(NJS_POINT3 *pos, NJS_VECTOR *growth, Float size);

#endif
