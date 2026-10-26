/* flames: sprites that play flame01 to 10 once as they rise */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/flame.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* a flame's frames, and its size at a scale of 1 */
#define FLAME_FRAMES 10
#define FLAME_SIZE 0.15F

struct flame_vars {
    struct dust_params *params;
};

#define FLAME(particle) ((struct flame_vars *)&(particle)->vars)

/* EFF_REGULAR's flame01 to 10 */
NJS_TEXLIST flame_texlist = {
#include "effect/flame/flame_texlist.inc"
};
static NJS_TEXANIM flame_frames[FLAME_FRAMES] = {
#include "effect/flame/flame_frames.inc"
};
static struct particle_anim flame_anim = {
    &flame_texlist, flame_frames, FLAME_FRAMES, NJD_COLOR_BLENDING_SRCALPHA,
    NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec flame_pool = {&flame_anim, 0, NULL, NULL};

/* flame_create's params */
static struct dust_params flame_defaults = {
    1.0F,
    0.038F,
    0.19F,
    0.065F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static void flame_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = FLAME(particle)->params;
    Float drag;

    particle->frame = particle->time;
    if (particle->frame > FLAME_FRAMES - 1) {
        particle->frame = FLAME_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    particle->time += params->rate;
    drag = params->drag;
    /* @hack spelling: the drag's product negated and added, FNEG and FADD at
     * 0x8c04807c and 0x8c04807e; with -=, SHC subtracts, the function runs 6
     * bytes short and 65 of 87 words differ */
    particle->speed.x += -particle->speed.x * drag;
    particle->speed.y += (-particle->speed.y * drag) + params->lift;
    particle->speed.z += -particle->speed.z * drag;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale += params->growth * FLAME_SIZE;
}

void flame_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale)
{
    struct particle_pool *pool = flame_pool.pool;
    struct particle *particle = particle_alloc(pool, flame_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale * FLAME_SIZE;
        particle->time = rand() * RAND_SCALE * 5.0F;
        particle->ang = RANDOM(100.0F, 260.0F) * ANGLE_PER_DEGREE;
        particle->color = flame_defaults.color;
        FLAME(particle)->params = &flame_defaults;
    }
}

void flame_create_from(struct dust_params *params)
{
    struct particle_pool *pool = flame_pool.pool;
    struct particle *particle = particle_alloc(pool, flame_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale * FLAME_SIZE;
        particle->time = rand() * RAND_SCALE * 5.0F;
        particle->ang = RANDOM(100.0F, 260.0F) * ANGLE_PER_DEGREE;
        particle->color = params->color;
        FLAME(particle)->params = params;
    }
}
