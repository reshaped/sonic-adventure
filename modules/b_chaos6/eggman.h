/* Chaos 6's Eggman, who flies over the deck in the Egg Mobile in Sonic's
 * fight */
#ifndef B_CHAOS6_EGGMAN_H
#define B_CHAOS6_EGGMAN_H

#include "system/task.h"

/* the first step of Eggman's task, which needs a work and a motion. he
 * starts at y 844 over the fight's center */
void chaos6_eggman_step(struct task *task);
/* drop a bomb from where Eggman is, as a child of his task. a sound plays
 * with it, and his voice while no hint is waiting. it drops none unless the
 * character is Sonic, and none while the boss is in BOSS_START or six bombs
 * are out. his task must have run its first step */
void chaos6_eggman_drop_bomb(void);

#endif
