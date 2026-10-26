/* the west rock in the station act, which breaks apart once player 0's
 * character has its west rock flag set */
#ifndef ADV02_WEST_ROCK_H
#define ADV02_WEST_ROCK_H

#include "system/task.h"

/* task step the Mystic Ruins field task creates in the station act. the task
 * needs TASK_FORCE's record, whose turn and speed the step zeroes */
void west_rock_step(struct task *task);

#endif
