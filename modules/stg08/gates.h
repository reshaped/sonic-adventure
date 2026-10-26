/* O Kuguri: snowboard gates, and the combo they score. they stand only in a
 * mini game's trial */
#ifndef STG08_GATES_H
#define STG08_GATES_H

#include "system/task.h"

/* the object list's step */
void icecap_gate_step(struct task *task);
/* the combo's step, which the stage task creates for the board run */
void icecap_gate_combo_step(struct task *task);

#endif
