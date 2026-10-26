/* O LIGHT, a siren light whose mirror turns, and O LIGHTB, a yellow lamp that
 * stands still */
#ifndef STG05_LIGHT_H
#define STG05_LIGHT_H

#include "system/task.h"

/* the steps mountain_object_list names for O LIGHT and O LIGHTB */
void mountain_light_step(struct task *task);
void mountain_light_b_step(struct task *task);

#endif
