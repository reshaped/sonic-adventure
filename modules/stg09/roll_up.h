/* RollUpR, RollUpL and RollDownL, the platforms on the tables that turn
 * about y */
#ifndef STG09_ROLL_UP_H
#define STG09_ROLL_UP_H

#include "system/task.h"

/* the object list's steps. RollUpR turns without stopping. RollUpL stops at
 * each quarter turn, given a y angle with bit 8 clear. RollDownL swings
 * through a whole turn and back, and stops at each end. it ignores its set
 * entry's y angle */
void roll_up_right_step(struct task *task);
void roll_up_left_step(struct task *task);
void roll_down_left_step(struct task *task);

#endif
