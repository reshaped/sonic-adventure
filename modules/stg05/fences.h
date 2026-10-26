/* the O SAKU, O SAKU2, MT_TSAKU and MT_SOTO fences, the O ROUYA prison door
 * and the MT_TBOX crate. each draws at one more than its set entry's
 * scale */
#ifndef STG05_FENCES_H
#define STG05_FENCES_H

#include "system/task.h"

/* task steps for O SAKU, O SAKU2, O ROUYA, MT_SOTO, MT_TBOX and MT_TSAKU, named
 * in the object list */
void mountain_fence_step(struct task *task);
void mountain_fence_2_step(struct task *task);
void prison_door_step(struct task *task);
void mountain_fence_outside_step(struct task *task);
void mountain_crate_step(struct task *task);
void iron_fence_step(struct task *task);

#endif
