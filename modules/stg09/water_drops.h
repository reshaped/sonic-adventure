/* O SUITEKI water drops, which fall one by one through a tank's height and
 * splash at its floor */
#ifndef STG09_WATER_DROPS_H
#define STG09_WATER_DROPS_H

#include "system/task.h"

/* the object list's step. ang.z's low byte is the count of drops, and scl is
 * half the tank's size along each axis. ang.x turns the tank about y, as
 * ang.y does */
void water_drops_step(struct task *task);

#endif
