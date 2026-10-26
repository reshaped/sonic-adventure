/* O ELEVATOR button panel that sends the elevator between its two floors */
#ifndef ADV0100_ELEVATOR_H
#define ADV0100_ELEVATOR_H

#include "system/task.h"

/* the event flags set while the elevator stands at floor 1 and at floor 0.
 * both are clear while it travels. as the panel starts, it sets floor 0's at
 * entrance 0, else floor 1's. the doors read them */
#define FLAG_ELEVATOR_AT_1 41
#define FLAG_ELEVATOR_AT_0 42

/* the object list's step */
void egg_carrier_elevator_step(struct task *task);

#endif
