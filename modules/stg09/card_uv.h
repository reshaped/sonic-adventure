/* CardUV: seven models of scenery on the pinball tables, whose UVs scroll */
#ifndef STG09_CARD_UV_H
#define STG09_CARD_UV_H

#include "system/task.h"

/* the object list's step. the models stand at fixed places in the world,
 * whatever the placement's */
void card_uv_step(struct task *task);

#endif
