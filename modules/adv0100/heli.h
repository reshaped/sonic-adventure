/* O HELI helicopter, which kills its placement on its first step and so is
 * never drawn */
#ifndef ADV0100_HELI_H
#define ADV0100_HELI_H

#include "system/task.h"

/* task step for O HELI, named in the object list */
void egg_carrier_heli_step(struct task *task);
/* draw the helicopter at work's position and angles. the main rotor turns
 * about y by motion->aim.x, and the tail rotor by motion->aim.z. nothing in
 * this image calls it */
void egg_carrier_heli_draw(struct task_work *work, struct task_motion *motion);

#endif
