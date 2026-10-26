/* Minigame Mogura, the Hedgehog Hammer's machine */
#ifndef STG00_MOLE_H
#define STG00_MOLE_H

#include "system/task.h"

/* the step the object list names for Minigame Mogura. its task needs a work
 * and an extra, where the step keeps the game. the first step allocates the
 * game from the heap, and writes through null if the heap runs out. in the
 * set editor the machine and its board are then only drawn. elsewhere the
 * first step also gives the machine collision. for Amy alone, it makes each
 * target's two collision tasks as children. after a round, the game over's
 * choice restarts the stage or ends the act. the destructor frees the game */
void mole_step(struct task *task);
/* report that the score screen has ended, so the machine's results go on to
 * the game over. the Hedgehog Hammer's score screen calls it as it ends */
void mole_score_done(void);

#endif
