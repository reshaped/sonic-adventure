/* the wheel the object list names O ROTARY, which turns a third of a turn
 * each time ROTARY_TURN is set in its event flag */
#ifndef ADV0130_ROTARY_H
#define ADV0130_ROTARY_H

#include "system/task.h"

/* the wheel's byte of event_flags, and its bits. setting ROTARY_TURN asks for
 * a turn. a turn clears ROTARY_DONE as it starts. as it ends, it clears
 * ROTARY_TURN and sets ROTARY_DONE, so a turn asked for during one is lost.
 * the wheel's first step clears the byte */
#define FLAG_ROTARY 37
#define ROTARY_TURN 0x1
#define ROTARY_DONE 0x2

/* the object list's step */
void egg_carrier_rotary_step(struct task *task);

#endif
