/* O S2SAKU fence of the second shop, which comes down while the egg stand is
 * empty */
#ifndef ADV00_SHOP2_FENCE_H
#define ADV00_SHOP2_FENCE_H

#include "system/task.h"

/* the step the object list names for O S2SAKU. it starts the fence on its
 * first step while the set editor is closed, and runs it on each after */
void shop2_fence_step(struct task *task);

#endif
