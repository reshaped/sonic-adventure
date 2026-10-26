/* the Egg Walker's cannon attack: three missiles lobbed at player 0. they
 * trail smoke and blow up where they land */
#ifndef B_EGM2_CANNONS_H
#define B_EGM2_CANNONS_H

#include "system/task.h"

/* the boss's step in EGG_WALKER_CANNONS: it aims, turns and fires three
 * times, then goes back to EGG_WALKER_WALK. task is the boss's */
void egg_walker_cannons(struct task *task);
/* set up the sprite the missiles' smoke draws. call it before any smoke
 * draws, as the boss's EGG_WALKER_INIT step does */
void egg_walker_cannons_init(void);

#endif
