/* the chasing orca, which leaps out of the sea after player 0 in act 0 */
#ifndef STG01_ORCA_H
#define STG01_ORCA_H

#include "system/task.h"

/* the chasing orca's first step. the task needs a work and a motion.
 * work->pos holds the orca's place, and work->ang.y its heading. its
 * destructor calls orca_clear_task */
void orca_init(struct task *task);

#endif
