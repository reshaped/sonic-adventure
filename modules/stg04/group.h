/* group objects that each place a row of one Speed Highway object */
#ifndef STG04_GROUP_H
#define STG04_GROUP_H

#include "system/task.h"

/* task steps for O GFENCE, O GCURB, O GFENCE02, O GPINPIN, O GFF, O GRAFTA,
 * O GRAFTC, O GGRENA, O GGRENB, O GGREND and O GCLIGHT, named in the object
 * list. O GFENCE places the fences O FF names, and O GFF those O FENCE names.
 * O GCURB and O GRAFTA place a curb and a raft of their own */
void fence_group_step(struct task *task);
void curb_group_step(struct task *task);
void fence_2_group_step(struct task *task);
void pinpin_group_step(struct task *task);
void fence_ff_group_step(struct task *task);
void raft_a_group_step(struct task *task);
void raft_c_group_step(struct task *task);
void green_a_group_step(struct task *task);
void green_b_group_step(struct task *task);
void green_d_group_step(struct task *task);
void corner_light_group_step(struct task *task);

#endif
