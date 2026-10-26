/* bombs: a puff that plays bomb01 to 16 once */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */

/* a puff's size at a scale of 1 */
#define BOMB_SIZE 0.5F
/* bomb01 to bomb16, a frame each */
#define BOMB_FRAMES 16

struct bomb_vars {
    struct dust_params *params;
};

#define BOMB(particle) ((struct bomb_vars *)&(particle)->vars)

/* EFF_REGULAR's bomb01 to 16 */
NJS_TEXLIST bomb_texlist = {
#include "effect/bomb/bomb_texlist.inc"
};
static NJS_TEXANIM bomb_frames[BOMB_FRAMES] = {
#include "effect/bomb/bomb_frames.inc"
};
static struct particle_anim bomb_anim = {
    &bomb_texlist, bomb_frames, BOMB_FRAMES, NJD_COLOR_BLENDING_SRCALPHA,
    NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec bomb_pool = {&bomb_anim, 0, NULL, NULL};

/* bomb_create's params: a white puff that plays at 0.4 frames a step */
static struct dust_params bomb_defaults = {
    1.0F,
    0.0F,
    0.4F,
    0.0F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* a puff's step. it plays its frames at the params' rate, but its last at
 * twice the rate, and ends after it */
static void bomb_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = BOMB(particle)->params;

    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    if (particle->time < BOMB_FRAMES - 1) {
        particle->time += params->rate;
    } else {
        particle->time += params->rate * 2.0F;
    }
}

void bomb_create(NJS_POINT3 *pos, Float scale)
{
    struct particle_pool *pool = bomb_pool.pool;
    struct particle *particle = particle_alloc(pool, bomb_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->scale = scale * BOMB_SIZE;
        particle->time = 0.0F;
        particle->ang = 0;
        particle->color = bomb_defaults.color;
        BOMB(particle)->params = &bomb_defaults;
    }
}
