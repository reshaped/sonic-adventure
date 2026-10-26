/* the stand the object list names O ITEM STAND. it holds a turning item, which
 * a player's touch fades away. the item is back five seconds after the touch */
#ifndef ADV02_ITEM_STAND_H
#define ADV02_ITEM_STAND_H

#include "system/task.h"

/* the step the object list names for O ITEM STAND */
void item_stand_step(struct task *task);

#endif
