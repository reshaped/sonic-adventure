/* the Hedgehog Hammer's stage task */
#ifndef STG00_PRACTICE_H
#define STG00_PRACTICE_H

#include "system/task.h"

/* the task 1ST_READ.BIN starts for the stage, from its stage_steps. it sets
 * up the act the stage starts in, and again each time the act changes */
void practice_step(struct task *task);

#endif
