/* O CARGO, a flat car of the cargo train */
#ifndef STG12_CARGO_CAR_H
#define STG12_CARGO_CAR_H

#include "system/task.h"

/* the step the object lists of acts 2 and 3 name for O CARGO */
void cargo_car_step(struct task *task);
/* the step cargo_train_start gives each car of the train */
void cargo_car_train_step(struct task *task);

#endif
