/* smoke puff particles shared by Final Egg and Hot Shelter objects */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "effect/snow.h"
#include "stg10/puff.h"
#include "system/util.h"

/* the frames of a puff's animation. the puff ends once its time passes the
 * last */
#define PUFF_FRAMES 8

static void final_puff_step(struct particle *particle,
                            struct particle_pool *pool);

static NJS_TEXANIM final_puff_frames[PUFF_FRAMES] = {
#include "stg10/puff/final_puff_frames.inc"
};
static struct particle_anim final_puff_anim = {
    &snow_texlist, final_puff_frames, PUFF_FRAMES, NJD_COLOR_BLENDING_SRCALPHA,
    NJD_COLOR_BLENDING_ONE};

/* no code reads these params */
static struct dust_params final_puff_params = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

struct particle_pool_spec final_puff_pool = {&final_puff_anim, 0, NULL, NULL};

static void final_puff_step(struct particle *particle,
                            struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;
    Float kept;
    Float *walk;
    Float x;
    Float y;
    Float z;
    Float px;
    Float py;
    Float pz;

    particle->frame = particle->time;
    if (particle->frame >= PUFF_FRAMES) {
        particle->frame = PUFF_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    particle->time += params->rate;
    /* @hack spelling: the speed out and back through a Float cursor, which
     * loads with @R5+ and stores with @-R5 (0x0c90fa5e, 0x0c90fa6e). through
     * the fields, SHC indexes them through R0, the function is 8 bytes longer,
     * and 44 of 64 words differ */
    walk = &particle->speed.x;
    kept = 1.0F - params->drag;
    x = *walk++;
    y = *walk++;
    x *= kept;
    y *= kept;
    z = *walk++;
    z *= kept;
    y += params->lift;
    *--walk = z;
    *--walk = y;
    *--walk = x;
    /* @hack spelling: the position out and back through the same cursor, which
     * loads with @R5+ and stores with @-R5 (0x0c90fa78, 0x0c90fa90). through
     * the fields, the function is 8 bytes longer and 32 of 64 words differ */
    /* @hack spelling: each step of walk a statement of its own, which loads the
     * x into FR7 (FMOV.S @R5+,FR7 at 0x0c90fa78). with *walk++ and *--walk,
     * FR5 and FR7 trade, and 10 of 64 words differ */
    walk = &particle->pos.x;
    px = *walk;
    walk++;
    py = *walk;
    walk++;
    pz = *walk;
    walk++;
    px += x;
    py += y;
    pz += z;
    walk--;
    *walk = pz;
    walk--;
    *walk = py;
    walk--;
    *walk = px;
    particle->scale += params->growth * DUST_SIZE;
}

/* a puff starts at a random time in its first four frames, turned by 22.5
 * to 45 degrees */
void final_puff_create(struct dust_params *params)
{
    struct particle_pool *pool = final_puff_pool.pool;
    struct particle *particle = particle_alloc(pool, final_puff_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale * DUST_SIZE;
        particle->time = rand() * RAND_SCALE * 4.0F;
        particle->ang =
            NJM_DEG_ANG(45) - (int)(rand() * RAND_SCALE * NJM_DEG_ANG(22.5));
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}
