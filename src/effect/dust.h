/* dust: puffs from two pools, one plain and one that glows */
#ifndef EFFECT_DUST_H
#define EFFECT_DUST_H
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/particle.h"
#include "effect/pools.h"

/* the params dust_step reads, which other effects' creates take too. a
 * create given params keeps them, not a copy, for its effect's life. the
 * comments below give a dust puff's reading, and each other create says
 * where its reading differs */
struct dust_params {
    Float scale; /* a puff's starting size, in DUST_SIZEs */
    Float growth; /* added to a puff's size each step, in DUST_SIZEs */
    /* the animation frames a puff plays a step. from frame 6, it plays twice
     * rate a step */
    Float rate;
    Float drag; /* the share of its speed it loses each step */
    Float lift; /* added to its upward speed each step */
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_ARGB color;
};

/* a puff's size at a scale of 1 */
#define DUST_SIZE 0.15F

struct dust_vars {
    struct dust_params *params;
};

#define DUST(particle) ((struct dust_vars *)&(particle)->vars)

/* dust01 to dust08, then dust08 twice more, and their ten frames */
extern NJS_TEXLIST dust_texlist;
extern NJS_TEXANIM dust_frames[];
extern struct particle_pool_spec dust_pool;
extern struct particle_pool_spec dust_glow_pool;

/* a puff's step that ignores params. the puff rises and drifts in z as it
 * grows. it plays 0.19 frames a step, and 0.57 from frame 6 */
void dust_rise_step(struct particle *particle, struct particle_pool *pool);
/* a puff's step, as DUST(particle)->params say. a caller that takes the
 * particle itself sets them */
void dust_step(struct particle *particle, struct particle_pool *pool);
/* a white puff from dust_pool at pos, moving at speed, at scale times
 * DUST_SIZE. it steps as dust_step does, with params of its own. it,
 * dust_create_from and dust_create_glow start a puff at a random frame below
 * 6, turned at random. each makes none when its pool is null or no particle
 * is free */
void dust_create(NJS_POINT3 *pos, const NJS_VECTOR *speed, Float scale);
/* a puff from dust_pool as params give it */
void dust_create_from(struct dust_params *params);
/* a puff from dust_glow_pool, which adds its light, as params give it */
void dust_create_glow(struct dust_params *params);
/* a puff's step that ignores params. the puff grows as it plays its ten
 * frames, 9.5 of them every 60 steps */
void dust_grow_step(struct particle *particle, struct particle_pool *pool);

#endif
