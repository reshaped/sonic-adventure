/* Dtarget: a row of three drop targets on the tables. Dtarget2: the same row
 * at twice the size */
#ifndef STG09_DROP_TARGET_H
#define STG09_DROP_TARGET_H

#include "system/task.h"

/* the object list's steps. task needs TASK_WORK's record. a hit of a
 * standing piece sinks it. the hit turns player 0 to minus its y angle. it
 * then sends player 0 off at half its forward speed */
void drop_target_step(struct task *task);
void drop_target2_step(struct task *task);

#endif
