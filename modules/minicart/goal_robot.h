/* GOAL ROBOT: two robots by the goal that wave their flags on the last
 * lap's home stretch */
#ifndef MINICART_GOAL_ROBOT_H
#define MINICART_GOAL_ROBOT_H

#include "system/task.h"

/* the step cart_object_kinds names for GOAL ROBOT. task needs a work, whose
 * pos and ang place the pair. its scl.z is how far each robot stands from
 * pos. once placement_try_leave finds the pair out of range, the task
 * destroys itself on its next step */
void goal_robot_step(struct task *task);

#endif
