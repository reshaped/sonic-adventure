/* O KBC and O KBS signs, whose lamps step through their faces */
#ifndef STG09_SIGN_H
#define STG09_SIGN_H

#include "system/task.h"

/* the object list's steps. the lamps show each face for scl.z + 2 steps.
 * O KBC's motion moves on by scl.y's magnitude, in frames, each step */
void sign_c_step(struct task *task);
void sign_s_step(struct task *task);

#endif
