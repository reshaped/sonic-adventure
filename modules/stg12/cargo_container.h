/* O CARGOCONTAINER, a container, A or B, that rides a cargo car */
#ifndef STG12_CARGO_CONTAINER_H
#define STG12_CARGO_CONTAINER_H

#include "system/task.h"

/* the step the object lists of acts 2 and 3 name for O CARGOCONTAINER */
void cargo_container_step(struct task *task);
/* the step cargo_train_start gives each container of the train */
void cargo_container_train_step(struct task *task);

#endif
