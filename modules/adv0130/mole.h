/* Minigame Mogura, the Hedgehog Hammer: a machine whose targets rise for Amy
 * to hit, under a board that keeps the day's best */
#ifndef ADV0130_MOLE_H
#define ADV0130_MOLE_H

#include "system/task.h"

/* the step the object list names for Minigame Mogura. its task needs a work
 * and an extra, where the step keeps the game. the first step allocates the
 * game from the heap, and writes through null if the heap runs out. in the
 * set editor the machine and its board are then only drawn. elsewhere the
 * first step also gives the machine collision. for Amy alone, it makes each
 * target's two collision tasks as children. the destructor frees the game
 * and enables player 0's pad */
void mole_step(struct task *task);

#endif
