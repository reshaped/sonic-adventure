/* sparkles: glints that drift and spin, and a glint that comes and goes */
#ifndef EFFECT_SPARKLE_H
#define EFFECT_SPARKLE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/pools.h"

/* kiran01 to kiran04, which the sparkles and glints draw */
extern NJS_TEXLIST sparkle_texlist;
extern struct particle_pool_spec sparkle_pool;

/* a sparkle from sparkle_pool at pos, moving at speed, drawn at scale and
 * turned at random. each step it turns by the same amount, picked at its
 * start from -spin / 2 up to spin / 2. each step its speed grows by 2% in
 * x and z and shrinks by 2% in y, and its size shrinks by a sixtieth. it
 * plays kiran01 to 04 at a hundredth of a frame a step. it makes none when
 * the pool is null or no particle is free */
void sparkle_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale,
                    Angle spin);
/* a glint task at level 3 at pos, drawn at size * 0.1. each step it adds
 * rate to its frame and draws, but it ends instead once its frame reaches
 * 7. its frames show kiran01 up to 04 and back. when no task can be made,
 * it writes through a null task */
void sparkle_create_glint(NJS_POINT3 *pos, Float size, Float rate);

#endif
