/* O FIRESMOKE, smoke over a fire. every PUFF_STEPS steps it asks
 * past_smoke_pool for a puff, which grows and pales from red, then fades.
 * nothing makes the pool, so no puff appears */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv03/fire_smoke.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/util.h"

/* the past's smoke pool, which past_pools lists. 1ST_READ.BIN's stage_pools
 * row for the past is null, so nothing makes the pool. its pool stays null,
 * and particle_alloc gives no puff */
extern struct particle_pool_spec past_smoke_pool;

/* the steps between puffs */
#define PUFF_STEPS 24

/* a puff's age counts its steps of growth from 0. the step that finds it at
 * PUFF_GROWN still grows the puff, then turns it to fading */
#define PUFF_GROWN 50

enum fire_smoke_mode { FIRE_SMOKE_INIT, FIRE_SMOKE_RUN };

/* a puff's life: it grows, then fades */
enum puff_mode { PUFF_GROW, PUFF_FADE };

struct puff_vars {
    /* @hack type: age as an array of one, which has SHC form its address in
     * a register for each access (ADD #68,R3 at 0x0c902f6a, ADD #68,R0 at
     * 0x0c902fb4); as one Sint16, SHC reaches it through @(R0,Rn), and,
     * summed over the two functions that use it, 73 of 230 words differ */
    Sint16 age[1];
};

#define PUFF(particle) ((struct puff_vars *)&(particle)->vars)

static void fire_smoke_puff_step(struct particle *particle,
                                 struct particle_pool *pool);

void fire_smoke_step(struct task *task)
{
    struct task_work *work = task->work;
    struct particle *puff;
    struct particle_pool *pool;
    Float x;
    Float z;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case FIRE_SMOKE_INIT:
        work->mode = FIRE_SMOKE_RUN;
        break;
    case FIRE_SMOKE_RUN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) == PUFF_STEPS) {
            /* @hack spelling: the pool read into a local in a statement of its
             * own, which loads the step's address ahead of the pool's (MOV.L
             * at 0x0c902ebe and 0x0c902ec0); passed directly, the pool's loads
             * first, FR13 and FR15 trade, and 11 of 145 words differ */
            pool = past_smoke_pool.pool;
            puff = particle_alloc(pool, fire_smoke_puff_step);
            if (puff != NULL) {
                puff->life = PUFF_GROW;
                x = ((rand() * RAND_SCALE) - 0.5F) * 15.0F;
                z = ((rand() * RAND_SCALE) - 0.5F) * 15.0F;
                puff->pos.x = work->pos.x + x;
                puff->pos.y = work->pos.y;
                puff->pos.z = work->pos.z + z;
                puff->speed.x = x;
                puff->speed.y = 0.6F;
                puff->speed.z = z;
                puff->frame = (int)(rand() * (1.0F / 8192.0F));
                puff->ang = (int)(((rand() * RAND_SCALE) - 0.5F) * 65536.0F);
                puff->scale = 1.0F + (rand() * (0.5F * RAND_SCALE));
                PUFF(puff)->age[0] = 0;
                puff->color.a = 0.8F;
                puff->color.r = 1.0F;
                puff->color.g = 0.1F;
                puff->color.b = 0.1F;
            }
            TASK_TIMER(work) = 0;
        }
        break;
    }
}

static void fire_smoke_puff_step(struct particle *particle,
                                 struct particle_pool *pool)
{
    switch (particle->life) {
    case PUFF_GROW:
        if (PUFF(particle)->age[0]++ == PUFF_GROWN) {
            particle->life = PUFF_FADE;
        }
        if (particle->color.r > 0.3F) {
            particle->color.r -= 0.01F;
        }
        particle->scale += 0.02F;
        break;
    case PUFF_FADE:
        particle->color.a -= 0.04F;
        if (particle->color.a < 0.0F) {
            particle_kill(particle);
            return;
        }
        particle->scale += 0.04F;
        break;
    }
    if (particle->ang > 0) {
        particle->ang += 64;
    } else {
        particle->ang -= 64;
    }
    particle->speed.y += 0.05F;
    particle->speed.y *= 0.97F;
    particle->pos.y += particle->speed.y;
}
