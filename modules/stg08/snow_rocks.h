/* O SnowNewA, O SnowMountA to O SnowMountD, O SnowRockA to O SnowRockD and
 * O SnowStoneA to O SnowStoneF: snowy mounds, rocks and stones in the
 * collision world, each scaled by its set entry */
#ifndef STG08_SNOW_ROCKS_H
#define STG08_SNOW_ROCKS_H

#include "system/task.h"

/* the steps icecap_object_list names for them, in its order */
void snow_new_a_step(struct task *task);
void snow_mount_a_step(struct task *task);
void snow_mount_b_step(struct task *task);
void snow_mount_c_step(struct task *task);
void snow_mount_d_step(struct task *task);
void snow_rock_a_step(struct task *task);
void snow_rock_b_step(struct task *task);
void snow_rock_c_step(struct task *task);
void snow_rock_d_step(struct task *task);
void snow_stone_a_step(struct task *task);
void snow_stone_b_step(struct task *task);
void snow_stone_c_step(struct task *task);
void snow_stone_d_step(struct task *task);
void snow_stone_e_step(struct task *task);
void snow_stone_f_step(struct task *task);

#endif
