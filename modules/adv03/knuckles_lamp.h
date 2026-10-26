/* O KNULAMP, a lamp under a flickering red glow */
#ifndef ADV03_KNUCKLES_LAMP_H
#define ADV03_KNUCKLES_LAMP_H

#include "system/task.h"

/* the step the object list names for O KNULAMP. it draws the lamp and its
 * glow on each step, and from its second step it flickers the glow. once
 * placement_try_leave finds the task out of range, the step does nothing, and
 * the task destroys itself on its next step */
void knuckles_lamp_step(struct task *task);

#endif
