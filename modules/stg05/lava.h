/* o_yougan, a lava sheet that rises as MagmaCnt sets it, and O MYOUGAN and
 * O YE102, lava pools. all three draw one texture animation */
#ifndef STG05_LAVA_H
#define STG05_LAVA_H

#include "system/task.h"

/* the steps mountain_object_list names for o_yougan, O MYOUGAN and O YE102.
 * o_yougan reads mountain_work, which must be set as it starts and while it
 * bobs */
void lava_step(struct task *task);
void lava_pool_step(struct task *task);
void lava_e102_step(struct task *task);
/* the step of the lava's texture animation. the stage task creates its task
 * as each act starts. the task destroys itself once the stage or act
 * changes */
void lava_anim_step(struct task *task);

#endif
