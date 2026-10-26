/* O SIDE_ARM robot arm that reaches out and back, turned toward the nearest
 * player, and bursts apart when hit */
#ifndef STG10_SIDE_ARM_H
#define STG10_SIDE_ARM_H

#include "system/task.h"

/* the object list's step */
void side_arm_step(struct task *task);

#endif
