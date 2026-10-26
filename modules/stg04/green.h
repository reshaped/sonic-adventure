/* O GREEN and O GREENE hedge beds that are only drawn, and O GREENA, O GREENB
 * and O GREEND solid bushes */
#ifndef STG04_GREEN_H
#define STG04_GREEN_H

#include "system/task.h"

/* task steps, one for each object above, named in the object list */
void green_step(struct task *task);
void green_a_step(struct task *task);
void green_b_step(struct task *task);
void green_d_step(struct task *task);
void green_e_step(struct task *task);

#endif
