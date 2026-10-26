/* sand: the puffs a board throws up on the sand hill, from a pool of their
 * own */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "sboard/sand.h"
#include "sboard/sand_textures.h"
#include "system/util.h"

/* a puff's frames, snabop01 to 08. a puff goes once its frame passes the
 * last */
#define SAND_FRAMES 8

/* a puff's vars: the params sand_step reads */
struct sand_vars {
    struct dust_params *params;
};

#define SAND(particle) ((struct sand_vars *)&(particle)->vars)

static void sand_step(struct particle *particle, struct particle_pool *pool);

static const NJS_TEXANIM sand_frames[SAND_FRAMES] = {
#include "sboard/sand/sand_frames.inc"
};
static const struct particle_anim sand_anim = {
    &eff_sandboard_texlist, (NJS_TEXANIM *)sand_frames, SAND_FRAMES,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_INVSRCALPHA};

struct particle_pool_spec sand_pool = {(struct particle_anim *)&sand_anim, 0,
                                       NULL, NULL};

/* in HEAD, which puts it at 0x0c900080, where 1ST_READ.BIN's
 * player_board_spray calls it */
#pragma section HEAD
void sand_create_from(struct dust_params *params)
{
    struct particle *particle = particle_alloc(sand_pool.pool, sand_step);

    if (particle != NULL) {
        particle->ang = (Angle)(rand() * (NJM_DEG_ANG(22.5) * RAND_SCALE)) +
                        NJM_DEG_ANG(22.5);
        particle->time = rand() * (4.0F * RAND_SCALE);
        particle->scale = params->scale;
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->color = params->color;
        SAND(particle)->params = params;
    }
}
#pragma section

static void sand_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params;
    Float keep;
    int last = SAND_FRAMES - 1;
    NJS_VECTOR *speed;
    NJS_POINT3 *pos;

    particle->frame = particle->time;
    if (particle->frame > last) {
        particle->frame = last;
        particle_kill(particle);
        return;
    }
    params = SAND(particle)->params;
    speed = &particle->speed;
    pos = &particle->pos;
    keep = 1.0F - params->drag;
    particle->time += params->rate;
    particle->scale += params->growth;
    speed->x *= keep;
    speed->y = (speed->y * keep) + params->lift;
    speed->z *= keep;
    pos->x += speed->x;
    pos->y += speed->y;
    pos->z += speed->z;
}
