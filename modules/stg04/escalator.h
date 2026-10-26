/* O Escalator1 and O Escalator2 escalators whose steps scroll */
#ifndef STG04_ESCALATOR_H
#define STG04_ESCALATOR_H

#include "system/task.h"

/* task steps for O Escalator1 and O Escalator2, named in the object list */
void escalator_1_step(struct task *task);
void escalator_2_step(struct task *task);

#endif
