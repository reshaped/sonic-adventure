/* O POSTER0, O POSTER1 and O POSTER2 scaled poster models */
#ifndef STG04_POSTER_H
#define STG04_POSTER_H

#include "system/task.h"

/* the object list's steps, one for each poster. each draws its model
 * through model_start_scaled, at one more than work->scl on each axis */
void highway_poster_0_step(struct task *task);
void highway_poster_1_step(struct task *task);
void highway_poster_2_step(struct task *task);

#endif
