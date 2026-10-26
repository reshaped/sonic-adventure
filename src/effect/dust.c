/* dust: puffs from two pools, one plain and one that glows */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* EFF_REGULAR's dust01 to 08, then 08 twice more */
NJS_TEXLIST dust_texlist = {
#include "effect/dust/dust_texlist.inc"
};
NJS_TEXANIM dust_frames[10] = {
#include "effect/dust/dust_frames.inc"
};
static struct particle_anim dust_anim = {&dust_texlist, dust_frames, 10,
                                         NJD_COLOR_BLENDING_SRCALPHA,
                                         NJD_COLOR_BLENDING_INVSRCALPHA};
static struct particle_anim dust_glow_anim = {&dust_texlist, dust_frames, 10,
                                              NJD_COLOR_BLENDING_SRCALPHA,
                                              NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec dust_pool = {&dust_anim, 0, NULL, NULL};
struct particle_pool_spec dust_glow_pool = {&dust_glow_anim, 0, NULL, NULL};

/* dust_create's params */
static struct dust_params dust_defaults = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

void dust_rise_step(struct particle *particle, struct particle_pool *pool)
{
    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    if (particle->time < 6.0F) {
        particle->time += 0.19F;
    } else {
        particle->time += 0.57F;
    }
    particle->scale += 0.038F * DUST_SIZE;
    particle->pos.y += 0.3F;
    particle->pos.z += 0.1F;
}

void dust_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;
    Float drag;

    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    if (particle->time < 6.0F) {
        particle->time += params->rate;
    } else {
        particle->time += params->rate * 2.0F;
    }
    drag = params->drag;
    /* @hack spelling: the drag's product negated and added, FNEG and FADD at
     * 0x8c0492ae and 0x8c0492b0; with -=, SHC subtracts, the function runs 6
     * bytes short and 56 of 105 words differ */
    particle->speed.x += -particle->speed.x * drag;
    particle->speed.y += (-particle->speed.y * drag) + params->lift;
    particle->speed.z += -particle->speed.z * drag;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale += params->growth * DUST_SIZE;
}

void dust_create(NJS_POINT3 *pos, const NJS_VECTOR *speed, Float scale)
{
    struct particle_pool *pool = dust_pool.pool;
    struct particle *particle = particle_alloc(pool, dust_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale * DUST_SIZE;
        particle->time = rand() * RAND_SCALE * 6.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        DUST(particle)->params = &dust_defaults;
    }
}

void dust_create_from(struct dust_params *params)
{
    struct particle_pool *pool = dust_pool.pool;
    struct particle *particle = particle_alloc(pool, dust_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale * DUST_SIZE;
        particle->time = rand() * RAND_SCALE * 6.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}

void dust_create_glow(struct dust_params *params)
{
    struct particle_pool *pool = dust_glow_pool.pool;
    struct particle *particle = particle_alloc(pool, dust_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale * DUST_SIZE;
        particle->time = rand() * RAND_SCALE * 6.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}

void dust_grow_step(struct particle *particle, struct particle_pool *pool)
{
    particle->frame = particle->time;
    if (particle->frame > 9) {
        particle->frame = 9;
        particle_kill(particle);
        return;
    }
    particle->time += 9.5F / 60.0F;
    particle->scale += 0.646F * DUST_SIZE;
}
