/* C SCENECHG, a cylinder that sends player 0 to PAST_CHAOS or PAST_KNUCKLES */
#ifndef ADV03_SCENE_CHANGE_H
#define ADV03_SCENE_CHANGE_H

#include "system/task.h"

/* the step the object list names for C SCENECHG. once player 0 is inside the
 * cylinder, the step sends it to entrance 0 of the act the placement's ang.x
 * names. the task then destroys itself on its next step. while the set editor
 * holds the task, the step draws the cylinder instead */
void scene_change_step(struct task *task);

#endif
