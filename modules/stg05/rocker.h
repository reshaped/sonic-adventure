/* O HAGO, a base with two rails. an arm on it rocks about x, 15 degrees each
 * way */
#ifndef STG05_ROCKER_H
#define STG05_ROCKER_H

#include "system/task.h"

/* the step mountain_object_list names for O HAGO. unless the set editor holds
 * the rocker, its first step adds a copy to the world. the rocker writes
 * through null when every object of the world's pool is lent */
void rocker_step(struct task *task);

#endif
