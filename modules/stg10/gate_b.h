/* O GATE_B door whose leaves slide apart while the nearest player is near,
 * and stay shut for E-102 */
#ifndef STG10_GATE_B_H
#define STG10_GATE_B_H

#include "system/task.h"

/* the object list's step */
void gate_b_step(struct task *task);

#endif
