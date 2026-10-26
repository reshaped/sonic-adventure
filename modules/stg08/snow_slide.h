/* the snow slides that chase player 0 down the board run once the board is
 * taken. the lower one comes to rest at a launch */
#ifndef STG08_SNOW_SLIDE_H
#define STG08_SNOW_SLIDE_H

#include "system/task.h"

/* the step a task with a work and no collision record starts with. it makes
 * the task the lower slide, and creates the upper slide at level 3. the task
 * pool and the heap must have room for every task it creates. with none, it
 * writes through null. the slides change their models in place. of those
 * changes, the lower slide's destructor puts back only its model's heights,
 * as this step found them. the step clears the release, so only a later
 * snow_slide_release lets the slides move */
void snow_slide_step(struct task *task);
/* let the slides move. the upper one starts at once. the lower one starts
 * once player 0 is below -700, snow_slide.c's LOWER_START */
void snow_slide_release(void);

#endif
