/* the light: O RLIGHT, and the glow a panel spreads */
#ifndef STG07_LIGHT_H
#define STG07_LIGHT_H

#include "system/task.h"

/* the object list's step */
void ruin_light_step(struct task *task);
/* a flat glow at the task's pos, facing the way its ang.y faces, that
 * spreads as it fades. its scl.y adds to the spread each step */
void ruin_light_glow_step(struct task *task);

#endif
