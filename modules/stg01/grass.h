/* O GRASA to O GRASD: tufts of grass, each a model that only draws. O GRASB
 * alone draws scaled, by one more than its set entry's scale */
#ifndef STG01_GRASS_H
#define STG01_GRASS_H

#include "system/task.h"

/* the steps beach_object_list names for O GRASA to O GRASD */
void beach_grass_a_step(struct task *task);
void beach_grass_b_step(struct task *task);
void beach_grass_c_step(struct task *task);
void beach_grass_d_step(struct task *task);

#endif
