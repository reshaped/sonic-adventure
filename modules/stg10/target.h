/* targets: boards with a character on them that break into pieces when hit,
 * and their box */
#ifndef STG10_TARGET_H
#define STG10_TARGET_H

#include "system/task.h"

/* the object list's steps. O Target is a board that rides a cart back and
 * forth. O UP TARGET1 is a box that opens and raises a board. 0 UP TARGET2,
 * spelled with a zero, is the box alone, shut */
void moving_target_step(struct task *task);
void pop_up_target_step(struct task *task);
void target_box_step(struct task *task);

#endif
