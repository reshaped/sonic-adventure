/* O SCENECHG cylinder that sends player 0 into the stage its set entry names */
#ifndef ADV02_SCENE_CHANGE_H
#define ADV02_SCENE_CHANGE_H

#include "system/task.h"

/* the step the object list names for O SCENECHG: a cylinder of radius scl.x
 * reaching scl.y above and below the task. player 0 inside it goes where ang.x
 * says, if the character may and no event has control. the task then ends, and
 * its placement makes no more tasks until placement_reset. while the set editor
 * holds the task, the step draws the cylinder instead */
void mystic_ruins_scene_change_step(struct task *task);

#endif
