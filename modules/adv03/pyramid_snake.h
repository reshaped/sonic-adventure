/* O SNAKE, the snake on the pyramid, a model that only draws */
#ifndef ADV03_PYRAMID_SNAKE_H
#define ADV03_PYRAMID_SNAKE_H

#include "system/task.h"

/* the step the object list names for O SNAKE. it draws the snake on each
 * step. once placement_try_leave finds the task out of range, the step does
 * nothing, and the task destroys itself on its next step */
void pyramid_snake_step(struct task *task);

#endif
