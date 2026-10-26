/* O FENA, O FENB, O FENC and O FEND solid fences of four lengths */
#ifndef STG01_FENCES_H
#define STG01_FENCES_H

#include "system/task.h"

/* the steps the object list names for O FENA, O FENB, O FENC and O FEND.
 * each task needs a work. outside the set editor, each fence's box pushes
 * back players, enemies, objects and pickups. none of them pushes it */
void beach_fence_a_step(struct task *task);
void beach_fence_b_step(struct task *task);
void beach_fence_c_step(struct task *task);
void beach_fence_d_step(struct task *task);

#endif
