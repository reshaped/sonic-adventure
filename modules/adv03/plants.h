/* O SHIDA, a fern, and O SOTETSU, a cycad. each sways as a touch on it starts
 * or ends */
#ifndef ADV03_PLANTS_H
#define ADV03_PLANTS_H

#include "system/task.h"

/* the steps the object list names for O SHIDA and O SOTETSU. each draws its
 * plant on each step. while the set editor holds the task, the plant only
 * draws. once placement_try_leave finds the task out of range, the step does
 * nothing, and the task destroys itself on its next step */
void shida_step(struct task *task);
void sotetsu_step(struct task *task);

#endif
