/* O PALM, a palm tree, which burns in PAST_FIRE */
#ifndef ADV03_PALM_H
#define ADV03_PALM_H

#include "system/task.h"

/* the step the object list names for O PALM. it draws the palm on each
 * step, burning in PAST_FIRE. once placement_try_leave finds the task out of
 * range, the step does nothing, and the task destroys itself on its next step
 */
void palm_step(struct task *task);

#endif
