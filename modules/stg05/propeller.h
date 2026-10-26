/* O PROPELLER, a fan that turns and bobs, carrying players who stand on it */
#ifndef STG05_PROPELLER_H
#define STG05_PROPELLER_H

#include "system/task.h"

/* the step mountain_object_list names for O PROPELLER. unless the set editor
 * holds the fan, its first step adds a copy to the world. the fan writes
 * through null when every object of the world's pool is lent */
void mountain_propeller_step(struct task *task);

#endif
