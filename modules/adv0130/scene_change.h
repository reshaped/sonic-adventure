/* SCENE CHANGE, a cylinder that sends player 0 to another act of the Egg
 * Carrier inside, or to Hot Shelter */
#ifndef ADV0130_SCENE_CHANGE_H
#define ADV0130_SCENE_CHANGE_H

#include "system/task.h"

/* the object list's step */
void egg_carrier_inside_scene_change_step(struct task *task);

#endif
