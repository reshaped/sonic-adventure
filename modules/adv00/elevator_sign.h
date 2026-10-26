/* O ELEKANBAN elevator sign, lit at night */
#ifndef ADV00_ELEVATOR_SIGN_H
#define ADV00_ELEVATOR_SIGN_H

#include "system/task.h"

/* the step the object list names for O ELEKANBAN. by night the sign draws with
 * NJD_FLAG_IGNORE_LIGHT, so the scene's lights don't shade it */
void elevator_sign_init(struct task *task);

#endif
