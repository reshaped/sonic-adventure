/* the Egg Carrier's puffs: snow sprites that grow, slow and rise. from
 * FADE_FRAME on, a puff plays at half its rate and fades */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv01obj/puff.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "effect/snow.h"
#include "system/util.h"

/* the frames of a puff's animation. the puff ends once its time passes the
 * last */
#define PUFF_FRAMES 8

/* the frame a puff starts to fade at. from there, each step takes
 * params->rate times FADE_ALPHA off its alpha */
#define FADE_FRAME 6.0F
#define FADE_ALPHA 0.3F

static void puff_step(struct particle *particle, struct particle_pool *pool);

static NJS_TEXANIM puff_frames[PUFF_FRAMES] = {
#include "adv01obj/puff/puff_frames.inc"
};
static struct particle_anim puff_anim = {
    &snow_texlist, puff_frames, PUFF_FRAMES, NJD_COLOR_BLENDING_SRCALPHA,
    NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec egg_carrier_puff_pool = {&puff_anim, 0, NULL, NULL};

static void puff_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;
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
    if (particle->time < FADE_FRAME) {
        particle->time += params->rate;
    } else {
        particle->time += params->rate * 0.5F;
        particle->color.a -= params->rate * FADE_ALPHA;
    }
    /* @hack spelling: the speed out and back through a Float cursor, which
     * loads with @R5+ and stores with @-R5 (0x0c90006a, 0x0c900086). through
     * the fields, SHC indexes them through R0, the function is 8 bytes longer,
     * and 70 of 90 words differ */
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
    /* @hack spelling: the position out and back through the same cursor, which
     * loads with @R5+ and stores with @-R5 (0x0c900090, 0x0c90009c). through
     * the fields, the function is 6 bytes longer and 38 of 90 words differ */
    /* @hack spelling: each step of walk a statement of its own, which keeps
     * params in R6 and walk in R5 (MOV.L @(R0,R4),R6 at 0x0c900002). with
     * *walk++ and *--walk, the two trade registers, and 26 of 90 words
     * differ */
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
void egg_carrier_puff_create(struct dust_params *params)
{
    struct particle_pool *pool = egg_carrier_puff_pool.pool;
    struct particle *particle = particle_alloc(pool, puff_step);

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
