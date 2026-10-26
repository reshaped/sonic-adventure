/* a spotlight that follows above player 0 and shines back down at it */
#ifndef STG04_SPOTLIGHT_H
#define STG04_SPOTLIGHT_H

#include "system/task.h"

/* the light's task step, which kills the task in any act but 0. nothing in
 * STG04.BIN names it */
void spotlight_step(struct task *task);

#endif
