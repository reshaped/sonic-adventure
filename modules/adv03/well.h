/* O WELL, a solid box with the well drawn round it */
#ifndef ADV03_WELL_H
#define ADV03_WELL_H

#include "system/task.h"

/* the step the object list names for O WELL. it draws the well and its
 * shadow on each step and registers the well's box. once placement_try_leave
 * finds the task out of range, the step does nothing, and the task destroys
 * itself on its next step */
void well_step(struct task *task);

#endif
