/* O NEON1 to O NEON4 neon signs whose tubes light in turn */
#ifndef STG04_NEON_H
#define STG04_NEON_H

#include "system/task.h"

/* task steps for O NEON1 to O NEON4, named in the object list */
void neon_1_step(struct task *task);
void neon_2_step(struct task *task);
void neon_3_step(struct task *task);
void neon_4_step(struct task *task);

#endif
