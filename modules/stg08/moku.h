/* O MOKU A to O MOKU C, O DRY A and O DRY B: scenery that is only drawn */
#ifndef STG08_MOKU_H
#define STG08_MOKU_H

#include "system/task.h"

/* the object list's steps */
void moku_a_step(struct task *task);
void moku_b_step(struct task *task);
void moku_c_step(struct task *task);
void dry_a_step(struct task *task);
void dry_b_step(struct task *task);

#endif
