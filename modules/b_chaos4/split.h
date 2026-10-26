/* Chaos 4's split into four pieces, which melt back into him */
#ifndef B_CHAOS4_SPLIT_H
#define B_CHAOS4_SPLIT_H

#include "system/task.h"

/* his step in BOSS_SPLIT, which his task's step calls. extra is his
 * struct chaos */
void chaos4_split(struct task_work *work, struct task_motion *motion,
                  void *extra);

#endif
