/* O BARRICADDE police barricade across the road, which blocks Sonic and Tails
 * until their stories clear it */
#ifndef ADV00_BARRICADE_H
#define ADV00_BARRICADE_H

#include "system/task.h"

/* the step the object list names for O BARRICADDE. outside the station act
 * it prints ACT STATION ONLY ! and stays the task's step, so the message
 * shows every step */
void barricade_init(struct task *task);

#endif
