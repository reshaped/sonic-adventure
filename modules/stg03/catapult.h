/* the catapult, which the set editor names O CATAPULT. a band between two
 * sliding posts catches player 0, and A or B launches it */
#ifndef STG03_CATAPULT_H
#define STG03_CATAPULT_H

#include "system/task.h"

/* the step twinkle_object_kinds names for O CATAPULT. its task needs a
 * work, and player 0 must exist */
void catapult_step(struct task *task);

#endif
