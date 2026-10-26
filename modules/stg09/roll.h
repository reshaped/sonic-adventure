/* Roll, the first table's slot machine. a touch holds player 0 and spins
 * its three reels, and presses of A stop them in turn. three of one symbol
 * in a line start that symbol's countdown */
#ifndef STG09_ROLL_H
#define STG09_ROLL_H

#include "system/task.h"

/* Roll's step in the object list */
void roll_step(struct task *task);

#endif
