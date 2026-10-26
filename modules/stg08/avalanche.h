/* the avalanche on the board run: two waves of pieces that chase player 0
 * down their paths. the second wave drops rocks */
#ifndef STG08_AVALANCHE_H
#define STG08_AVALANCHE_H

#include "system/task.h"

/* the step avalanche_path_1 names, so the path's task runs it. each wave
 * starts once player 0 comes down to its height */
void avalanche_step(struct task *task);

#endif
