/* O HAGURUMA1: a gear that turns about z, the other way while its switch
 * is on. it carries player 0 round with it */
#ifndef STG12_GEAR_1_H
#define STG12_GEAR_1_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's and TASK_FORCE's
 * records */
void gear_1_step(struct task *task);

#endif
