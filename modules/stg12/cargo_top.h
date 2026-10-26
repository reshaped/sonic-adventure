/* O CARGOTOP, the car at the head of the cargo train, which loops an action */
#ifndef STG12_CARGO_TOP_H
#define STG12_CARGO_TOP_H

#include "system/task.h"

/* the step the object lists of acts 2 and 3 name for O CARGOTOP */
void cargo_top_step(struct task *task);
/* the step cargo_train_start gives the train's head car */
void cargo_top_train_step(struct task *task);

#endif
