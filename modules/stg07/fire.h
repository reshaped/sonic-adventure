/* O FIRE, flames that rise round a glow */
#ifndef STG07_FIRE_H
#define STG07_FIRE_H

#include "system/task.h"

/* the step ruin_object_list names for O FIRE. the snake statues of
 * snake_statue.c and mirror.c also start fires with it, 10 above themselves */
void ruin_fire_step(struct task *task);

#endif
