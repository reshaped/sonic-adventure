/* Chaos 4's sickle, a curved wave he sends across the swamp at player 0 */
#ifndef B_CHAOS4_SICKLE_H
#define B_CHAOS4_SICKLE_H

#include "chaos/chaos.h"
#include "system/task.h"

/* his step in BOSS_SICKLE, which his task's step calls */
void chaos4_sickle(struct task_work *work, struct task_motion *motion,
                   struct chaos *chaos);

#endif
