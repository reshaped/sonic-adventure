/* O BREAKSTEP, runs of rock blocks. a block landed on shakes and falls, and
 * its neighbors fall after it */
#ifndef STG05_BREAK_STEP_H
#define STG05_BREAK_STEP_H

#include "system/task.h"

/* the object list's step */
void break_step_step(struct task *task);

#endif
