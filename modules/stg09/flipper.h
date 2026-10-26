/* FlipperL and FlipperR: the left and right flippers of the pinball tables.
 * FlipperLx and FlipperRx: the same, with one more flag, which no code of
 * theirs reads */
#ifndef STG09_FLIPPER_H
#define STG09_FLIPPER_H

#include "system/task.h"

/* the object list's steps. task needs TASK_WORK's record. scl.x + 1 scales
 * a flipper's swing each step and its push. the left flippers swing up while
 * the first pad holds L or A. the right ones swing up while it holds R or A.
 * while Sonic is played and player 1 is present, the second pad's buttons
 * swing them too */
void flipper_left_step(struct task *task);
void flipper_right_step(struct task *task);
void flipper_left_x_step(struct task *task);
void flipper_right_x_step(struct task *task);

#endif
