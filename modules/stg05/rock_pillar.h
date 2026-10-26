/* O DEKA, a rock pillar that topples and lies where it falls. it topples
 * once E-102 has shot down its targets, or for another character, once a
 * player comes near */
#ifndef STG05_ROCK_PILLAR_H
#define STG05_ROCK_PILLAR_H

#include "system/task.h"

/* the step mountain_object_list names for O DEKA. once the pillar topples,
 * it writes through null when every object of the world's pool is lent */
void rock_pillar_step(struct task *task);

#endif
