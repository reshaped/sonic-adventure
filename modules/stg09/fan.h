/* O FANFAN: a fan that lifts players in its column of air and blows bits
 * about */
#ifndef STG09_FAN_H
#define STG09_FAN_H
#include <sg_xpt.h>

#include "system/task.h"

/* the object list's step. task needs TASK_WORK's record. scl.x and scl.y
 * size the column, and scl.z gives the fan its number */
void fan_step(struct task *task);
/* 1 while the fan with that number isn't blowing, and 0 while it is. the
 * result is also 0 before that fan has run. a fan sets its state outside the
 * set editor, on steps with player 0 or 1 near. number must not be negative.
 * a number of 5 or more returns 0 */
int fan_idle(Sint8 number);

#endif
