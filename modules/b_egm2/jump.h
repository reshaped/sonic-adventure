/* the Egg Walker's jump */
#ifndef B_EGM2_JUMP_H
#define B_EGM2_JUMP_H

#include "system/task.h"

/* the boss's step in EGG_WALKER_JUMP. it jumps where struct egg_walker's
 * jump says, and lands. low on hits, it scatters bombs once in its flight.
 * lower still, it jumps again if player 0 is close across the ground as it
 * lands. a jump to a random spot is followed by one to player 0, and leaves
 * jump at JUMP_TO_PLAYER. the boss then goes back to EGG_WALKER_WALK. task
 * is the boss's */
void egg_walker_jump(struct task *task);

#endif
