/* O DMGSPHERE: an invisible sphere that hurts players */
#ifndef STG09_DAMAGE_SPHERE_H
#define STG09_DAMAGE_SPHERE_H

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. the sphere's
 * radius is |scl.x| */
void damage_sphere_step(struct task *task);

#endif
