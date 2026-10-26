/* O MKUSA and O BKUSA plants, and O K2, a plant that sends up butterflies as
 * player 0 runs past */
#ifndef STG01_PLANTS_H
#define STG01_PLANTS_H

#include "system/task.h"

/* the object list's steps for O MKUSA, O BKUSA and O K2. outside the set
 * editor, O BKUSA's and O K2's steps read player 0's work, which must be
 * set. O BKUSA's task needs TASK_EXTRA's record, where a touch keeps a
 * vector */
void beach_mkusa_step(struct task *task);
void beach_bkusa_step(struct task *task);
void beach_k2_step(struct task *task);

#endif
