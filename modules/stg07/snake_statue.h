/* the snake statues: HebiZou, HebiZouL and HebiZouR */
#ifndef STG07_SNAKE_STATUE_H
#define STG07_SNAKE_STATUE_H

#include "system/task.h"

/* the object list's steps */
void snake_statue_step(struct task *task);
void snake_statue_l_step(struct task *task);
void snake_statue_r_step(struct task *task);

#endif
