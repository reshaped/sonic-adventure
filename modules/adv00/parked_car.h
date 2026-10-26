/* O ROTYU car parked over the way into the sewers, which Big lifts and carries
 * aside */
#ifndef ADV00_PARKED_CAR_H
#define ADV00_PARKED_CAR_H

#include "system/task.h"

/* the step the object list names for O ROTYU. outside the set editor, it
 * kills its placement when player 0 is Sonic and FLAG_SONIC_SS_ENTRANCE_SEWER
 * is set */
void parked_car_init(struct task *task);

#endif
