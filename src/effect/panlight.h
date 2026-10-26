/* panlights: glows that move as dust does, fading and shrinking as they go */
#ifndef EFFECT_PANLIGHT_H
#define EFFECT_PANLIGHT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "effect/pools.h"

extern NJS_TEXLIST panlight_texlist;
extern struct particle_pool_spec panlight_pool;

/* a glow from panlight_pool as params give it, kept, not copied, for the
 * glow's life. it starts at params->scale * 0.15, turned at random, and
 * moves as a puff does. each step its alpha falls by rate / 10, and its
 * size by growth * 0.015 of itself. it ends once its alpha falls below 0.
 * it makes none when the pool is null or no particle is free */
void panlight_create(struct dust_params *params);

#endif
