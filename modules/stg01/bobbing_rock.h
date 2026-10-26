/* O IWA, a rock that bobs under player 0 and tips toward them */
#ifndef STG01_BOBBING_ROCK_H
#define STG01_BOBBING_ROCK_H

#include "system/task.h"

/* the step the object list names for O IWA. the task needs a work, a force
 * and an extra. its first step puts the rock in the collision world. outside
 * the set editor, the rock then bobs and tips under player 0 */
void beach_bobbing_rock_step(struct task *task);

#endif
