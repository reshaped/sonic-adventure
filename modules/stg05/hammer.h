/* O HANMA, a hammer whose arm swings down onto the ground, raising dust,
 * and lifts again */
#ifndef STG05_HAMMER_H
#define STG05_HAMMER_H

#include "system/task.h"

/* task step for O HANMA, named in the object list. outside the set editor,
 * its first step makes a task for the head's sphere. it reads through null
 * when it can't */
void hammer_step(struct task *task);

#endif
