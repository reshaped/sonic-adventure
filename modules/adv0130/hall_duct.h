/* O HALLDUCT, a duct drawn at a fixed place in the hall, whatever its set
 * entry's position. the set entry's y angle turns it */
#ifndef ADV0130_HALL_DUCT_H
#define ADV0130_HALL_DUCT_H

#include "system/task.h"

/* the step egg_carrier_inside_object_kinds names for O HALLDUCT */
void egg_carrier_hall_duct_step(struct task *task);

#endif
