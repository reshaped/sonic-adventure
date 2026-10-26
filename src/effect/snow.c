/* snow: puffs that grow, slow and rise, from a pool of their own */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "effect/snow.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

#define SNOW_FRAMES 8

struct snow_vars {
    struct dust_params *params;
};

#define SNOW(particle) ((struct snow_vars *)&(particle)->vars)

static void snow_step(struct particle *particle, struct particle_pool *pool);

/* EFF_REGULAR's snowp01 to 08 */
NJS_TEXLIST snow_texlist = {
#include "effect/snow/snow_texlist.inc"
};
static NJS_TEXANIM snow_frames[SNOW_FRAMES] = {
#include "effect/snow/snow_frames.inc"
};
static struct particle_anim snow_anim = {
    &snow_texlist, snow_frames, SNOW_FRAMES, NJD_COLOR_BLENDING_SRCALPHA,
    NJD_COLOR_BLENDING_ONE};

/* snow_create's params */
static struct dust_params snow_defaults = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

struct particle_pool_spec snow_pool = {&snow_anim, 0, NULL, NULL};

static void snow_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = SNOW(particle)->params;
    Float drag;

    particle->frame = particle->time;
    if (particle->frame >= SNOW_FRAMES) {
        particle->frame = SNOW_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    particle->time += params->rate;
    drag = params->drag;
    /* @hack spelling: the drag's product negated and added, FNEG and FADD at
     * 0x8c0a3e1c and 0x8c0a3e1e; with -=, SHC subtracts, the function runs 6
     * bytes short and 62 of 84 words differ */
    particle->speed.x += -particle->speed.x * drag;
    particle->speed.y += (-particle->speed.y * drag) + params->lift;
    particle->speed.z += -particle->speed.z * drag;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale += params->growth;
}

void snow_create(NJS_POINT3 *pos, const NJS_VECTOR *speed, Float scale)
{
    struct particle_pool *pool = snow_pool.pool;
    struct particle *particle = particle_alloc(pool, snow_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale;
        particle->time = rand() * RAND_SCALE * 4.0F;
        particle->ang =
            NJM_DEG_ANG(45) - (Angle)(rand() * RAND_SCALE * NJM_DEG_ANG(22.5));
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        SNOW(particle)->params = &snow_defaults;
    }
}

void snow_create_from(struct dust_params *params)
{
    struct particle_pool *pool = snow_pool.pool;
    struct particle *particle = particle_alloc(pool, snow_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale;
        particle->time = rand() * RAND_SCALE * 4.0F;
        particle->ang =
            NJM_DEG_ANG(45) - (Angle)(rand() * RAND_SCALE * NJM_DEG_ANG(22.5));
        particle->color = params->color;
        SNOW(particle)->params = params;
    }
}
