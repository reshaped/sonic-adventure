/* the casino act's ornament, whose textures scroll and whose light blinks
 * by night, with a row of ten lamps */
#ifndef ADV00_CASINO_ORNAMENT_H
#define ADV00_CASINO_ORNAMENT_H

#include "system/task.h"

/* the step of the ornament's task. create the task with TASK_WORK. by night
 * it scrolls the ornament's textures, blinks its light, and starts a child
 * that lights the lamps in patterns. the task then ends once the stage or act
 * changes. by day and in the evening it draws the ornament, its parts and the
 * lamps each step, and lasts past a change of act */
void casino_ornament_init(struct task *task);

#endif
