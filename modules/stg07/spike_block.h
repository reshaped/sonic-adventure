/* the spike blocks: TogedaiUD and TogedaiD */
#ifndef STG07_SPIKE_BLOCK_H
#define STG07_SPIKE_BLOCK_H

#include "system/task.h"

/* the object list's steps: the first rises back after each drop, and the
 * second stays down */
void spike_block_up_down_step(struct task *task);
void spike_block_down_step(struct task *task);

#endif
