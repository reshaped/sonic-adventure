/* O JDOOR, a gate that rises once its flag is 1, and O JSWITCH, the floor
 * switch that sets the flag to 1 */
#ifndef ADV0130_JDOOR_H
#define ADV0130_JDOOR_H

#include "system/task.h"

/* a gate's or a switch's event flag is JDOOR_FLAGS plus its set entry's x
 * angle. a gate and a switch of the same angle share the flag */
#define JDOOR_FLAGS 33

/* the bit an event's script sets in a switch's flag to press the switch. the
 * switch clears the bit and goes down, as it does when a player stands on
 * it */
#define JSWITCH_PRESS 0x4

/* the steps egg_carrier_inside_object_kinds names for O JDOOR and O
 * JSWITCH */
void egg_carrier_jdoor_step(struct task *task);
void egg_carrier_jswitch_step(struct task *task);

#endif
