/* SitaAlEgg and UeAlEgg, eggs that loop along the table's lower and upper
 * routes */
#ifndef STG09_ROUTE_EGG_H
#define STG09_ROUTE_EGG_H

#include "system/task.h"

/* the object list's steps. each egg moves 1 along its route a step, and
 * starts it again just short of its end */
void route_egg_lower_step(struct task *task);
void route_egg_upper_step(struct task *task);

#endif
