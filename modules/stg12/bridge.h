/* O BRIDGE, a walkway on an arm that turns about a hub. each time switch 6
 * turns on, the bridge swings nearly a quarter turn to its other stop */
#ifndef STG12_BRIDGE_H
#define STG12_BRIDGE_H

#include "system/task.h"

/* the step the object lists of acts 0 to 3 name for O BRIDGE */
void shelter_bridge_step(struct task *task);

#endif
