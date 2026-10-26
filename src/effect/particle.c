/* particles: sprites from a free list, stepped and drawn in pools */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/sprite.h"
#include "effect/particle.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/timer.h"

#define POOL_PAUSED 0x1
#define POOL_DEAD 0x2

static struct particle *particle_buffer;
static Uint32 particle_capacity;
static struct particle *particle_free_list;
static Uint32 particle_free_count;
static struct particle_pool *particle_pools;
static int particle_pool_count;
static int particle_drawn; /* how many particles the last draw drew */
static NJS_SPRITE particle_sprite;
static Float particle_depth; /* the depth particle_alloc gives */

static NJS_ARGB clear_material = {0.0F, 0.0F, 0.0F, 0.0F};

static void particle_free_list_init(struct particle *buffer, Uint32 count)
{
    Uint32 i;

    particle_free_list = buffer;
    particle_free_count = count;
    for (i = 0; i < count - 1; i++) {
        buffer[i].next = &buffer[i + 1];
    }
    buffer[count - 1].next = NULL;
}

void particle_init(struct particle *buffer, Uint32 count)
{
    particle_buffer = buffer;
    particle_capacity = count;
    particle_free_list_init(particle_buffer, count);
    particle_pools = NULL;
    particle_pool_count = 0;
    particle_depth = 0.0F;
}

struct particle *particle_alloc(struct particle_pool *pool,
                                void (*step)(struct particle *particle,
                                             struct particle_pool *pool))
{
    struct particle *particle;

    if (pool == NULL) {
        return NULL;
    }
    if (particle_free_list == NULL) {
        return NULL;
    }
    particle = particle_free_list;
    particle_free_list = particle->next;
    particle_free_count--;
    particle->next = pool->particles;
    pool->particles = particle;
    pool->count++;
    particle->step = step;
    particle->depth = particle_depth;
    return particle;
}

void particle_kill(struct particle *particle)
{
    particle->step = NULL;
}

struct particle_pool *particle_pool_create(struct particle_anim *anim,
                                           Uint32 size)
{
    struct particle_pool *pool;

    if (anim == NULL) {
        return NULL;
    }
    pool = heap_calloc(1, size + sizeof(*pool));
    if (pool == NULL) {
        return NULL;
    }
    pool->next = particle_pools;
    particle_pools = pool;
    particle_pool_count++;
    if (size > 0) {
        pool->work = pool + 1;
    }
    pool->anim = anim;
    return pool;
}

Bool particle_pool_step(struct particle_pool *pool)
{
    if (pool->flags & POOL_DEAD) {
        return FALSE;
    }
    if (pool->flags & POOL_PAUSED) {
        return FALSE;
    }
    if (pool->step != NULL) {
        pool->step(pool);
    }
    return TRUE;
}

void particle_pool_kill(struct particle_pool *pool)
{
    if (!(pool->flags & POOL_DEAD)) {
        pool->flags |= POOL_DEAD;
    }
}

/* the pool's last particle keeps its step until particle_alloc reuses it */
void particle_pool_release(struct particle_pool *pool)
{
    struct particle *particle;

    if (!(pool->flags & POOL_DEAD)) {
        pool->flags |= POOL_DEAD;
        if (pool->particles != NULL) {
            for (particle = pool->particles; particle->next != NULL;
                 particle = particle->next) {
                particle->step = NULL;
            }
            particle->next = particle_free_list;
            particle_free_list = pool->particles;
            particle_free_count += pool->count;
            pool->particles = NULL;
            pool->count = 0;
        }
    }
}

void particle_pool_pause(struct particle_pool *pool)
{
    pool->flags |= POOL_PAUSED;
}

void particle_pool_resume(struct particle_pool *pool)
{
    if (!(pool->flags & POOL_DEAD)) {
        pool->flags &= ~POOL_PAUSED;
    }
}

