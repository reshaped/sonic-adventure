/* O S1DNTO lamp of the first shop, lit in the evening and at night */
#ifndef ADV00_SHOP1_LAMP_H
#define ADV00_SHOP1_LAMP_H

#include "system/task.h"

/* the step the object list names for O S1DNTO. it starts the lamp on its
 * first step while the set editor is closed, and draws it on each after */
void shop1_lamp_step(struct task *task);

#endif
