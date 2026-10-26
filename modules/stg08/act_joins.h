/* ACT23JOIN joins act 1 to act 2 with solid ground, and ACT12JOIN act 0 to
 * act 1. their names count acts from 1. each join ends itself in
 * any act but the two it joins */
#ifndef STG08_ACT_JOINS_H
#define STG08_ACT_JOINS_H

#include "system/task.h"

/* the joins' steps, which the object list names. as act 1 starts, the
 * stage's task creates ACT12JOIN, unless the character is Big. as act 2
 * starts, it creates ACT23JOIN */
void icecap_act23_join_step(struct task *task);
void icecap_act12_join_step(struct task *task);

#endif
