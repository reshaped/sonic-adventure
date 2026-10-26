/* O Cone1 and O Cone2 traffic cones that a running player kicks away */
#ifndef STG04_CONE_H
#define STG04_CONE_H

#include "system/task.h"

/* task steps for O Cone1 and O Cone2, named in the object list */
void cone_1_step(struct task *task);
void cone_2_step(struct task *task);

#endif
