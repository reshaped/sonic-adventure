/* the sand worms, which leap out of the sand down four paths while a player
 * is near */
#ifndef SBOARD_SAND_WORM_H
#define SBOARD_SAND_WORM_H

#include "system/task.h"

/* create a path's worm as a child of the act's parent, and return it as
 * act_task_create does. a path takes one worm at a time. a second worm made
 * for a path while the first lives destroys itself on its next step */
struct task *sand_worm_a_create(void);
struct task *sand_worm_b_create(void);
struct task *sand_worm_c_create(void);
struct task *sand_worm_d_create(void);

#endif
