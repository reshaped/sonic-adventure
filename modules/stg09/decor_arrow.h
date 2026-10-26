/* KazariYaji: a decorative arrow on the tables, which cycles through four
 * models */
#ifndef STG09_DECOR_ARROW_H
#define STG09_DECOR_ARROW_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record */
void decor_arrow_step(struct task *task);

#endif
