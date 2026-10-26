/* O OFFICE DOOR pair of office doors, which open only for Knuckles, once he
 * has knocked the barricade down */
#ifndef ADV00_OFFICE_DOOR_H
#define ADV00_OFFICE_DOOR_H

#include "system/task.h"

/* the step the object list names for O OFFICE DOOR. a pair it keeps clears
 * FLAG_OFFICE_BARRICADE. locked doors swing open by themselves once the
 * thrown barricade sets that flag as it lands, and stay open */
void office_door_init(struct task *task);

#endif
