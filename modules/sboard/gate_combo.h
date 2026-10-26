/* the gate combo: more points for each sand gate passed soon after the last */
#ifndef SBOARD_GATE_COMBO_H
#define SBOARD_GATE_COMBO_H

#include "system/task.h"

/* create the combo as a child of the act's parent, and return it as
 * act_task_create does */
struct task *gate_combo_create(void);
/* score a gate passed. while the combo task lives, each gate adds one to
 * the combo's count, up to 50. the gate then scores 10 points times the
 * count. the count goes back to 0 once 180 steps pass without a gate. with
 * no combo task, a gate scores 10 */
void gate_combo_add(void);

#endif
