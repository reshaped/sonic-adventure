/* hits: flashes that drift and grow, and lightning that plays its frames
 * once, from two pools */
#ifndef EFFECT_HIT_H
#define EFFECT_HIT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "effect/pools.h"

/* hit_l01 to 08 */
extern NJS_TEXLIST hit_texlist;
extern struct particle_pool_spec hit_pool;
/* mlightning01 to 08, and their eight frames */
extern NJS_TEXLIST lightning_texlist;
extern NJS_TEXANIM lightning_frames[];
extern struct particle_pool_spec lightning_pool;

/* a flash from hit_pool at the world's origin, drawn at scale, with params
 * of its own. pos is unread, and the flash's speed is what its particle last
 * held. it starts at a random frame below 4. both flash creates turn a flash
 * 22.5 to 45 degrees at random. each create here makes none when its pool is
 * null or no particle is free */
void hit_create(NJS_POINT3 *pos, Float scale);
/* a flash as params give it, kept, not copied, for the flash's life. it
 * starts at its first frame. params->scale and growth set its size itself,
 * not in DUST_SIZEs. it plays rate frames a step from frame 6 too, where a
 * puff plays twice rate */
void hit_create_from(struct dust_params *params);
/* lightning from lightning_pool at pos, drawn at size * 0.15, that plays its
 * eight frames once, one a step */
void lightning_create(NJS_POINT3 *pos, Float size);

#endif