void particle_run(void)
{
    struct particle_pool *pool;
    struct particle_pool *next;
    struct particle_pool *prev;
    struct particle *particle;
    struct particle *after;
    struct particle *before;
    Float step_time;
    Float draw_time;

    timer_mark();
    prev = NULL;
    for (pool = particle_pools; pool != NULL; pool = next) {
        next = pool->next;
        if (pool->flags & POOL_DEAD) {
            if (pool->particles == NULL) {
                if (prev == NULL) {
                    particle_pools = next;
                } else {
                    prev->next = next;
                }
                particle_pool_count--;
                heap_free(pool);
                continue;
            } else {
                prev = pool;
            }
        } else {
            prev = pool;
        }
        before = NULL;
        for (particle = pool->particles; particle != NULL; particle = after) {
            after = particle->next;
            if (particle->step != NULL) {
                particle->step(particle, pool);
                before = particle;
            } else {
                if (before == NULL) {
                    pool->particles = after;
                } else {
                    before->next = after;
                }
                pool->count--;
                particle->next = particle_free_list;
                particle_free_list = particle;
                particle_free_count++;
            }
        }
    }
    step_time = timer_lap();
    timer_mark();
    if (frame_step == 0) {
        particle_draw_all();
    }
    draw_time = timer_lap();
    if (debug_page) {
        njPrintC(NJM_LOCATION(30, 10), "SPR TASK");
        njPrintC(NJM_LOCATION(30, 11), "MAX:");
        njPrintD(NJM_LOCATION(35, 11), particle_capacity, 4);
        njPrintC(NJM_LOCATION(30, 12), "NUM:");
        njPrintD(NJM_LOCATION(35, 12), particle_drawn, 4);
        njPrintC(NJM_LOCATION(30, 13), "EXEC:");
        njPrintD(NJM_LOCATION(35, 13), (int)step_time, 4);
        njPrintC(NJM_LOCATION(30, 14), "DRAW:");
        njPrintD(NJM_LOCATION(35, 14), (int)draw_time, 4);
    }
}

void particle_draw_all(void)
{
    struct particle_pool *pool;
    struct particle *particle;
    struct particle_anim *anim;
    NJS_SPRITE *sprite;
    int count;

    count = 0;
    njFogDisable();
    for (pool = particle_pools; pool != NULL; pool = pool->next) {
        anim = pool->anim;
        /* @hack spelling: the particles read before the blend (MOV.L
         * @(4,R12),R14 at 0x8c0a3b86), which keeps the count's 0 in R9 and
         * njColorBlendingMode's address in R10 (0x8c0a3b72, 0x8c0a3b76); read
         * after it, SHC also hoists sprite_set_depth's address and 44 of 86
         * words differ */
        particle = pool->particles;
        /* @hack spelling: the sprite through a pointer, whose address loads
         * into R13 after the loop's test (0x8c0a3b82); by name, it loads
         * before, and 12 of 86 words differ */
        sprite = &particle_sprite;
        sprite->tlist = anim->tlist;
        sprite->tanim = anim->tanim;
        njColorBlendingMode(NJD_SOURCE_COLOR, anim->src_blend);
        njColorBlendingMode(NJD_DESTINATION_COLOR, anim->dst_blend);
        for (; particle != NULL; particle = particle->next) {
            if (particle->step != NULL) {
                njSetConstantMaterial(&particle->color);
                sprite->p = particle->pos;
                sprite->sx = sprite->sy = particle->scale;
                sprite->ang = particle->ang;
                sprite_set_depth(particle->depth);
                sprite_draw_3d(sprite, particle->frame,
                               NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR |
                                   NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
                count++;
            }
        }
    }
    njSetConstantMaterial(&clear_material);
    njFogEnable();
    particle_drawn = count;
}

void particle_free_all(void)
{
    struct particle_pool *pool;
    struct particle_pool *next;

    particle_free_list_init(particle_buffer, particle_capacity);
    for (pool = particle_pools; pool != NULL; pool = next) {
        next = pool->next;
        heap_free(pool);
    }
    particle_pools = NULL;
    particle_pool_count = 0;
}

void particle_release_all(void)
{
    struct particle_pool *pool;
    struct particle *particle;

    for (pool = particle_pools; pool != NULL; pool = pool->next) {
        if (pool->particles != NULL) {
            for (particle = pool->particles; particle->next != NULL;
                 particle = particle->next) {
                particle->step = NULL;
            }
            particle->next = particle_free_list;
            particle_free_list = pool->particles;
            particle_free_count += pool->count;
            pool->particles = NULL;
            pool->count = 0;
        }
    }
}

void particle_set_depth(Float depth)
{
    particle_depth = depth;
}
