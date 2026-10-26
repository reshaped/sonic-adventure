/* O HIDDENDOOR door whose two leaves slide apart when its switch is on, and
 * close again after a set time */
#ifndef ADV00_HIDDEN_DOOR_H
#define ADV00_HIDDEN_DOOR_H

#include "system/task.h"

/* the step the object list names for O HIDDENDOOR. the door's switch is
 * |scl.x|, truncated, which must be under 32: switch_set and switch_is_on take
 * it unchecked. this turns the switch's flag off. once open, the door waits the
 * truncated |scl.y| steps before it closes, and at least one. |scl.y| must fit
 * an int, since nothing checks its conversion */
void hidden_door_init(struct task *task);

#endif
