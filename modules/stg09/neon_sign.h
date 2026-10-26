/* O M1FNEO neon sign, whose tubes step through their faces */
#ifndef STG09_NEON_SIGN_H
#define STG09_NEON_SIGN_H

#include "system/task.h"

/* the object list's step. the tubes show their own face, then three
 * shifted faces, and round again. each face shows for scl.z + 2 steps,
 * scl.z truncated */
void neon_sign_step(struct task *task);

#endif
