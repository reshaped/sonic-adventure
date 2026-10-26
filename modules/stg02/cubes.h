/* O_CUBES, O_CUBEM, O_CUBEL, O_UKISIM, O TATES and O TATEL: models that
 * stand solid. O BIGFLA: a model in the collision world */
#ifndef STG02_CUBES_H
#define STG02_CUBES_H

#include "system/task.h"

/* the object list's steps for O_CUBES, O_CUBEM, O_CUBEL, O_UKISIM, O BIGFLA,
 * O TATEL and O TATES, in that order */
void windy_small_cube_step(struct task *task);
void windy_medium_cube_step(struct task *task);
void windy_large_cube_step(struct task *task);
void windy_floating_island_step(struct task *task);
void windy_bigfla_step(struct task *task);
void windy_tate_large_step(struct task *task);
void windy_tate_small_step(struct task *task);

#endif
