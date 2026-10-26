/* particle pools: those every stage makes, and those each stage adds */
#ifndef EFFECT_POOLS_H
#define EFFECT_POOLS_H
#include <sg_xpt.h>

struct particle_anim;
struct particle_pool;

/* a pool for particle_create_pools to make, and where it keeps it */
struct particle_pool_spec {
    struct particle_anim *anim;
    Uint32 size; /* the bytes of work after the pool */
    /* called with the new pool, which is null when it can't be made. init
     * may be null */
    void (*init)(struct particle_pool *pool);
    struct particle_pool *pool; /* null until made, or if it can't be */
};

/* make the pools every stage has, then the current stage's own */
void particle_create_pools(void);

#endif
