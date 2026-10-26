/* O SLOTKAN, a slot sign with spinning parts and changing faces */
#ifndef STG09_SLOT_SIGN_H
#define STG09_SLOT_SIGN_H

#include "system/task.h"

/* the object list's step. scl.x and scl.y turn the upper and middle parts,
 * in degrees a step. each face shows for scl.z + 2 steps */
void slot_sign_step(struct task *task);

#endif
