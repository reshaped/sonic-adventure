/* act 1's texture cycle, which steps four models' materials through a run
 * of textures */
#ifndef STG03_CYCLE_H
#define STG03_CYCLE_H

#include "system/task.h"

/* the cycle's step, whose task twinkle.c starts as act 1 begins. the task
 * needs a work. outside act 1, the step kills it */
void twinkle_cycle_step(struct task *task);

#endif
