/* O BEWIND box of rising wind that lifts player 0 */
#ifndef STG01_WIND_H
#define STG01_WIND_H

#include "system/task.h"

/* the object list's step for O BEWIND, whose task needs a work. scl is the
 * box's size, held to 10 to 200 along each axis. the box turns about y by
 * -ang.y. ang.x's low byte is the wind's power, held to 1 to 8 */
void beach_wind_step(struct task *task);

#endif
