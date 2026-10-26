/* the train the object list names MONORAIL, inside the Egg Carrier. the
 * station calls its two cars in, and they carry player 0 out to the deck */
#ifndef ADV0130_MONORAIL_H
#define ADV0130_MONORAIL_H

#include "system/task.h"

/* the step the object list names for MONORAIL. it does nothing until the
 * character's EC_MONORAIL flag is set. its first step then allocates the
 * train's record at task->motion and makes the two cars as children. it
 * writes through null if the heap runs out. the destructor destroys the
 * children and frees the record
 *
 * the step disables both players' pads as the train comes in and as player
 * 0 boards. it enables them only as the doors open at the station. when the
 * act starts at entrance 9, that first step disables them too and sends the
 * train off empty. they then stay disabled until the train next opens its
 * doors at the station */
void egg_carrier_monorail_step(struct task *task);

#endif
