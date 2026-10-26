/* O HAGURUMA2: O HAGURUMA1 half as big again across its face. it turns
 * about z, the other way while its switch is on. it carries player 0 round
 * with it */
#ifndef STG12_GEAR_2_H
#define STG12_GEAR_2_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's and TASK_FORCE's
 * records */
void gear_2_step(struct task *task);

#endif
