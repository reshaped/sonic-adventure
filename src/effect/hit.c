/* hits: flashes that drift and grow, and lightning that plays its frames
 * once, from two pools */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/hit.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* the frames each plays, and lightning's size at a size of 1 */
#define HIT_FRAMES 8
#define LIGHTNING_FRAMES 8
#define LIGHTNING_SIZE 0.15F

/* EFF_REGULAR's hit_l01 to 08 */
NJS_TEXLIST hit_texlist = {
#include "effect/hit/hit_texlist.inc"
};
static NJS_TEXANIM hit_frames[HIT_FRAMES] = {
#include "effect/hit/hit_frames.inc"
};
static struct particle_anim hit_anim = {&hit_texlist, hit_frames, HIT_FRAMES,
                                        NJD_COLOR_BLENDING_SRCALPHA,
                                        NJD_COLOR_BLENDING_ONE};

/* hit_create's params */
static struct dust_params hit_defaults = {
    1.0F,
    0.038F,
    0.19F,
    0.0F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

struct particle_pool_spec hit_pool = {&hit_anim, 0, NULL, NULL};

/* EFF_REGULAR's mlightning01 to 08, its frames flipped top to bottom */
NJS_TEXLIST lightning_texlist = {
#include "effect/hit/lightning_texlist.inc"
};
NJS_TEXANIM lightning_frames[LIGHTNING_FRAMES] = {
#include "effect/hit/lightning_frames.inc"
};
static struct particle_anim lightning_anim = {
    &lightning_texlist, lightning_frames, LIGHTNING_FRAMES,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec lightning_pool = {&lightning_anim, 0, NULL, NULL};

static void hit_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;
    Float *walk;
    char *position;
    Float x;
    Float y;
    Float z;
    Float px;
    Float py;
    Float pz;

    particle->frame = particle->time;
    if (particle->frame >= HIT_FRAMES) {
        particle->frame = HIT_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    particle->time += params->rate;
    /* @hack spelling: speed out and back through a Float cursor stepped
     * across its members, which loads with @R5+ and stores with @-R5
     * (0x8c0a405e, 0x8c0a406e); as particle->speed.x, .y and .z, the function
     * runs 8 bytes long and 41 of 61 words differ */
    walk = &particle->speed.x;
    x = *walk++;
    y = *walk++;
    z = *walk++;
    x *= 1.0F - params->drag;
    y *= 1.0F - params->drag;
    z *= 1.0F - params->drag;
    y += params->lift;
    *--walk = z;
    *--walk = y;
    *--walk = x;
    /* @hack spelling: pos out and back through a byte cursor, the same way;
     * through the members, SHC indexes them through R0 and 28 of 61 words
     * differ */
    position = (char *)&particle->pos;
    px = *(Float *)position;
    position += sizeof(Float);
    py = *(Float *)position;
    position += sizeof(Float);
    pz = *(Float *)position;
    position += sizeof(Float);
    px += x;
    py += y;
    pz += z;
    position -= sizeof(Float);
    *(Float *)position = pz;
    position -= sizeof(Float);
    *(Float *)position = py;
    position -= sizeof(Float);
    *(Float *)position = px;
    particle->scale += params->growth;
}

void hit_create(NJS_POINT3 *pos, Float scale)
{
    char *walk;
    Float x;
    Float y;
    Float z;
    struct particle_pool *pool = hit_pool.pool;
    struct particle *particle = particle_alloc(pool, hit_step);

    if (particle != NULL) {
        /* @hack spelling: each store through walk, a byte cursor stepped down
         * from past pos, FMOV.S FRm,@-R4; through the members, each takes
         * MOV #n,R0 and @(R0,R14), the function runs 4 bytes long and 57 of
         * 71 words differ */
        walk = (char *)&particle->pos + sizeof(NJS_POINT3);
        /* @hack redundant: the unset x, y and z stored and then zeroed,
         * which stores FR15, as the caller left it, to the flash's z, y and x
         * (0x8c0a40e0, 0x8c0a40e6, 0x8c0a40ea); without it, the function runs
         * 12 bytes short and 68 of 71 words differ */
        /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
        walk -= sizeof(Float);
        *(Float *)walk = z;
        walk -= sizeof(Float);
        *(Float *)walk = y;
        walk -= sizeof(Float);
        *(Float *)walk = x;
        /* NOLINTEND(clang-diagnostic-uninitialized) */
        x = y = z = 0.0F;
        walk += 3 * sizeof(Float);
        walk -= sizeof(Float);
        *(Float *)walk = z;
        walk -= sizeof(Float);
        *(Float *)walk = y;
        walk -= sizeof(Float);
        *(Float *)walk = x;
        particle->scale = scale;
        particle->time = rand() * RAND_SCALE * 4.0F;
        particle->ang =
            NJM_DEG_ANG(45) - (Angle)(rand() * RAND_SCALE * NJM_DEG_ANG(22.5));
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        DUST(particle)->params = &hit_defaults;
    }
}

void hit_create_from(struct dust_params *params)
{
    struct particle_pool *pool = hit_pool.pool;
    struct particle *particle = particle_alloc(pool, hit_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale;
        particle->time = 0.0F;
        particle->ang =
            NJM_DEG_ANG(45) - (Angle)(rand() * RAND_SCALE * NJM_DEG_ANG(22.5));
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}

static void lightning_step(struct particle *particle,
                           struct particle_pool *pool)
{
    particle->frame = particle->time;
    if (particle->frame >= LIGHTNING_FRAMES) {
        particle->frame = LIGHTNING_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    particle->time += 1.0F;
}

void lightning_create(NJS_POINT3 *pos, Float size)
{
    struct particle_pool *pool = lightning_pool.pool;
    struct particle *particle = particle_alloc(pool, lightning_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->scale = size * LIGHTNING_SIZE;
        particle->time = 0.0F;
        particle->ang = 0;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
    }
}
