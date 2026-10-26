/* Eggman in the Egg Walker's cockpit */
#ifndef B_EGM2_EGGMAN_H
#define B_EGM2_EGGMAN_H

#include "system/task.h"

/* draw Eggman in his seat. the current matrix must be the turret's, as the
 * boss's draw leaves it. task is the boss's, and goes unread */
void egg_walker_eggman_draw(struct task *task);

#endif
