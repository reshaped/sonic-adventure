/* O S1SOLT salt in the first shop, drawn with no collision */
#ifndef ADV00_SHOP1_SALT_H
#define ADV00_SHOP1_SALT_H

#include "system/task.h"

/* the step the object list names for O S1SOLT. it starts the salt on its
 * first step while the set editor is closed, and draws it on each after */
void shop1_salt_step(struct task *task);

#endif
