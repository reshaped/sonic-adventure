/* the walking robot the object list names O W ROBO, which paces back and
 * forth and looks round at each end */
#ifndef ADV0130_W_ROBO_H
#define ADV0130_W_ROBO_H

#include "system/task.h"

/* the object list's step */
void egg_carrier_w_robo_step(struct task *task);
/* the step of a robot that casts a shadow, which no object list names */
void egg_carrier_w_robo_shadowed_step(struct task *task);

#endif
