/* E-102's smoke: puffs that grow and rise, from a pool of their own */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "player/e102_smoke.h"
#include "player/textures.h"
#include "system/util.h"

struct smoke_vars {
    struct dust_params *params;
};

#define SMOKE(particle) ((struct smoke_vars *)&(particle)->vars)

/* E102EFFECT's pom01 to pom08, a frame each */
static NJS_TEXANIM smoke_frames[8] = {
#include "player/e102_smoke/smoke_frames.inc"
};
static struct particle_anim smoke_anim = {&e102effect_texlist, smoke_frames, 8,
                                          NJD_COLOR_BLENDING_SRCALPHA,
                                          NJD_COLOR_BLENDING_ONE};

/* how every puff grows and moves */
static struct dust_params smoke_params = {
    1.0F,
    0.015F,
    0.25F,
    0.06F,
    0.01F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static struct particle_pool_spec smoke_pool = {&smoke_anim, 0, NULL, NULL};

static void smoke_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = SMOKE(particle)->params;
    Float drag;

    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += params->rate;
    drag = params->drag;
    /* @hack spelling: the drag's products negated and added, as FNEG and
     * FADD at 0x8c0dfc04 and 0x8c0dfc06 for x, and likewise for y and z.
     * with -=, SHC subtracts, the function runs 6 bytes short, and 62 of 88
     * words differ */
    particle->speed.x += -particle->speed.x * drag;
    particle->speed.y += (-particle->speed.y * drag) + params->lift;
    particle->speed.z += -particle->speed.z * drag;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale += params->growth;
}

void e102_smoke_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale)
{
    struct particle_pool *pool = smoke_pool.pool;
    struct particle *particle = particle_alloc(pool, smoke_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale;
        particle->time = rand() * RAND_SCALE * 4.0F;
        particle->ang = 0x2000 - (int)(rand() * RAND_SCALE * 4096.0F);
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        SMOKE(particle)->params = &smoke_params;
    }
}

void e102_smoke_init(void)
{
    smoke_pool.pool = particle_pool_create(smoke_pool.anim, 0);
}
