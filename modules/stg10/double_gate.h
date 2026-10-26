/* O DOUBLE_GATE two doors whose leaves slide apart while the nearest player
 * is near. for E-102, they stay shut */
#ifndef STG10_DOUBLE_GATE_H
#define STG10_DOUBLE_GATE_H

#include "system/task.h"

/* the object list's step */
void double_gate_step(struct task *task);

#endif
