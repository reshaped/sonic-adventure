/* the hall's light glows: a sprite at each of the hall's 34 lamps, all
 * pulsing together */
#ifndef ADV0130_HALL_LIGHTS_H
#define ADV0130_HALL_LIGHTS_H

#include "system/task.h"

/* the step of the task egg_carrier_inside_step creates in act 1 */
void egg_carrier_hall_lights_step(struct task *task);

#endif
