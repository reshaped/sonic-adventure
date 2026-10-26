/* particles: sprites from a free list, stepped and drawn in pools */
#ifndef EFFECT_PARTICLE_H
#define EFFECT_PARTICLE_H
#include <ninja.h>
#include <sg_xpt.h>

struct particle_pool;

/* how a pool's particles draw: the frames of tanim, from tlist's textures,
 * in two of njColorBlendingMode's modes */
struct particle_anim {
    NJS_TEXLIST *tlist;
    NJS_TEXANIM *tanim;
    Sint32 frames; /* tanim's count */
    Sint32 src_blend; /* for NJD_SOURCE_COLOR */
    Sint32 dst_blend; /* for NJD_DESTINATION_COLOR */
};

/* an effect's own two words, which it reads through a struct of its own
 * laid over them, as dust.h's DUST does */
union particle_vars {
    Sint32 l[2];
    Float f[2];
    void *p[2];
};

/* a particle with a null step has ended. particle_draw_all skips it, and
 * the next particle_run returns it to the free list */
struct particle {
    struct particle *next;
    void (*step)(struct particle *particle, struct particle_pool *pool);
    /* left to the effect, as vars are */
    Uint8 life;
    Uint8 unk_09;
    Sint16 frame; /* index in the pool's sprite animation */
    Angle ang;
    /* the frame as a Float, which a step truncates into frame, then advances */
    Float time;
    Float scale;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_ARGB color;
    Float depth; /* what sprite_set_depth adds to its z as it draws */
    union particle_vars vars;
};

/* particles that draw alike, with the pool's own step and work. don't
 * allocate from a pool once particle_pool_kill or particle_pool_release
 * has run on it. particle_run frees a dead pool once it holds no particle,
 * and particle_alloc doesn't test for a dead one */
struct particle_pool {
    struct particle_pool *next;
    struct particle *particles;
    /* the step particle_pool_step runs, or null */
    void (*step)(struct particle_pool *pool);
    Sint32 count; /* particles linked in this pool, including ended ones */
    struct particle_anim *anim; /* borrowed until the pool is freed */
    Sint32 flags; /* the paused and dead bits the particle_pool_ calls set */
    void *work; /* the zeroed bytes after the pool, or null for none */
};

/* make the free list from the count particles in buffer, count at least 1.
 * buffer is borrowed until the next particle_init. the pool list starts over
 * without its pools freed, so free them first with particle_free_all */
void particle_init(struct particle *buffer, Uint32 count);
/* move a free particle to the front of pool's list, with step and the
 * depth particle_set_depth last set. returns it, or null for a null pool or
 * when none is free. its other fields keep what they held, so set them
 * before the next particle_run or particle_draw_all */
struct particle *particle_alloc(struct particle_pool *pool,
                                void (*step)(struct particle *particle,
                                             struct particle_pool *pool));
/* end the particle: its step becomes null */
void particle_kill(struct particle *particle);
/* make a zeroed pool, with size bytes of work after it. returns it, or null
 * for a null anim or when it can't be allocated. anim and its sprites are
 * borrowed until the pool is freed */
struct particle_pool *particle_pool_create(struct particle_anim *anim,
                                           Uint32 size);
/* run pool's own step, if it has one. returns TRUE, or FALSE without
 * running it for a dead or paused pool. pool must not be null */
Bool particle_pool_step(struct particle_pool *pool);
/* mark the pool dead. its particles run on until each ends */
void particle_pool_kill(struct particle_pool *pool);
/* mark a live pool dead and return its particles to the free list now. a
 * pool already dead stays as it is, particles and all */
void particle_pool_release(struct particle_pool *pool);
/* pause or resume the pool's own step, which particle_pool_step runs. its
 * particles run either way. resume does nothing to a dead pool */
void particle_pool_pause(struct particle_pool *pool);
void particle_pool_resume(struct particle_pool *pool);
/* run every particle's step, return the ended ones to the free list, and
 * free each dead pool that holds no particle. then, on a frame's first step
 * only, draw them with particle_draw_all. the pools' own steps don't run.
 * while debug_page is set, it prints particle_init's count, how many
 * particles the last draw drew, and the time each half took */
void particle_run(void);
/* draw every particle with a step. it leaves fog on, the constant material
 * zeroed and the last pool's blend modes set */
void particle_draw_all(void);
/* free every pool, and make the free list again from particle_init's
 * buffer. every pool and particle pointer goes stale */
void particle_free_all(void);
/* return every pool's particles to the free list, and leave the pools'
 * flags as they are */
void particle_release_all(void);
/* the particles particle_alloc takes from now on draw at depth. it is 0
 * after particle_init */
void particle_set_depth(Float depth);

#endif
