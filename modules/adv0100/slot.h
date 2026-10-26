/* O SLOT, a slot machine that pulls its own lever, spins its drum to a stop
 * and plays again */
#ifndef ADV0100_SLOT_H
#define ADV0100_SLOT_H

#include "system/task.h"

/* the step egg_carrier_object_kinds names for O SLOT */
void egg_carrier_slot_step(struct task *task);

#endif
