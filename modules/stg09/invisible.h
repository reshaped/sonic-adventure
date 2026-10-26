/* O CRECT2, O CCYL2 and O CCAP: a solid box, cylinder and capsule. only the
 * set editor draws them. the box turns with its task's y angle, and the
 * others with all three of its angles */
#ifndef STG09_INVISIBLE_H
#define STG09_INVISIBLE_H

#include "system/task.h"

/* the object list's steps. a shape's radius, or the box's half size along x,
 * is its task's scl.x. its half height, not counting a capsule's round ends,
 * is scl.y. the box's half size along z is scl.z */
void invisible_rectangle2_step(struct task *task);
void invisible_cylinder2_step(struct task *task);
void invisible_capsule_step(struct task *task);

#endif
