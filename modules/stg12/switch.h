/* C SWITCH, an invisible box that turns a switch on once a player touches it */
#ifndef STG12_SWITCH_H
#define STG12_SWITCH_H

#include "system/task.h"

/* the step each act's object list names for C SWITCH */
void shelter_switch_step(struct task *task);
/* the step of a box like C SWITCH's, which a player's touch ends without
 * turning a switch on. no word in the image or in 1ST_READ.BIN points at
 * it */
void shelter_touch_box_step(struct task *task);

#endif
