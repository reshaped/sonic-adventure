/* O BRIDGE, a bridge that rises out of the way while FLAG_BRIDGE_UP is set */
#ifndef ADV0130_BRIDGE_H
#define ADV0130_BRIDGE_H

#include "system/task.h"

/* the event flag that raises the bridge while it is set.
 * modules/adv0130/monorail.c sets it as the monorail comes in */
#define FLAG_BRIDGE_UP 38

/* the step egg_carrier_inside_object_kinds names for O BRIDGE */
void egg_carrier_bridge_step(struct task *task);

#endif
