/* O_HouseA to O_HouseD: houses that are only drawn */
#ifndef STG08_HOUSES_H
#define STG08_HOUSES_H

#include "system/task.h"

/* the object list's steps */
void house_a_step(struct task *task);
void house_b_step(struct task *task);
void house_c_step(struct task *task);
void house_d_step(struct task *task);

#endif
