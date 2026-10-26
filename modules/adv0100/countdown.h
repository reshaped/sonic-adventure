/* countdown task step that no object list or code refers to */
#ifndef ADV0100_COUNTDOWN_H
#define ADV0100_COUNTDOWN_H

#include "system/task.h"

/* its first step sets work->timer to 0. each later step takes scl.y from
 * scl.x, and where that leaves scl.x at 0 or below, adds scl.z to it and sets
 * work->timer to 20 */
void fun_0c92d2a0(struct task *task);

#endif
