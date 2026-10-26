/* O_TANPOPO, a dandelion that sheds its seeds. O POPO, a flower with two
 * heads. O HANAA, O HANAB, O GRFLOWERA and O GRFLOWERB, flowers that are
 * only drawn */
#ifndef STG02_DANDELION_H
#define STG02_DANDELION_H
#include <sg_xpt.h>

#include "system/task.h"

/* the object list's steps for O_TANPOPO, O POPO, O HANAA, O HANAB,
 * O GRFLOWERB and O GRFLOWERA, in that order. O_TANPOPO's and O POPO's
 * heads, and O_TANPOPO's seeds, turn to face the camera as they draw, so
 * camera_work must be set while they draw */
void windy_dandelion_step(struct task *task);
void windy_popo_step(struct task *task);
void windy_flower_a_step(struct task *task);
void windy_flower_b_step(struct task *task);
void windy_grass_flower_b_step(struct task *task);
void windy_grass_flower_a_step(struct task *task);
/* TRUE while none of task's children has task_destroy as its step, which
 * task_kill sets. nothing calls it */
Bool windy_no_child_killed(struct task *task);

#endif
