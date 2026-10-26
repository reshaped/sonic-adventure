/* panlights: glows that move as dust does, fading and shrinking as they go */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "effect/panlight.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* a glow's size at a scale of 1 */
#define PANLIGHT_SIZE 0.15F

/* EFF_REGULAR's panlight */
NJS_TEXLIST panlight_texlist = {
#include "effect/panlight/panlight_texlist.inc"
};
static NJS_TEXANIM panlight_frame = {
#include "effect/panlight/panlight_frame.inc"
};
static struct particle_anim panlight_anim = {&panlight_texlist, &panlight_frame,
                                             1, NJD_COLOR_BLENDING_SRCALPHA,
                                             NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec panlight_pool = {&panlight_anim, 0, NULL, NULL};

static void panlight_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;
    Float keep;

    particle->color.a -= params->rate * 0.1F;
    if (particle->color.a < 0.0F) {
        particle_kill(particle);
        return;
    }
    keep = 1.0F - params->drag;
    /* @hack spelling: the products and sums spelled out, which SHC works in
     * registers (FNEG at 0x8c04878e); with *= and +=, it takes each field's
     * address into a register first, the function runs 6 bytes long and 47
     * of 72 words differ */
    particle->speed.x = particle->speed.x * keep;
    particle->speed.y = (particle->speed.y * keep) + params->lift;
    particle->speed.z = particle->speed.z * keep;
    particle->pos.x = particle->pos.x + particle->speed.x;
    particle->pos.y = particle->pos.y + particle->speed.y;
    particle->pos.z = particle->pos.z + particle->speed.z;
    particle->scale = particle->scale - (particle->scale * params->growth *
                                         (0.1F * PANLIGHT_SIZE));
}

void panlight_create(struct dust_params *params)
{
    struct particle_pool *pool = panlight_pool.pool;
    struct particle *particle = particle_alloc(pool, panlight_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale * PANLIGHT_SIZE;
        particle->frame = 0;
        particle->time = 0.0F;
        particle->ang = RANDOM_ANGLE();
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}
