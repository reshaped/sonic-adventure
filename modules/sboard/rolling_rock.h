/* the rolling rocks, which roll down three routes as a player comes near */
#ifndef SBOARD_ROLLING_ROCK_H
#define SBOARD_ROLLING_ROCK_H

#include "system/task.h"

/* create a route's watcher as a child of the act's parent, and return it
 * as act_task_create does. a route takes one watcher at a time. a second
 * watcher made for a route while the first lives destroys itself on its
 * next step */
struct task *rolling_rock_a_create(void);
struct task *rolling_rock_b_create(void);
struct task *rolling_rock_c_create(void);

#endif
