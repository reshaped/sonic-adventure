/* the machines O MEKA and O TUTU: deck machinery that plays its action over
 * and over */
#ifndef STG06_MACHINES_H
#define STG06_MACHINES_H

#include "system/task.h"

/* the steps the object list names for O MEKA and O TUTU */
void meka_step(struct task *task);
void tutu_step(struct task *task);

#endif
