/* O EC DOORS sliding door, and O EC DOOR, the same door with a lintel */
#ifndef ADV0100_DOOR_H
#define ADV0100_DOOR_H

#include "system/task.h"

/* the object list's steps. egg_carrier_door_step, O EC DOOR's, gives the door
 * its lintel and hands over to egg_carrier_doors_step, O EC DOORS's */
void egg_carrier_door_step(struct task *task);
void egg_carrier_doors_step(struct task *task);

#endif
