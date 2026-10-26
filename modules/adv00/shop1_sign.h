/* O S1KNBN sign of the first shop, with a day face by day and a night face in
 * the evening and at night */
#ifndef ADV00_SHOP1_SIGN_H
#define ADV00_SHOP1_SIGN_H

#include "system/task.h"

/* the step the object list names for O S1KNBN. it starts the sign on its
 * first step while the set editor is closed. on each after, it registers the
 * sign's collision and draws it */
void shop1_sign_step(struct task *task);

#endif
