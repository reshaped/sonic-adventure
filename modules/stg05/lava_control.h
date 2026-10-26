/* MagmaCnt, a box that sets the lava rising the first time player 0 touches
 * it. its set entry gives the level and the speed */
#ifndef STG05_LAVA_CONTROL_H
#define STG05_LAVA_CONTROL_H

#include "system/task.h"

/* the step mountain_object_list names for MagmaCnt */
void lava_control_step(struct task *task);

#endif
