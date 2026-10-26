/* the kirai and the hodai: KIRAI, a mine, and HODAI, a battery */
#ifndef SHOOTING_KIRAI_H
#define SHOOTING_KIRAI_H

#include <sg_xpt.h>

#include "system/task.h"

/* the object list's steps for KIRAI and HODAI */
void kirai_step(struct task *task);
void hodai_step(struct task *task);
/* TRUE while task's pos is within reach of the camera's, else FALSE.
 * reach is a distance squared */
Bool hodai_near(struct task *task, Float reach);

#endif
