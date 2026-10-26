/* O DRUM drum that bursts into pieces and chips when an attack hurts it */
#ifndef STG10_DRUM_H
#define STG10_DRUM_H
#include <ninja.h>

#include "effect/pools.h"
#include "system/task.h"

/* the drum's model, which O AMY DRUM draws too */
extern NJS_OBJECT final_drum_object;

/* the pool of the pieces' dust. final_pools lists it, so the stage makes it
 * as it starts */
extern struct particle_pool_spec final_drum_dust_pool;

/* the object list's step */
void final_drum_step(struct task *task);

#endif
