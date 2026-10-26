/* O M EMERALD, the Master Emerald, which turns and bobs */
#ifndef ADV03_MASTER_EMERALD_H
#define ADV03_MASTER_EMERALD_H

#include "system/task.h"

/* the step the object list names for O M EMERALD. its first step gives the
 * emerald its collision and its draw, and each step after turns and bobs it.
 * every step draws the emerald and registers its collision */
void master_emerald_step(struct task *task);

#endif
