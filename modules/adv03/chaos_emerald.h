/* O C EMERALD, a Chaos Emerald that turns and bobs */
#ifndef ADV03_CHAOS_EMERALD_H
#define ADV03_CHAOS_EMERALD_H

#include "system/task.h"

/* the step the object list names for O C EMERALD. the low byte of ang.x,
 * read as signed, picks the emerald. 0 to 6 pick red, yellow, green, sky,
 * blue, purple and white. any other value picks red. the emerald turns
 * about y, and bobs between 1 below pos.y and 1 above it. the step draws
 * the emerald. so does the task's draw, which task_draw_all runs while the
 * game is paused */
void past_chaos_emerald_step(struct task *task);

#endif
