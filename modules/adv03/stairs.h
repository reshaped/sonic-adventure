/* O PYSTAIRS and O BIGSTAIRS, the pyramid's stairs */
#ifndef ADV03_STAIRS_H
#define ADV03_STAIRS_H

#include "system/task.h"

/* the steps the object list names for O PYSTAIRS and O BIGSTAIRS. each
 * draws its stairs on every step: the near model within 400 of the camera,
 * and the far one beyond it. neither checks its placement's range */
void pyramid_stairs_step(struct task *task);
void big_stairs_step(struct task *task);

#endif
