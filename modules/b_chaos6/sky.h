/* Chaos 6's sky, and the clouds that stream past the deck */
#ifndef B_CHAOS6_SKY_H
#define B_CHAOS6_SKY_H
#include <ninja.h>

#include "system/task.h"

/* the step of the sky's task. 1ST_READ.BIN's sky_steps reaches it for the
 * stage by its address, 0x0c9000a0, not by its name */
void chaos6_sky_step(struct task *task);
/* the step of the clouds' task, which chaos6_start creates after the boss.
 * the task kills itself once the act changes */
void chaos6_clouds_step(struct task *task);

/* the sky's textures, which chaos6_pvms loads from CHAOS6_BG */
extern NJS_TEXLIST chaos6_bg_texlist;

#endif
