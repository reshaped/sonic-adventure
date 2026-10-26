/* Casinopolis water smoke particles, with a pool of their own */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "stg09/water_smoke.h"
#include "system/util.h"

/* watersmoke01 to watersmoke08 */
extern NJS_TEXLIST water_smoke_texlist;

#define WATER_SMOKE_FRAMES 8

/* the turn each step. a puff with an odd angle turns one way, and one with
 * an even angle the other. the turn is even, so each puff keeps its way */
#define WATER_SMOKE_TURN 48

struct water_smoke_vars {
    struct dust_params *params; /* what the puff moves by, read each step */
    /* the alpha added each step as it fades in, and taken as it fades out */
    Float fade;
};

#define SMOKE(particle) ((struct water_smoke_vars *)&(particle)->vars)

static NJS_TEXANIM water_smoke_frames[WATER_SMOKE_FRAMES] = {
#include "stg09/water_smoke/water_smoke_frames.inc"
};
static struct particle_anim water_smoke_anim = {
    &water_smoke_texlist, water_smoke_frames, WATER_SMOKE_FRAMES,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_ONE};
static struct dust_params water_smoke_defaults = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

struct particle_pool_spec water_smoke_pool = {&water_smoke_anim, 0, NULL, NULL};

static void water_smoke_step(struct particle *particle,
                             struct particle_pool *pool)
{
    struct dust_params *params = SMOKE(particle)->params;
    Float drag;

    particle->frame = particle->time;
    if (particle->frame >= WATER_SMOKE_FRAMES) {
        particle->color.a -= SMOKE(particle)->fade;
        if (particle->color.a < 0.0F) {
            particle->color.a = 0.0F;
        }
        particle->frame = WATER_SMOKE_FRAMES - 1;
        if (particle->color.a < 0.05F) {
            particle->frame = WATER_SMOKE_FRAMES - 1;
            particle_kill(particle);
            return;
        }
    } else if (particle->color.a < params->color.a) {
        particle->color.a += SMOKE(particle)->fade;
        if (particle->color.a > params->color.a) {
            particle->color.a = params->color.a;
        }
    } else {
        particle->scale += params->growth;
        particle->time += params->rate;
    }
    drag = params->drag;
    particle->speed.x += -particle->speed.x * drag;
    particle->speed.y += (-particle->speed.y * drag) + params->lift;
    particle->speed.z += -particle->speed.z * drag;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    if (particle->ang & 1) {
        particle->ang += WATER_SMOKE_TURN;
    } else {
        particle->ang -= WATER_SMOKE_TURN;
    }
}

void water_smoke_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale)
{
    struct particle_pool *pool = water_smoke_pool.pool;
    struct particle *particle = particle_alloc(pool, water_smoke_step);

    if (particle) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale;
        particle->time = rand() * (2.0F * RAND_SCALE);
        particle->ang = NJM_DEG_ANG(45) - (int)(rand() * 0.125F);
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        SMOKE(particle)->params = &water_smoke_defaults;
        SMOKE(particle)->fade = 0.02F;
    }
}

void water_smoke_create_from(struct dust_params *params)
{
    struct particle_pool *pool = water_smoke_pool.pool;
    struct particle *particle = particle_alloc(pool, water_smoke_step);

    if (particle) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale;
        particle->time = rand() * (2.0F * RAND_SCALE);
        particle->ang = NJM_DEG_ANG(45) - (int)(rand() * 0.125F);
        particle->color = params->color;
        particle->color.a = 0.05F;
        SMOKE(particle)->params = params;
        SMOKE(particle)->fade = 0.05F;
    }
}
