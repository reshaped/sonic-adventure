/* the two rocks that burst under player 0 in Red Mountain act 0, and Ex
 * Rock, which kills itself on its first step */
#ifndef STG05_ROCK_BURST_H
#define STG05_ROCK_BURST_H

#include "effect/pools.h"
#include "system/task.h"

/* the pool of the rocks' dust, which 1ST_READ.BIN's mountain_pools names
 * for particle_create_pools to make as the stage starts */
extern struct particle_pool_spec rock_burst_dust_pool;

/* the step mountain_object_list names for Ex Rock. its task destroys itself
 * on its next step, and its placement makes no more */
void ex_rock_step(struct task *task);

/* create the two rocks. each bursts once player 0 has stood on its pieces
 * for 21 steps running. it then flings player 0 toward a point of its own.
 * player 0 must exist until each rock has flung it. the camera must exist as
 * a rock bursts. a rock, piece or fragment that gets no task is left out. a
 * piece writes through null when every object of the world's pool is lent */
void rock_burst_create(void);

#endif
