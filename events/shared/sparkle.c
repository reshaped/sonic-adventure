/* an event's sparkles: particles in the sparkle pool that drift, spin and
 * shrink, then end */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/particle.h"
#include "effect/pools.h"
#include "effect/sparkle.h"
#include "shared/sparkle.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* a sparkle's turn each step is random, up to half of this either way */
#define SPARKLE_SPIN NJM_DEG_ANG(5)

/* the frames of the pool's animation a sparkle plays each step */
#define SPARKLE_RATE 0.01F

/* the factors of a sparkle's speed each step: sideways, then up and down */
#define SPARKLE_SPREAD 1.02F
#define SPARKLE_SETTLE 0.98F

/* the share of its scale event_sparkle_create's sparkle keeps each step */
#define SPARKLE_SHRINK (59.0F / 60.0F)

/* a sparkle's size at a scale of 1 */
#define SPARKLE_SIZE 1.0F

struct sparkle_vars {
    Angle spin; /* the turn each step */
    Float shrink; /* the share of its scale it keeps each step */
};

#define SPARKLE(particle) ((struct sparkle_vars *)&(particle)->vars)

/* EFF_REGULAR's kiran01 to 04, a copy of the main program's
 * sparkle_texlist */
static NJS_TEXLIST event_sparkle_texlist = {
#include "shared/sparkle/event_sparkle_texlist.inc"
};
/* a copy of the main program's sparkle_frames */
static NJS_TEXANIM event_sparkle_frames[4] = {
#include "shared/sparkle/event_sparkle_frames.inc"
};
/* a copy of the main program's sparkle_anim, which nothing reads: the
 * sparkles draw with the sparkle pool's anim */
static struct particle_anim event_sparkle_anim = {
    &event_sparkle_texlist, event_sparkle_frames, 4,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_ONE};

static void event_sparkle_step(struct particle *particle,
                               struct particle_pool *pool)
{
    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += SPARKLE_RATE;
    particle->speed.x *= SPARKLE_SPREAD;
    particle->speed.y *= SPARKLE_SETTLE;
    particle->speed.z *= SPARKLE_SPREAD;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->ang += SPARKLE(particle)->spin;
    particle->scale *= SPARKLE_SHRINK;
}

void event_sparkle_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale)
{
    struct particle_pool *pool = sparkle_pool.pool;
    struct particle *particle = particle_alloc(pool, event_sparkle_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale * SPARKLE_SIZE;
        particle->time = 0.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        SPARKLE(particle)->spin = (SPARKLE_SPIN * (Float)rand() * RAND_SCALE) -
                                  (Angle)(SPARKLE_SPIN / 2);
    }
}

static void event_sparkle_step_shrinking(struct particle *particle,
                                         struct particle_pool *pool)
{
    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += SPARKLE_RATE;
    particle->speed.x *= SPARKLE_SPREAD;
    particle->speed.y *= SPARKLE_SETTLE;
    particle->speed.z *= SPARKLE_SPREAD;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->ang += SPARKLE(particle)->spin;
    particle->scale *= SPARKLE(particle)->shrink;
}

void event_sparkle_create_shrinking(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                    Float scale, Float shrink)
{
    struct particle_pool *pool = sparkle_pool.pool;
    struct particle *particle =
        particle_alloc(pool, event_sparkle_step_shrinking);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale * SPARKLE_SIZE;
        particle->time = 0.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        SPARKLE(particle)->spin = (SPARKLE_SPIN * (Float)rand() * RAND_SCALE) -
                                  (Angle)(SPARKLE_SPIN / 2);
        SPARKLE(particle)->shrink = shrink;
    }
}
