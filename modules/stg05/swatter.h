/* O HATAKI, an arm whose rockets swing it over its hinge. it hurts players
 * as it comes down on the far side, and swings back after a wait */
#ifndef STG05_SWATTER_H
#define STG05_SWATTER_H

#include "system/task.h"

/* the step mountain_object_list names for O HATAKI. its task needs a work
 * and a motion */
void swatter_step(struct task *task);

#endif
