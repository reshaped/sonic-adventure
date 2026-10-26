/* O LIGHT4: a standing lamp whose parts loop their 200-frame motion, a
 * frame each step */
#ifndef STG12_LIGHT_4_H
#define STG12_LIGHT_4_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record */
void shelter_light_4_step(struct task *task);

#endif
