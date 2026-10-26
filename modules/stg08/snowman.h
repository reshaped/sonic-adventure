/* E SMAN, a snowman that chases player 0 and blows a freezing breath. once
 * its body is beaten, its head hops out and runs from player 0 */
#ifndef STG08_SNOWMAN_H
#define STG08_SNOWMAN_H
#include <ninja.h>

#include "effect/pools.h"
#include "system/task.h"

/* E_SNOWMAN's texlist, which icecap_pvms loads */
extern NJS_TEXLIST e_snowman_texlist;
/* the snow puffs' pool, which 1ST_READ.BIN's icecap_pools names by its
 * address */
extern struct particle_pool_spec snowman_snow_pool;

/* the step icecap_object_list names for E SMAN */
void snowman_step(struct task *task);

#endif
