/* O ARCHROCK, a rock arch standing on two big rocks. its top falls in
 * pieces once either rock starts to topple */
#ifndef STG01_ARCH_ROCK_H
#define STG01_ARCH_ROCK_H

#include "system/task.h"

/* the step the object list names for O ARCHROCK. outside the set editor, it
 * gives the top a capsule and starts two big rocks as children. big rock a
 * stands ahead and big rock b behind. the capsule stays solid after the top
 * has fallen */
void arch_rock_init(struct task *task);

#endif
