/* O FIRE, a fire sprite that plays bigfire01 to 16 */
#ifndef ADV03_FIRE_H
#define ADV03_FIRE_H

#include "system/task.h"

/* the step the object list names for O FIRE. it draws the fire on each step
 * from its second. the task has no draw, so the fire doesn't show under the
 * pause menu. once placement_try_leave finds the task out of range, the step
 * does nothing, and the task destroys itself on its next step */
void fire_step(struct task *task);

#endif
