/* O CONNECTION, a coupling between two cargo cars */
#ifndef STG12_CONNECTION_H
#define STG12_CONNECTION_H

#include "system/task.h"

/* the step the object lists of acts 2 and 3 name for O CONNECTION */
void connection_step(struct task *task);
/* the step cargo_train_start gives each coupling of the train */
void connection_train_step(struct task *task);

#endif
