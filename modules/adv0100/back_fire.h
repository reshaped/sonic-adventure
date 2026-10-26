/* fires far below the Egg Carrier's deck, drawn while it flies */
#ifndef ADV0100_BACK_FIRE_H
#define ADV0100_BACK_FIRE_H

#include "system/task.h"

/* the step of a task with a work. the field task creates it as each of acts
 * 0 to 2 starts, while the Egg Carrier hasn't sunk. act 0 has an action of
 * its own, and acts 1 and 2 share one. in any other act the step draws
 * through a null action */
void egg_carrier_back_fire_step(struct task *task);

#endif
