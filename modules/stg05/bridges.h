/* O TURI1 to O TURI4, rope bridges. each one's first step hands its parts to
 * bridge_init, which builds the bridge and takes over the task */
#ifndef STG05_BRIDGES_H
#define STG05_BRIDGES_H

#include "system/task.h"

/* the object list's steps */
void mountain_bridge_1_step(struct task *task);
void mountain_bridge_2_step(struct task *task);
void mountain_bridge_3_step(struct task *task);
void mountain_bridge_4_step(struct task *task);

#endif
