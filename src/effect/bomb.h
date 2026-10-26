/* bombs: a puff that plays bomb01 to 16 once */
#ifndef EFFECT_BOMB_H
#define EFFECT_BOMB_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/pools.h"

/* bomb01 to 16 */
extern NJS_TEXLIST bomb_texlist;
extern struct particle_pool_spec bomb_pool;

/* a white puff from bomb_pool at pos, drawn at scale * 0.5. it makes none
 * when the pool is null or no particle is free */
void bomb_create(NJS_POINT3 *pos, Float scale);

#endif
