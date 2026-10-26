/* O Hall Gate five-part gate that opens once the switch on its fourth part is
 * on */
#ifndef STG10_HALL_GATE_H
#define STG10_HALL_GATE_H

#include "system/task.h"

/* the object list's step */
void hall_gate_step(struct task *task);

#endif
