/* the ruin's water textures: a task for each act */
#ifndef STG07_TEXTURES_H
#define STG07_TEXTURES_H

#include "system/task.h"

/* the tasks ruin_sky_draw starts as each act's sky lights. each cycles the
 * textures of its act's water until the act changes */
void ruin_act0_textures_step(struct task *task);
void ruin_act1_textures_step(struct task *task);
void ruin_act2_textures_step(struct task *task);

#endif
