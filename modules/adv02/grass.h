/* the clumps of grass the object list names O GRASS and O BIG GRASS. O GRASS
 * rustles as player 0 comes near, and O BIG GRASS sways as player 0 runs
 * through it */
#ifndef ADV02_GRASS_H
#define ADV02_GRASS_H

#include "system/task.h"

/* the steps the object list names for O GRASS and O BIG GRASS */
void grass_step(struct task *task);
void big_grass_step(struct task *task);

#endif
