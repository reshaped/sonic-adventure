/* O TOROKKO mine cart that carries player 0 along one of three tracks */
#ifndef ADV02_TRUCK_H
#define ADV02_TRUCK_H
#include <sg_xpt.h>

#include "system/task.h"

/* TRUE once a truck has set off with player 0 in it. no code sets it back to
 * FALSE: only its initializer does, each time ADV02 is read in */
extern Bool truck_ridden;

/* the step the object list names for O TOROKKO */
void truck_step(struct task *task);

#endif
