/* the stage's platforms. O FLOAT tilts toward a player standing on it. O
 * FLYER sinks under a player and rises back. O FOOTHOLD stays still */
#ifndef STG03_PLATFORMS_H
#define STG03_PLATFORMS_H

#include "system/task.h"

/* task steps for O FLOAT, O FLYER and O FOOTHOLD, named in the object list.
 * each needs its task to have a work. twinkle_flyer_step needs a force too */
void twinkle_float_step(struct task *task);
void twinkle_flyer_step(struct task *task);
void twinkle_foothold_step(struct task *task);

#endif
