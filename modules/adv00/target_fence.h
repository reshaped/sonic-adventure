/* O M SAKU fence carrying three targets for E-102, which tips over once he
 * shoots them all */
#ifndef ADV00_TARGET_FENCE_H
#define ADV00_TARGET_FENCE_H

#include "system/task.h"

/* the step the object list names for O M SAKU. outside the set editor, it kills
 * its placement while player 0 is Sonic or Big, or E-102 once
 * FLAG_E102_SS_HOTEL_POOL is set. a fence it keeps carries targets for E-102,
 * and stands plain for any other character */
void target_fence_init(struct task *task);
/* the plain fence's step, which no object list names */
void target_fence_plain_init(struct task *task);

#endif
