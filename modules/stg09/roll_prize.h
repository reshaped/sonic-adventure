/* the prize for a line on Roll's reels. the screen dims, and the symbol's
 * word comes in a letter at a time, each with a flash. a model then grows,
 * and the screen turns white and fades */
#ifndef STG09_ROLL_PRIZE_H
#define STG09_ROLL_PRIZE_H

#include "system/task.h"

/* the prize's first step. Roll creates its task with work->mode set to the
 * line's symbol, 0 to 5. the task ends itself once it has counted down 75
 * frames. it ends sooner once the act is no longer the first table */
void roll_prize_step(struct task *task);

#endif
