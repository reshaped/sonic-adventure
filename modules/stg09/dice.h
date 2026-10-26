/* Saico: a die on the tables, which rolls when hit */
#ifndef STG09_DICE_H
#define STG09_DICE_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record */
void dice_step(struct task *task);

#endif
